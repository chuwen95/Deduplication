#ifndef FILETOOLS_H
#define FILETOOLS_H

#include <vector>
#include <string>

class FileTools
{
public:
	//���ļ��Ի���ѡ���ļ�
	static std::vector<std::string> selectFiles();
	//ѡ��һ���ļ�����·��
	static std::string selectSaveFilename();
};

#endif