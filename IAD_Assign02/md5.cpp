#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>
#include <string>

#pragma warning(disable: 4996)

#define BUFSIZE 1024
#define MD5LEN  16

//prototype
char* GetMd5Value(LPCSTR filename);

//int main(void)
//{
//	LPCSTR filename = "filename.txt";
//
//	// File 1
//	char test1Hash[33];
//	char* test1 = GetMd5Value(filename);
//	// Copy over to our main char*
//	strcpy(test1Hash, test1);
//	// Copy to a string
//	std::string test1String = std::string(test1Hash);
//
//	// File 2
//	char test2Hash[33];
//	char* test2 = GetMd5Value(filename);
//	// Copy over to our main char*
//	strcpy(test2Hash, test2);
//	// Copy to a string
//	std::string test2String = std::string(test2Hash);
//
//
//	// Error Checking
//	if ((test1String.length() < 32) || (test2String.length() < 32))
//	{
//		printf("ERROR");
//	}
//
//	// Compare results
//	if (strcmp(test1, test2) == 0)
//	{
//		printf("Matched Successfully!");
//	}
//	else
//	{
//		printf("Matched Failed...");
//	}
//	printf("%s\n", test1String);
//	printf("%s\n", test2String);
//
//	getchar();
//	return 0;
//}

// Function:	GetMd5Value()
// Description: Gathered from MSDN and altered for our assignment requirements.
//				https://docs.microsoft.com/en-gb/windows/desktop/SecCrypto/example-c-program--creating-an-md-5-hash-from-file-content
// Parameters:	LPCSTR filename	-	The file to get a hashvalue for
// Returns:		char* -		The hash value for the file.
char* GetMd5Value(LPCSTR filename)
{
	char md5Value[MD5LEN + MD5LEN + 1];
	memset(md5Value, 0, sizeof(md5Value));
	DWORD dwStatus = 0;
	BOOL bResult = FALSE;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	HANDLE hFile ;
	BYTE rgbFile[BUFSIZE];
	DWORD cbRead = 0;
	BYTE rgbHash[MD5LEN];
	DWORD cbHash = 0;
	CHAR rgbDigits[] = "0123456789abcdef";
	// Logic to check usage goes here.

	hFile = CreateFile(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		dwStatus = GetLastError();
		printf("Error opening file %s\nError: %d\n", filename,
			dwStatus);
		memcpy(md5Value, &dwStatus, sizeof(dwStatus));
		return md5Value;
	}

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv,
		NULL,
		NULL,
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT))
	{
		dwStatus = GetLastError();
		printf("CryptAcquireContext failed: %d\n", dwStatus);
		CloseHandle(hFile);
		md5Value[0] = dwStatus;
		md5Value[1] = '\0';
		memcpy(md5Value, &dwStatus, sizeof(dwStatus));
		return md5Value;
	}

	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	{
		dwStatus = GetLastError();
		printf("CryptAcquireContext failed: %d\n", dwStatus);
		CloseHandle(hFile);
		CryptReleaseContext(hProv, 0);
		md5Value[0] = dwStatus;
		md5Value[1] = '\0';
		memcpy(md5Value, &dwStatus, sizeof(dwStatus));
		return md5Value;
	}

	while (bResult = ReadFile(hFile, rgbFile, BUFSIZE,
		&cbRead, NULL))
	{
		if (0 == cbRead)
		{
			break;
		}

		if (!CryptHashData(hHash, rgbFile, cbRead, 0))
		{
			dwStatus = GetLastError();
			printf("CryptHashData failed: %d\n", dwStatus);
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hFile);
			md5Value[0] = dwStatus;
			md5Value[1] = '\0';
			memcpy(md5Value, &dwStatus, sizeof(dwStatus));
			return md5Value;
		}
	}

	if (!bResult)
	{
		dwStatus = GetLastError();
		printf("ReadFile failed: %d\n", dwStatus);
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hFile);
		md5Value[0] = dwStatus;
		md5Value[1] = '\0';
		memcpy(md5Value, &dwStatus, sizeof(dwStatus));
		return md5Value;
	}

	int j = 0;
	cbHash = MD5LEN;
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
	{
		printf("MD5 hash of file %s is: ", filename);
		printf("\n");
		for (DWORD i = 0; i < cbHash; i++)
		{
			printf("%c%c", rgbDigits[rgbHash[i] >> 4],
				rgbDigits[rgbHash[i] & 0xf]);
			md5Value[j++] = rgbDigits[rgbHash[i] >> 4];
			md5Value[j++] = rgbDigits[rgbHash[i] & 0xf];
		}
		printf("\n");
		md5Value[j] = '\0';
	}
	else
	{
		dwStatus = GetLastError();
		printf("CryptGetHashParam failed: %d\n", dwStatus);
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hFile);

	return md5Value;
}