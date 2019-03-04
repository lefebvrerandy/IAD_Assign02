/*
*  FILE          : FileIO.h
*  PROJECT       : CNTR 2115 - A02
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  DESCRIPTION   : This file contains the FileIO class. The class was created with the intent of providing 
*				   the program with capacity to read and write files in ascii, or binary format. 
*/


#ifndef FILEIO_H
#define FILEIO_H
#pragma once
#include "shared.h"
using namespace std;


/*
 * CLASS		: FileIO
 * DESCRIPTION	: This class is used to read in data from the specified file path, as a binary, or ASCII file.
 *				  It also provides methods for writing files received over the network.
 */
class FileIO final
{
	static fstream* file;
	static fstream* GetFileStream();

public:

	~FileIO();
	static string ReadBinaryFile(const string filepath);
	static string ReadAsciiFile(const string filepath);
	static void WriteBinaryFile(const string filepath, const string filePath);
	static void WriteAsciiFile(const string file, const string filePath);
};
#endif // FILEIO_H
