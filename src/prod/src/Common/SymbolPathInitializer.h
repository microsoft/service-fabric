//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#include <dbghelp.h>

namespace Common
{
    class SymbolPathInitializer
    {
    public:
        std::string InitializeSymbolPaths()
        {
            std::string symPathOut;

#ifndef PLATFORM_UNIX
            wstring symPath;
            Environment::GetEnvironmentVariable(L"_NT_SYMBOL_PATH", symPath, NOTHROW());
            wstring symPathAlt;
            Environment::GetEnvironmentVariable(L"_NT_ALTERNATE_SYMBOL_PATH", symPathAlt, NOTHROW());
            auto symPathsW = wformatString("{0};{1};{2};{3}", Environment::GetExecutablePath(), Directory::GetCurrentDirectory(), symPath, symPathAlt);
            StringUtility::Utf16ToUtf8(symPathsW, symPathOut);

            SymInitialize(GetCurrentProcess(), symPathOut.c_str(), TRUE);
#endif

            return symPathOut;
        }
    };
    
}