#pragma once
class TextParser
{
public:

#pragma warning (disable : 26495)

	TextParser()
	{

	}

#pragma warning (default : 26495)

	~TextParser()
	{
		delete[] _fileName;
		delete[] _pTotalBuffer;
		delete[] _fileName_c;
		delete[] _pTotalBuffer_c;
	}

	BOOL LoadFile(const char* fileName);
	BOOL GetValue(const char* szName, int* ipValue);
	BOOL GetString(const char* szName, char* ipString);
	BOOL GetText(const char* szName, char* ipText);

	BOOL LoadFile(const WCHAR* fileName);
	BOOL GetValue(const WCHAR* szName, int* ipValue);
	BOOL GetString(const WCHAR* szName, WCHAR* ipString);
	BOOL GetText(const WCHAR* szName, WCHAR* ipText);
private:
	BOOL WSkipNoneCommand(void);
	BOOL WSkipNoneCommandS(void);

	BOOL SkipNoneCommand(void);
	BOOL SkipNoneCommandS(void);

	BOOL GetNextWord(char** chppBuffer, int* ipLength);
	BOOL GetStringWord(char** chppBuffer, int* ipLength);
	BOOL GetStringText(char* ipText, char* chpBuffer, int ipLength);

	BOOL GetNextWord(WCHAR** chppBuffer, int* ipLength);
	BOOL GetStringWord(WCHAR** chppBuffer, int* ipLength);
	BOOL GetStringText(WCHAR* ipText, WCHAR* chpBuffer, int ipLength);

	FILE* _file = nullptr;
	WCHAR* _fileName = nullptr;
	WCHAR* _chpBuffer = nullptr;
	WCHAR* _pTotalBuffer = nullptr;
	int _fileSize = 0;

	char* _fileName_c = nullptr;
	char* _chpBuffer_c = nullptr;
	char* _pTotalBuffer_c = nullptr;
	int _fileSize_c = 0;

};

