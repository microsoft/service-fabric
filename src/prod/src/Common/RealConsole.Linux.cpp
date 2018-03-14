// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

typedef map<DWORD, string> ColorMap;

static ColorMap* CreateColorMap()
{
    ColorMap* result = new ColorMap();
    result->insert(make_pair(Color::fgBlue,    "34"));
    result->insert(make_pair(Color::fgYellow,  "33"));
    result->insert(make_pair(Color::fgGreen,   "32"));
    result->insert(make_pair(Color::fgRed,     "31"));
    result->insert(make_pair(Color::fgIntense, "1"));
    result->insert(make_pair(Color::bgBlue,    "44"));
    result->insert(make_pair(Color::bgYellow,  "43"));
    result->insert(make_pair(Color::bgGreen,   "42"));
    result->insert(make_pair(Color::bgRed,     "41"));
    result->insert(make_pair(Color::bgIntense, "1"));
    
    return result;
}

static Global<ColorMap> colorMap(CreateColorMap());
static const GlobalString escSequence = make_global<string>("\033[");

RealConsole::RealConsole() :
    inHandle_( GetStdHandle( STD_INPUT_HANDLE )),
    outHandle_( GetStdHandle( STD_OUTPUT_HANDLE ))
{
    DWORD consoleMode;
    consoleOutput_ = (0 != ttyname(STDERR_FILENO));
}

void RealConsole::SetColor(DWORD color)
{
    currentColor_ = color;
    string seq = *escSequence;

    // intense
    if (color & Color::fgIntense || color & Color::bgIntense)
    {
        seq += (*colorMap)[Color::fgIntense];
    }
    
    // color
    color = color & (~Color::fgIntense) & (~Color::bgIntense);
    seq += ";" + (*colorMap)[color] + "m";
    WriteAsciiBuffer(seq.c_str(), seq.length());
}

int RealConsole::Read() const
{
    char ch;
    DWORD count;

    CHK_WBOOL( ReadFile(inHandle_, &ch, 1, &count, nullptr));
    return count == 0 ? -1 : ch;
}

bool RealConsole::ReadLine(wstring& buffer) const
{
    string ansiBuffer;
    while (true)
    {
        char ch;
        DWORD count;

        CHK_WBOOL(ReadFile(inHandle_, &ch, 1, &count, nullptr));
        if (count == 0 || ch == '\n')
        {
            break;
        }
        
        ansiBuffer += ch;                
    }
    
    StringUtility::Utf8ToUtf16(ansiBuffer, buffer);
    return buffer.size() > 0;
}

#pragma prefast(push) // Begin disable Warning 38020 - ANSI API ('WriteConsoleA') should not be called from Unicode modules    
#pragma prefast(disable:38020, "Explicitly called by WriteLine and Write formatters")
void RealConsole::WriteAsciiBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen)
{
    DWORD unused = 0;
    WriteFile(outHandle_, buf, static_cast<int>(ccLen), &unused, nullptr);
}
#pragma prefast(pop) // end disable Warning 38020 - ANSI API ('WriteConsoleA') should not be called from Unicode modules

void RealConsole::WriteUnicodeBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen)
{
    DWORD unused = 0;
    string ansiVersion;
    StringUtility::UnicodeToAnsi(buf, ansiVersion);
    WriteFile(outHandle_, ansiVersion.c_str(), static_cast<DWORD>(ansiVersion.length()), &unused, nullptr);
}

