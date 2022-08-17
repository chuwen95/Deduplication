#include "FileTools.h"

#include <windows.h>
#include <iostream>

std::vector<std::string> FileTools::selectFiles()
{
	std::vector<std::string> filenames;

	char buffer[1024] = "\0";

	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "Text file(*.txt)\0*.txt\0";//要选择的文件后缀
	ofn.lpstrInitialDir = "./";//默认的文件路径
	ofn.lpstrFile = buffer;//存放文件的缓冲区
	ofn.nMaxFile = sizeof(buffer);
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT; //标志如果是多选要加上OFN_ALLOWMULTISELECT 
	BOOL ret = GetOpenFileName(&ofn);
	if (FALSE == ret)
		return filenames;

	std::string path = buffer;

	char* p = buffer + path.size() + 1;
	while ('\0' != *p)
	{
		filenames.push_back(path + "\\" + p);
		p += (strlen(p) + 1);
	}

	if (true == filenames.empty())
		filenames.push_back(path);

	return filenames;
}

std::string FileTools::selectSaveFilename()
{
	OPENFILENAME ofn;		//公共对话框
	TCHAR szFile[MAX_PATH]; // 保存获取文件名称的缓冲区。          

	//初始化选择文件对话框
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;

	//
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "*.txt";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

	// 显示打开选择文件对话框。
	if (GetSaveFileName(&ofn))
	{
		//显示选择的文件
		std::string path = szFile;
		if (std::string::npos == path.find(".txt"))
			path += ".txt";
		return path;
	}
	else
		return std::string();
}
