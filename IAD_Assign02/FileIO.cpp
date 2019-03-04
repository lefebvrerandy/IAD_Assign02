/*
*  FILE          : DEBUG
*  PROJECT       : DEBUG
*  PROGRAMMER    : Randy Lefebvre & Bence Karner
*  FIRST VERSION : DEBUG
*  DESCRIPTION   : DEBUG
*/


#include "FileIO.h"
#include <fstream>
#include <chrono>
#include <vector>
#include <cstdint>
#include <numeric>
#include <random>
#include <algorithm>
#include <iostream>
#include <cassert>

fstream* FileIO::file = new fstream();




std::vector<uint64_t> GenerateData(std::size_t bytes)
{
	assert(bytes % sizeof(uint64_t) == 0);
	std::vector<uint64_t> data(bytes / sizeof(uint64_t));
	std::iota(data.begin(), data.end(), 0);
	std::shuffle(data.begin(), data.end(), std::mt19937{ std::random_device{}() });
	return data;
}


FileIO::~FileIO()
{
	delete file;
}

fstream* FileIO::GetFileStream()
{
	return file;
}


/*
 * Bash, D. (2013). C++ reading a file in binary mode. Problems with END OF FILE [Forum Post]. Retrieved on February 26, from 
 *		https://stackoverflow.com/questions/16435180/c-reading-a-file-in-binary-mode-problems-with-end-of-file
 */
string FileIO::ReadBinaryFile(const string filepath)
{
	string fileContents = "";
	try
	{
		//open the file stream to the binary object, and check if it worked
		GetFileStream()->open(filepath, ios::in | ios::binary);
		if (GetFileStream()->is_open() == false)
		{
			throw filepath;
		}


		//Get the total length of the file, and reset the starting read position before reading in it's contents
		GetFileStream()->seekg(0, GetFileStream()->end);
		int length = GetFileStream()->tellg();
		GetFileStream()->seekg(0, GetFileStream()->beg);
		

		//Read the file into the buffer
		char * readBuffer = new char[length];
		GetFileStream()->read(readBuffer, length);


		//Close the stream and clean up before returning 
		GetFileStream()->close();
		fileContents.assign(readBuffer);
		delete[] readBuffer;
	}
	catch (...)
	{
		cout << "Error: Could not read file: " << filepath << endl;
	}

	return fileContents;
}


string FileIO::ReadAsciiFile(const string filepath)
{

	string fileContents = "";
	try
	{


		GetFileStream()->open(filepath, ios::in);
		if (GetFileStream()->is_open() == false) 
		{
			throw filepath;
		}
		while (!GetFileStream()->eof())
		{
			string readBuffer;
			*GetFileStream() >> readBuffer;
			fileContents += readBuffer;
		}


		GetFileStream()->close();
	}
	catch (...)
	{
		cout << "Error: Could not read file: " << filepath << endl;
	}

	return fileContents;
}

//https://stackoverflow.com/questions/11563963/writing-a-binary-file-in-c-very-fast
void FileIO::WriteBinaryFile(const string contents, const string filePath)
{
	std::size_t bytes = (std::size_t)contents.c_str();
	std::vector<uint64_t> data = GenerateData(bytes);

	std::ios_base::sync_with_stdio(false);
	auto myfile = std::fstream(filePath, std::ios::out | std::ios::binary);
	myfile.write((char*)&data[0], bytes);
	myfile.close();
}

void FileIO::WriteAsciiFile(const string contents, const string filePath)
{
	ofstream OpenedFile(filePath);
	try
	{
		OpenedFile << contents;
	}
	catch (...)
	{
		cout << "Error: Could not read file: " << filePath << endl;
	}

	OpenedFile.close();
}