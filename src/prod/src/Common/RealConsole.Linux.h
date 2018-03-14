// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Throw.h"
#include "Common/Formatter.h"
#include "Common/TextWriter.h"
#include "Common/RwLock.h"
#include "Common/StringUtility.h"
#include "Common/LogLevel.h"

#include <unistd.h>
#define NULL 0

using namespace std;

namespace Common
{
    namespace Color
    {
        enum Enum
        {
            fgBlue    = FOREGROUND_BLUE,
            fgGreen   = FOREGROUND_GREEN,
            fgRed     = FOREGROUND_RED,
            fgYellow  = FOREGROUND_RED | FOREGROUND_GREEN,
            fgIntense = FOREGROUND_INTENSITY,

            bgBlue    = BACKGROUND_BLUE,
            bgGreen   = BACKGROUND_GREEN,
            bgRed     = BACKGROUND_RED,
            bgYellow  = BACKGROUND_RED | BACKGROUND_GREEN,
            bgIntense = BACKGROUND_INTENSITY,
        };
    };

    class RealConsole : public TextWriter
    {
        ::HANDLE inHandle_;
        ::HANDLE outHandle_;
        BOOL consoleOutput_;
        DWORD currentColor_;

    public:
        RealConsole();

        void SetColor(DWORD color);

        DWORD Color() const
        {
            return currentColor_;
        }

        int Read() const;
        bool ReadLine( std::wstring& buffer ) const;
        void WriteAsciiBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen);
        void WriteUnicodeBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen);

        void Flush()
        {
        }
    };
}
