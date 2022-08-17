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
	//������Ҫȥ�ص��ļ�
	std::vector<std::string> m_fileNames;
	//����ļ������·��
	std::string m_savePath;

	//�ļ�����
	std::atomic_size_t m_fileIndex{ 0 };

	//ÿ���������Ķ�ȡ����
	size_t m_bufferWords{ 5000000 };
	//����index������˫�������
	std::atomic<size_t> m_currentBufferIndex{ 0 };
	//ʹ��˫���壬ÿ��������������m_bufferWords���ַ�������ֹһ���Խ������ļ����ݶ�ȡ���ڴ�ռ�ÿռ����
	std::mutex m_wordsBuffersMutex;
	std::vector<std::vector<std::string>> m_wordsBuffers;
	//ָ��ĳ������������ʼ�ͽ���������
	std::vector<std::string>::iterator m_iterStart, m_iterEnd;

	//��������: ��ǰ���������ݶ�ȡ��ɱ�־
	std::mutex m_bufferReadMutex;
	std::condition_variable m_bufferReadCv;

	//�Ƿ������ļ����Ѿ���ȡ��
	std::atomic_bool m_isAllFileRead{ false };
	//�ļ���ȡ ������
	std::ifstream m_input;

	using LetterNode = struct LetterNode
	{
		char letter;
		bool isEnd{ false };
		LetterNode *next{ nullptr };
		LetterNode *child{ nullptr };
	};
	LetterNode *head{ nullptr };		//����ͷ�ڵ�

	//������
	std::atomic_size_t m_sum{ 0 };
	//��ǰ���ڴ���ĸ���
	size_t m_currentPad{ 0 };
	//ȥ������
	std::atomic_size_t m_removeNums{ 0 };
	//���Ա���������
	size_t m_remainNums{ 0 };

	//�ļ����� ������
	std::ofstream m_output;

	size_t m_bufferOffset{0};
	size_t m_bufferSize{ 64 * 1024 * 1024 };
	char *m_outputBuffer{ nullptr };

	//�Ƿ�ֻ�Խ��������ֵ���ȥ��
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