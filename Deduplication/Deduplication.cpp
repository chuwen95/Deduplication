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
		//������ʽ�����ֻ����ļ��еĴ������н���ȥ�ش�����Ҫ�õ�
		std::regex reg("^[0-9a-zA-Z]+$");

		//���Դ��ļ�
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
				//һ���ļ���ȡ��ɣ��ر��ļ�
				m_input.close();

				//�ļ�index����1�����µ��ļ�������û��ڳ���������ɾ�����ļ����������Դ���һ��
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

	//�����ǰ�ڵ���ĳ���ַ��������һ���ַ�
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

	//����ýڵ��к��ӽڵ�
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
	//��ʼ��0�Ż������ж�ȡ����
	readFile(0);
	{
		//�ȴ���һ����������ȡ���
		std::unique_lock<std::mutex> ulock(m_bufferReadMutex);
		m_bufferReadCv.wait(ulock, [this]() {return (m_wordsBuffers[m_currentBufferIndex].size() == m_bufferWords || true == m_isAllFileRead); });

		m_iterStart = m_wordsBuffers[0].begin();
		m_iterEnd = m_wordsBuffers[0].end();
	}

	//��ʼ��ڶ����������ж�ȡ����
	readFile(1);

	std::cout << "removing duplicates..." << std::endl;

	//ʹ�õ�һ���ʴ�����
	for (auto iter = m_iterStart->begin(); iter != m_iterStart->end(); ++iter)
	{
		//�����ڵ�
		LetterNode* p = new LetterNode;
		//��ֵ�ַ�
		p->letter = *iter;
		//����Ǵʵ����һ���ַ�
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
	//++m_remainNums;			//���Ա������ַ�������+1
	++m_iterStart;

	while (true)
	{
		//std::cout << m_iterEnd - m_iterStart << std::endl;

		while (m_iterStart != m_iterEnd)
		{
			//�ô��Ƿ��ظ�
			bool flag{ false };
			//�Ӹ��ڵ㿪ʼ�ж�
			LetterNode* curListHead{ head };

			//�ж�ÿһ���ַ�
			auto wordIter = m_iterStart->cbegin();
			while (wordIter != m_iterStart->cend())
			{
				LetterNode* pos{ nullptr };
				if (true == isLetterExistInCurrentStage(curListHead, pos, *wordIter))		//��ǰ�ַ��ڵ�ǰ�ײ��ҵ�
				{
					if (wordIter + 1 == m_iterStart->cend())		//�����ǰ�ַ��Ѿ����ַ��������һ���ַ�
					{
						if (pos->isEnd == true)		//������������������д��ַ�Ҳ�Ǹôʵ����һ���ַ����������ǰ���ظ�
						{
							//wordsIter->first = true;			//��¼��־����ʾӦ��ɾ��
							flag = true;		//�ظ���־��true
							break;		//�����жϵ�ǰ���Ƿ��ظ���ѭ��
						}
						else    //���ƴ���������д���1234����ǰ�ַ�Ϊ123�������е�3�ڵ��isEnd��Ϊtrue������123�ַ�����������
						{
							pos->isEnd = true;
							++wordIter;		//ǰ���ôʵ���һ���ַ�������������end������ѭ������һ����
						}
					}
					else					//�����ǰ�ַ������ַ��������һ���ַ�
					{
						++wordIter;			//ǰ����һ���ַ�
						if (nullptr == pos->child)		//������е�ǰ�ʵ�ʣ���ַ���������
						{
							while (wordIter != m_iterStart->cend())		//ֱ�ӽ�ʣ���ַ������뵽����
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
				else       //����ַ��ڵ�ǰ�ײ㲻����
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
				++m_removeNums;			//�Ѿ��Ƴ����ַ�������+1
			//else
				//++m_remainNums;			//���Ա������ַ�������+1

			++m_iterStart;
			++m_currentPad;

			if (m_currentPad % 10000 == 0)
				std::cout << "\r" << "deduplicated: " << m_removeNums;
		}

		//��¼�ո�ʹ�õĻ�������index
		size_t lastBufferIndex = m_currentBufferIndex;

		//ʹ����һ��������������
		m_currentBufferIndex = (m_currentBufferIndex + 1) % 2;

		//�����һ���������Ѿ���ȡ��m_bufferWords���ַ����������е��ļ����Ѿ���ȡ�����
		if (m_wordsBuffers[m_currentBufferIndex].size() == m_bufferWords || true == m_isAllFileRead);
		else
		{
			//����ȴ���ȡ����ź�
			std::unique_lock<std::mutex> ulock(m_bufferReadMutex);
			m_bufferReadCv.wait(ulock, [this]() {return m_wordsBuffers[m_currentBufferIndex].size() == m_bufferWords || true == m_isAllFileRead; });
		}

		m_iterStart = m_wordsBuffers[m_currentBufferIndex].begin();
		m_iterEnd = m_wordsBuffers[m_currentBufferIndex].end();

		//��ʼ���������ڽ������������Ѿ�û�еȴ�ȥ�ص��ַ���
		if (m_iterStart == m_iterEnd)
			break;

		//���ʹ�ù��Ļ�����
		{
			std::unique_lock<std::mutex> ulock(m_wordsBuffersMutex);
			m_wordsBuffers[lastBufferIndex].clear();
		}
		//��ջ������ж�ȡ����
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
