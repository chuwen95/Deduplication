#include <iostream>

#include "Deduplication.h"
#include "TimeStamp.hpp"
#include "FileTools.h"

int main()
{
	std::vector<std::string> filenames = FileTools::selectFiles();
	if (true == filenames.empty())
		return 0;
	std::string saveFilename = FileTools::selectSaveFilename();

	Deduplication deduplication(filenames, saveFilename);

	std::cout << "whether to keep numbers only rows(y/n): " << std::endl;
	char onlyLetterNum;
	std::cin >> onlyLetterNum;
	if ('y' == onlyLetterNum)
		deduplication.setOnlyNumLetter(true);

	CellTimestamp timeStamp;
	timeStamp.update();
	deduplication.deduplication();
	std::cout << std::endl << "time elapsed: " << timeStamp.getElapsedTimeInMilliSec() << "ms" << std::endl;
	std::cout << "saving data..." << std::endl;
	deduplication.saveTree();
	std::cout << "save data finished" << std::endl;
	
	system("pause");

	return 0;
}