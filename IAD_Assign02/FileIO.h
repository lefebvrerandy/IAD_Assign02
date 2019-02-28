


#ifndef FILEIO_H
#define FILEIO_H
#pragma once
#include <iostream>
#include <fstream>
#include <string>
using namespace std;


/*
 * CLASS		:
 * DESCRIPTION	:
 */
class FileIO
{
	static fstream* file;
	static fstream* GetFileStream();


public:

	~FileIO();
	static string ReadBinaryFile(const string filepath);
	static string ReadAsciiFile(const string filepath);
};
#endif // FILEIO_H
