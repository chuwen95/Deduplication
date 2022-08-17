#ifndef DEDUPLICATION_H
#define DEDUPLICATION_H

#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <fstream>
#include <condition_variable>

class Deduplication
{
public:
	Deduplication(const std::vector<std::string>& files, const std::string& savePath);

	void setOnlyNumLetter(const bool flag);

	bool deduplication();
	void saveTree();

private:
	//所有需要去重的文件
	std::vector<std::string> m_fileNames;
	//结果文件保存的路径
	std::string m_savePath;

	//文件索引
	std::atomic_size_t m_fileIndex{ 0 };

	//每个缓冲区的读取行数
	size_t m_bufferWords{ 5000000 };
	//缓冲index，采用双缓冲机制
	std::atomic<size_t> m_currentBufferIndex{ 0 };
	//使用双缓冲，每个缓冲区仅保存m_bufferWords个字符串，防止一次性将所有文件内容读取到内存占用空间过大
	std::mutex m_wordsBuffersMutex;
	std::vector<std::vector<std::string>> m_wordsBuffers;
	//指向某个缓冲区的起始和结束迭代器
	std::vector<std::string>::iterator m_iterStart, m_iterEnd;

	//条件变量: 当前缓冲区数据读取完成标志
	std::mutex m_bufferReadMutex;
	std::condition_variable m_bufferReadCv;

	//是否所有文件都已经读取完
	std::atomic_bool m_isAllFileRead{ false };
	//文件读取 流对象
	std::ifstream m_input;

	using LetterNode = struct LetterNode
	{
		char letter;
		bool isEnd{ false };
		LetterNode *next{ nullptr };
		LetterNode *child{ nullptr };
	};
	LetterNode *head{ nullptr };		//树的头节点

	//总数量
	std::atomic_size_t m_sum{ 0 };
	//当前正在处理的个数
	size_t m_currentPad{ 0 };
	//去重数量
	std::atomic_size_t m_removeNums{ 0 };
	//得以保留的数量
	size_t m_remainNums{ 0 };

	//文件保存 流对象
	std::ofstream m_output;

	size_t m_bufferOffset{0};
	size_t m_bufferSize{ 64 * 1024 * 1024 };
	char *m_outputBuffer{ nullptr };

	//是否只对仅包含数字的行去重
	bool m_onlyNumLetter{ false };

private:
	bool isLetterExistInCurrentStage(LetterNode* head, LetterNode*& pos, const char letter);
	void findListLastPointer(LetterNode* ptr, LetterNode** res);
	void findLastStage(LetterNode* ptr, LetterNode*& res);

	void readFile(const size_t bufferIndex);
	void saveChild(std::vector<char>&, LetterNode*);
	void saveNext(std::vector<char>&, LetterNode*);
};

#endif