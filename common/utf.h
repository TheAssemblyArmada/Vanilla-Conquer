// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef COMMON_UTF_H
#define COMMON_UTF_H

#if defined _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wchar.h>

/**
 *  @brief Conversion class for interaction between UTF8 APIs and the WINAPI.
 * 
 *  This class is intended to be used as a wrapper for calling WINAPI unicode
 *  functions with UTF8 formatted strings to make them behave as if they took
 *  char* rather than wchar_t* while still allowing access to the full unicode
 *  character set.
 * 
 *  E.g Win32APIFuncW(UTF8To16(utf8_string));
 * 
 *  The class will act as a temporary here, doing the memory allocation and
 *  conversion for you on the fly with the destructor cleaning up after the
 *  function call when it goes out of scope.
 */
class UTF8To16
{
public:
    UTF8To16(const char* utf8)
        : m_buffer(nullptr)
    {
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
        m_buffer = new wchar_t[size];
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, m_buffer, size);
    }

    ~UTF8To16()
    {
        delete[] m_buffer;
        m_buffer = nullptr;
    }

    operator const wchar_t*() const
    {
        return m_buffer;
    }

private:
    wchar_t* m_buffer;
};

/**
 *  @brief Conversion class for interaction between UTF8 APIs and the WINAPI.
 * 
 *  This class is intended to be used as a wrapper for interpreting unicode
 *  strings provided by the WINAPI as UTF8 strings.
 * 
 *  E.g UTF8APIFunc(UTF16To8(utf16_string));
 * 
 *  The class will act as a temporary here, doing the memory allocation and
 *  conversion for you on the fly with the destructor cleaning up after the
 *  function call when it goes out of scope.
 */
class UTF16To8
{
public:
    UTF16To8(const wchar_t* utf16)
        : m_buffer(nullptr)
    {
        int size = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
        m_buffer = new char[size];
        WideCharToMultiByte(CP_UTF8, 0, utf16, -1, m_buffer, size, nullptr, nullptr);
    }

    ~UTF16To8()
    {
        delete[] m_buffer;
        m_buffer = nullptr;
    }

    operator const char*() const
    {
        return m_buffer;
    }

private:
    char* m_buffer;
};

#ifdef _UNICODE
#define TCHARToUTF8(str) UTF16To8(str)
#define UTF8ToTCHAR(str) UTF8To16(str)
#else
#define TCHARToUTF8(str) (str)
#define UTF8ToTCHAR(str) (str)
#endif

#endif // _WIN32

#endif // COMMON_UTF_H
