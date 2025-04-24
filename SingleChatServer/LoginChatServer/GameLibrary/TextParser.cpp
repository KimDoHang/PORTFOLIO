#include "pch.h"
#include "TextParser.h"
#pragma warning(disable:4267)
#pragma warning(disable:6387)
#pragma warning(disable:28020)


#define TAB 0x09
#define SPACE 0x20
#define BACKSPACE 0x08
#define LINEFEED 0x0a
#define CARRIGERETURN 0x0d
static const int readBufferSize = 256;

BOOL TextParser::LoadFile(const char* fileName)
{
    int size = strlen(fileName);

    if (_fileName)
    {
        delete[] _fileName;
        _fileName = nullptr;
    }
    if (_pTotalBuffer)
    {
        delete[] _pTotalBuffer;
        _pTotalBuffer = nullptr;
    }

    _fileName_c = new char[size + 1];
    strcpy_s(_fileName_c, size + 1, fileName);

    _fileName_c[size] = '\0';

    int ret = fopen_s(&_file, _fileName_c, "rt");

    if (ret != 0)
    {
        //file open fail
        return false;
    }

    fseek(_file, 0, SEEK_END);
    int fileSize = ftell(_file);

    if (fileSize == 0)
        return false;

    fseek(_file, 0, SEEK_SET);
    _pTotalBuffer_c = new char[fileSize + 1];

    if (fileSize >= 3)
    {
        char bomCodeBuffer[3];
        int readBomCode = fread_s(&bomCodeBuffer, sizeof(char) * 3, 1, 3, _file);
        if (strcmp(bomCodeBuffer, "EFBBBF"))
            fseek(_file, 0, SEEK_SET);
    }

    int readSize = fread_s(_pTotalBuffer_c, sizeof(CHAR) * (fileSize + 1), 1, sizeof(char) * fileSize, _file);

    if (readSize == 0)
    {
        //read file size 0
        return false;
    }

    _fileSize_c = readSize / sizeof(char);

    _pTotalBuffer_c[_fileSize_c] = '\0';
    _chpBuffer_c = _pTotalBuffer_c;

    fclose(_file);
    return true;
}

BOOL TextParser::LoadFile(const WCHAR* fileName)
{
    int size = wcslen(fileName);

    if (_fileName)
    {
        delete[] _fileName;
        _fileName = nullptr;
    }
    if (_pTotalBuffer)
    {
        delete[] _pTotalBuffer;
        _pTotalBuffer = nullptr;
    }

    _fileName = new WCHAR[size + 1];
    wcscpy_s(_fileName, size + 1, fileName);
    _fileName[size] = '\0';

    int ret = _wfopen_s(&_file, _fileName, L"rt");

    if (ret != 0)
    {
        //file open fail
        return false;
    }

    fseek(_file, 0, SEEK_END);
    int fileSize = ftell(_file);
    fseek(_file, 0, SEEK_SET);
    _pTotalBuffer = new WCHAR[fileSize + 1];

    if (fileSize >= 2)
    {
        char bomCodeBuffer[2];
        int readBomCode = fread_s(bomCodeBuffer, sizeof(char) * 2, 1, sizeof(char) * 2, _file);
        if (strcmp(bomCodeBuffer, "FFFE"))
            fseek(_file, 0, SEEK_SET);
    }


    int readSize = fread_s(_pTotalBuffer, sizeof(WCHAR) * (fileSize + 1), 1, sizeof(WCHAR) * fileSize, _file);

    if (readSize == 0)
    {
        //read file size 0
        return false;
    }

    _fileSize = readSize / sizeof(WCHAR);

    _pTotalBuffer[_fileSize] = '\0';
    _chpBuffer = _pTotalBuffer;

    fclose(_file);
    return true;
}

BOOL TextParser::GetValue(const char* szName, int* ipValue)
{
    char* chpBuff, readBuffer[256];
    int length;

    while (GetNextWord(&chpBuff, &length))
    {
        memset(readBuffer, 0, 256);
        memcpy(readBuffer, chpBuff, length);

        if (strcmp(szName, readBuffer) == 0)
        {
            if (GetNextWord(&chpBuff, &length))
            {
                if (strncmp("=", chpBuff, 1) == 0)
                {
                    if (GetNextWord(&chpBuff, &length))
                    {
                        memset(readBuffer, 0, 256);
                        memcpy(readBuffer, chpBuff, length);
                        *ipValue = atoi(readBuffer);
                        return TRUE;
                    }
                }
                return FALSE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

BOOL TextParser::GetString(const char* szName, char* ipString)
{
    char* chpBuff, readBuffer[256];
    int length;

    while (GetNextWord(&chpBuff, &length))
    {
        memset(readBuffer, 0, 256);
        memcpy(readBuffer, chpBuff, length);

        if (strcmp(szName, readBuffer) == 0)
        {
            if (GetNextWord(&chpBuff, &length))
            {
                if (strncmp("=", chpBuff, 1) == 0)
                {
                    if (GetStringWord(&chpBuff, &length))
                    {
                        memcpy(ipString, chpBuff, length);
                        ipString[length] = '\0';
                        return TRUE;
                    }
                }
                return FALSE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

BOOL TextParser::GetText(const char* szName, char* ipText)
{
    char* chpBuff, readBuffer[256];
    int length;

    while (GetNextWord(&chpBuff, &length))
    {
        memset(readBuffer, 0, 256);
        memcpy(readBuffer, chpBuff, length);

        if (strcmp(szName, readBuffer) == 0)
        {
            if (GetNextWord(&chpBuff, &length))
            {
                if (strncmp("=", chpBuff, 1) == 0)
                {
                    if (GetStringWord(&chpBuff, &length))
                    {
                        if (GetStringText(ipText, chpBuff, length))
                            return TRUE;
                        return FALSE;
                    }
                }
                return FALSE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

BOOL TextParser::GetValue(const WCHAR* szName, int* ipValue)
{
    WCHAR* chpBuff, readBuffer[256];
    int length;

    while (GetNextWord(&chpBuff, &length))
    {
        wmemset(readBuffer, 0, 256);
        wmemcpy(readBuffer, chpBuff, length);

        if (wcscmp(szName, readBuffer) == 0)
        {
            if (GetNextWord(&chpBuff, &length))
            {
                if (wcsncmp(L"=", chpBuff, 1) == 0)
                {
                    if (GetNextWord(&chpBuff, &length))
                    {
                        wmemset(readBuffer, 0, 256);
                        wmemcpy(readBuffer, chpBuff, length);
                        *ipValue = _wtoi(readBuffer);
                        return TRUE;
                    }
                }
                return FALSE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

BOOL TextParser::GetString(const WCHAR* szName, WCHAR* ipString)
{
    WCHAR* chpBuff, readBuffer[256];
    int length;

    while (GetNextWord(&chpBuff, &length))
    {
        wmemset(readBuffer, 0, 256);
        wmemcpy(readBuffer, chpBuff, length);

        if (wcscmp(szName, readBuffer) == 0)
        {
            if (GetNextWord(&chpBuff, &length))
            {
                if (wcsncmp(L"=", chpBuff, 1) == 0)
                {
                    if (GetStringWord(&chpBuff, &length))
                    {
                        wmemcpy(ipString, chpBuff, length);
                        ipString[length] = '\0';
                        return TRUE;
                    }
                }
                return FALSE;
            }
            return FALSE;
        }
    }
    return FALSE;
}

BOOL TextParser::GetText(const WCHAR* szName, WCHAR* ipText)
{
    WCHAR* chpBuff, readBuffer[256];
    int length;

    while (GetNextWord(&chpBuff, &length))
    {
        wmemset(readBuffer, 0, 256);
        wmemcpy(readBuffer, chpBuff, length);

        if (wcscmp(szName, readBuffer) == 0)
        {
            if (GetNextWord(&chpBuff, &length))
            {
                if (wcsncmp(L"=", chpBuff, 1) == 0)
                {
                    if (GetStringWord(&chpBuff, &length))
                    {
                        if (GetStringText(ipText, chpBuff, length))
                            return TRUE;
                        return FALSE;
                    }
                }
                return FALSE;
            }
            return FALSE;
        }
    }
    return FALSE;
}


BOOL TextParser::SkipNoneCommand(void)
{
    while (1)
    {
        if ((*_chpBuffer_c == '/' && *(_chpBuffer_c + 1) == '*'))
        {
            _chpBuffer_c += 2;
            while (!(*_chpBuffer_c == '*' && *(_chpBuffer_c + 1) == '/'))
            {
                if (*_chpBuffer_c == '\0')
                    return false;
                _chpBuffer_c++;
            }
            _chpBuffer_c += 2;
        }
        else if (*_chpBuffer_c == ',' || *_chpBuffer_c == '"' || *_chpBuffer_c == TAB || *_chpBuffer_c == CARRIGERETURN
            || *_chpBuffer_c == SPACE || *_chpBuffer_c == BACKSPACE || *_chpBuffer_c == LINEFEED)
        {
            _chpBuffer_c++;
        }
        else if (*_chpBuffer_c == '\0')
            return false;
        else
            break;
    }
    return true;
}

BOOL TextParser::SkipNoneCommandS(void)
{
    while (1)
    {
        if (*_chpBuffer_c == '"')
            break;
        else if (*_chpBuffer_c == '\0')
            return false;
        else
            _chpBuffer_c++;
    }

    _chpBuffer_c++;
    return true;
}

BOOL TextParser::WSkipNoneCommand(void)
{
    while (1)
    {
        if ((*_chpBuffer == '/' && *(_chpBuffer + 1) == '*'))
        {
            _chpBuffer += 2;
            while (!(*_chpBuffer == '*' && *(_chpBuffer + 1) == '/'))
            {
                if (*_chpBuffer == '\0')
                    return false;
                _chpBuffer++;
            }
            _chpBuffer += 2;
        }
        else if (*_chpBuffer == ',' || *_chpBuffer == '"' || *_chpBuffer == TAB || *_chpBuffer == CARRIGERETURN
            || *_chpBuffer == SPACE || *_chpBuffer == BACKSPACE || *_chpBuffer == LINEFEED)
        {
            _chpBuffer++;
        }
        else if (*_chpBuffer == '\0')
            return false;
        else
            break;
    }
    return true;
}

BOOL TextParser::WSkipNoneCommandS(void)
{
    while (1)
    {
        if (*_chpBuffer == '"')
            break;
        else if (*_chpBuffer == '\0')
            return false;
        else
            _chpBuffer++;
    }

    _chpBuffer++;
    return true;
}

BOOL TextParser::GetNextWord(char** chppBuffer, int* ipLength)
{
    if (!SkipNoneCommand())
        return false;
    *chppBuffer = _chpBuffer_c;
    *ipLength = 0;
    while (1)
    {
        //"¸é 
        if (*_chpBuffer_c == ',' || *_chpBuffer_c == '"' || *_chpBuffer_c == TAB || *_chpBuffer_c == CARRIGERETURN
            || *_chpBuffer_c == SPACE || *_chpBuffer_c == BACKSPACE || *_chpBuffer_c == LINEFEED || *_chpBuffer_c == '\0')
        {
            break;
        }
        else
        {
            _chpBuffer_c++;
            *ipLength += 1;
        }
    }
    return true;
}

BOOL TextParser::GetStringWord(char** chppBuffer, int* ipLength)
{
    if (!SkipNoneCommandS())
        return false;
    *chppBuffer = _chpBuffer_c;
    *ipLength = 0;
    while (1)
    {
        if (*_chpBuffer_c == '"')
        {
            break;
        }
        else
        {
            _chpBuffer_c++;
            *ipLength += 1;
        }
    }
    _chpBuffer_c++;
    return true;
}

BOOL TextParser::GetStringText(char* ipText, char* chpBuffer, int ipLength)
{
    int idx = 0;
    for (int i = 0; i < ipLength; i++)
    {
        if (*_chpBuffer_c == ',' || *_chpBuffer_c == '"' || *_chpBuffer_c == TAB || *_chpBuffer_c == CARRIGERETURN
            || *_chpBuffer_c == SPACE || *_chpBuffer_c == BACKSPACE || *_chpBuffer_c == LINEFEED)
            continue;
        ipText[idx++] = chpBuffer[i];
    }
    ipText[idx] = '\0';
    return true;
}

BOOL TextParser::GetNextWord(WCHAR** chppBuffer, int* ipLength)
{
    if (!WSkipNoneCommand())
        return false;
    *chppBuffer = _chpBuffer;
    *ipLength = 0;
    while (1)
    {
        //"¸é 
        if (*_chpBuffer == ',' || *_chpBuffer == '"' || *_chpBuffer == TAB || *_chpBuffer == CARRIGERETURN
            || *_chpBuffer == SPACE || *_chpBuffer == BACKSPACE || *_chpBuffer == LINEFEED || *_chpBuffer == '\0')
        {
            break;
        }
        else
        {
            _chpBuffer++;
            *ipLength += 1;
        }
    }
    return true;
}

BOOL TextParser::GetStringWord(WCHAR** chppBuffer, int* ipLength)
{
    if (!WSkipNoneCommandS())
        return false;
    *chppBuffer = _chpBuffer;
    *ipLength = 0;
    while (1)
    {
        if (*_chpBuffer == '"')
        {
            break;
        }
        else
        {
            _chpBuffer++;
            *ipLength += 1;
        }
    }
    _chpBuffer++;
    return true;
}

BOOL TextParser::GetStringText(WCHAR* ipText, WCHAR* chpBuffer, int ipLength)
{
    int idx = 0;
    for (int i = 0; i < ipLength; i++)
    {
        if (*_chpBuffer == ',' || *_chpBuffer == '"' || *_chpBuffer == TAB || *_chpBuffer == CARRIGERETURN
            || *_chpBuffer == SPACE || *_chpBuffer == BACKSPACE || *_chpBuffer == LINEFEED)
            continue;
        ipText[idx++] = chpBuffer[i];
    }
    ipText[idx] = '\0';
    return true;
}
