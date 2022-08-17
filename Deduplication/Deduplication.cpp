#include "Deduplication.h"

#include <thread>
#include <fstream>
#include <algorithm>
#include <memory>
#include <iostream>
#include <regex>
#include <condition_variable>

Deduplication::Deduplication(const std::vector<std::string>& files, const std::string& savePath)
{
	m_wordsBuffers.resize(2);

	m_fileNames.assign(files.begin(), files.end());
	m_savePath = savePath;
}

void Deduplication::setOnlyNumLetter(const bool flag)
{
	m_onlyNumLetter = flag;
}

void Deduplication::readFile(const size_t bufferIndex)
{
	const auto expression = [this, bufferIndex]()
	{
		//正则表达式，如果只想对文件中的纯数字行进行去重处理，需要用到
		std::regex reg("^[0-9a-zA-Z]+$");

		//尝试打开文件
		if (false == m_input.is_open())
		{
			if (m_fileIndex == m_fileNames.size())
			{
				m_isAllFileRead = true;
				m_bufferReadCv.notify_all();
				return;
			}

			m_input.open(m_fileNames[m_fileIndex], std::ios::in);
		}

		std::string line;
		while (true)
		{
			if (true == m_input.eof())
			{
				//一个文件读取完成，关闭文件
				m_input.close();

				//文件index自增1，打开新的文件，如果用户在程序运行中删掉了文件，继续尝试打开下一个
				++m_fileIndex;
				if (m_fileNames.size() == m_fileIndex)
				{
					m_isAllFileRead = true;
					break;
				}

				m_input.open(m_fileNames[m_fileIndex], std::ios::in);
			}

			std::getline(m_input, line);
			if (true == line.empty())
				continue;

			if (true == m_onlyNumLetter && false == std::regex_match(line, reg))
			{
				++m_removeNums;
				continue;
			}

			{
				std::unique_lock<std::mutex> ulock(m_wordsBuffersMutex);

				m_wordsBuffers[bufferIndex].push_back(line);
				++m_sum;

				if (m_wordsBuffers[bufferIndex].size() == m_bufferWords || true == m_isAllFileRead)
					break;
			}
		}

		m_bufferReadCv.notify_all();
	};
	std::thread(expression).detach();
}

void Deduplication::saveChild(std::vector<char>& word, LetterNode* node)
{
	word.emplace_back(node->letter);

	//如果当前节点是某个字符串的最后一个字符
	if (true == node->isEnd)
	{
		if (m_bufferOffset + word.size() >= m_bufferSize)
		{
			m_output.write(m_outputBuffer, m_bufferOffset);
			m_bufferOffset = 0;
		}

		memcpy(m_outputBuffer + m_bufferOffset, word.data(), word.size());
		m_bufferOffset += word.size();
		memcpy(m_outputBuffer + m_bufferOffset, "\n", 1);
		m_bufferOffset += 1;
	}

	//如果该节点有孩子节点
	if (nullptr != node->child)
		saveChild(word, node->child);

	word.pop_back();

	if (nullptr != node->next)
	{
		std::vector<char> temp = word;
		saveNext(temp, node);
	}
}

void Deduplication::saveNext(std::vector<char>& word, LetterNode* node)
{
	node = node->next;
	saveChild(word, node);
}

bool Deduplication::isLetterExistInCurrentStage(LetterNode* head, LetterNode*& pos, const char letter)
{
	while (head != nullptr)
	{
		pos = head;
		if (head->letter == letter)
			return true;
		else
			head = head->next;
	}
	return false;
}

void Deduplication::findListLastPointer(LetterNode* ptr, LetterNode** res)
{
	*res = nullptr;
	while (ptr != nullptr)
	{
		*res = ptr;
		ptr = ptr->next;
	}
}

void Deduplication::findLastStage(LetterNode* ptr, LetterNode*& res)
{
	while (ptr != nullptr)
	{
		res = ptr;
		ptr = ptr->child;
	}
}

bool Deduplication::deduplication()
{
	//开始向0号缓冲区中读取内容
	readFile(0);
	{
		//等待第一个缓冲区读取完成
		std::unique_lock<std::mutex> ulock(m_bufferReadMutex);
		m_bufferReadCv.wait(ulock, [this]() {return (m_wordsBuffers[m_currentBufferIndex].size() == m_bufferWords || true == m_isAllFileRead); });

		m_iterStart = m_wordsBuffers[0].begin();
		m_iterEnd = m_wordsBuffers[0].end();
	}

	//开始向第二个缓冲区中读取内容
	readFile(1);

	std::cout << "removing duplicates..." << std::endl;

	//使用第一个词创建树
	for (auto iter = m_iterStart->begin(); iter != m_iterStart->end(); ++iter)
	{
		//创建节点
		LetterNode* p = new LetterNode;
		//赋值字符
		p->letter = *iter;
		//如果是词的最后一个字符
		if (iter + 1 == m_iterStart->end())
			p->isEnd = true;

		if (head == nullptr)
			head = p;
		else
		{
			LetterNode* lastStage{ nullptr };
			findLastStage(head, lastStage);
			lastStage->child = p;
		}
	}

	++m_currentPad;
	//++m_remainNums;			//得以保留的字符串数量+1
	++m_iterStart;

	while (true)
	{
		//std::cout << m_iterEnd - m_iterStart << std::endl;

		while (m_iterStart != m_iterEnd)
		{
			//该词是否重复
			bool flag{ false };
			//从根节点开始判断
			LetterNode* curListHead{ head };

			//判断每一个字符
			auto wordIter = m_iterStart->cbegin();
			while (wordIter != m_iterStart->cend())
			{
				LetterNode* pos{ nullptr };
				if (true == isLetterExistInCurrentStage(curListHead, pos, *wordIter))		//当前字符在当前阶层找到
				{
					if (wordIter + 1 == m_iterStart->cend())		//如果当前字符已经是字符串的最后一个字符
					{
						if (pos->isEnd == true)		//如果在曾经建立的树中此字符也是该词的最后一个字符，则表明当前词重复
						{
							//wordsIter->first = true;			//记录标志，表示应该删除
							flag = true;		//重复标志置true
							break;		//跳出判断当前词是否重复的循环
						}
						else    //类似此情况，树中存在1234，当前字符为123，则将树中的3节点的isEnd置为true，代表将123字符串加入树中
						{
							pos->isEnd = true;
							++wordIter;		//前往该词的下一个字符，即迭代器的end，将会循环到下一个词
						}
					}
					else					//如果当前字符不是字符串的最后一个字符
					{
						++wordIter;			//前往下一个字符
						if (nullptr == pos->child)		//如果树中当前词的剩余字符都不存在
						{
							while (wordIter != m_iterStart->cend())		//直接将剩余字符都加入到树中
							{
								LetterNode* p = new LetterNode;
								p->letter = *wordIter;
								if (wordIter + 1 == m_iterStart->cend())
									p->isEnd = true;

								pos->child = p;
								pos = p;

								++wordIter;
							}
							break;
						}
						curListHead = pos->child;
					}
				}
				else       //如果字符在当前阶层不存在
				{
					LetterNode* p = new LetterNode;
					p->letter = *wordIter;
					if (wordIter + 1 == m_iterStart->cend())
						p->isEnd = true;

					p->next = curListHead->next;
					curListHead->next = p;
					curListHead = p;

					++wordIter;
					while (wordIter != m_iterStart->cend())
					{
						LetterNode* p = new LetterNode;
						p->letter = *wordIter;
						if (wordIter + 1 == m_iterStart->cend())
							p->isEnd = true;

						curListHead->child = p;
						curListHead = p;

						++wordIter;
					}
				}
			}

			if (flag == true)
				++m_removeNums;			//已经移除的字符串数量+1
			//else
				//++m_remainNums;			//得以保留的字符串数量+1

			++m_iterStart;
			++m_currentPad;

			if (m_currentPad % 10000 == 0)
				std::cout << "\r" << "deduplicated: " << m_removeNums;
		}

		//记录刚刚使用的缓冲区的index
		size_t lastBufferIndex = m_currentBufferIndex;

		//使用另一个缓冲区的数据
		m_currentBufferIndex = (m_currentBufferIndex + 1) % 2;

		//如果另一个缓冲区已经读取到m_bufferWords个字符串或者所有的文件都已经读取完成了
		if (m_wordsBuffers[m_currentBufferIndex].size() == m_bufferWords || true == m_isAllFileRead);
		else
		{
			//否则等待读取完成信号
			std::unique_lock<std::mutex> ulock(m_bufferReadMutex);
			m_bufferReadCv.wait(ulock, [this]() {return m_wordsBuffers[m_currentBufferIndex].size() == m_bufferWords || true == m_isAllFileRead; });
		}

		m_iterStart = m_wordsBuffers[m_currentBufferIndex].begin();
		m_iterEnd = m_wordsBuffers[m_currentBufferIndex].end();

		//起始迭代器等于结束迭代器，已经没有等待去重的字符串
		if (m_iterStart == m_iterEnd)
			break;

		//清空使用过的缓冲区
		{
			std::unique_lock<std::mutex> ulock(m_wordsBuffersMutex);
			m_wordsBuffers[lastBufferIndex].clear();
		}
		//向空缓冲区中读取数据
		readFile(lastBufferIndex);
	}
	std::cout << std::endl << "finished! sum: " << m_sum << ", deduplicated: " << m_removeNums << std::endl;

	return true;
}

void Deduplication::saveTree()
{
	std::vector<char> word;
	m_output.open(m_savePath, std::ios::out);

	m_outputBuffer = new char[m_bufferSize];
	memset(m_outputBuffer, 0, m_bufferSize);

	saveChild(word, head);

	m_output.write(m_outputBuffer, m_bufferOffset);
	m_bufferOffset = 0;

	delete[] m_outputBuffer;

	m_output.close();
}
