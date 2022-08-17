#ifndef FILETOOLS_H
#define FILETOOLS_H

#include <vector>
#include <string>

class FileTools
{
public:
	//打开文件对话框，选择文件
	static std::vector<std::string> selectFiles();
	//选择一个文件保存路径
	static std::string selectSaveFilename();
};

#endif