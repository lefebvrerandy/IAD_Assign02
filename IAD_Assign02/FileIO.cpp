



#include "FileIO.h"


fstream* FileIO::file = new fstream();

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

