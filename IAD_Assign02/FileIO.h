/*
*  FILE          : DEBUG
*  PROJECT       : DEBUG
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  DESCRIPTION   : DEBUG
*/


#ifndef FILEIO_H
#define FILEIO_H
#pragma once
#include "shared.h"
using namespace std;

/*
 * CLASS		: FileIO
 * DESCRIPTION	: This class is used to read in data from the specified file path, as a binary, or ASCII file.
 */
class FileIO final
{
	static fstream* file;
	static fstream* GetFileStream();


public:

	~FileIO();
	static string ReadBinaryFile(const string filepath);
	static string ReadAsciiFile(const string filepath);
};
#endif // FILEIO_H
