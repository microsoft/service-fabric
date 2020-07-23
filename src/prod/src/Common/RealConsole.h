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

namespace Common
{
    namespace Color
    {
        enum Enum
        {
            fgBlue    = FOREGROUND_BLUE,
            fgGreen   = FOREGROUND_GREEN,
            fgRed     = FOREGROUND_RED,
            fgIntense = FOREGROUND_INTENSITY,

            bgBlue    = BACKGROUND_BLUE,
            bgGreen   = BACKGROUND_GREEN,
            bgRed     = BACKGROUND_RED,
            bgIntense = BACKGROUND_INTENSITY,
        };
    };

    class RealConsole : public TextWriter
    {
        ::HANDLE inHandle_;
        ::HANDLE outHandle_;
        BOOL consoleOutput_;

    public:
        RealConsole() :
            inHandle_( GetStdHandle( STD_INPUT_HANDLE )),
            outHandle_( GetStdHandle( STD_OUTPUT_HANDLE ))
        {
            DWORD consoleMode;
            consoleOutput_ = GetConsoleMode(outHandle_, &consoleMode);
        }

        void SetColor( DWORD color )
        {
            ::SetConsoleTextAttribute( outHandle_, ::WORD( color )) ;
        }

        ::DWORD Color() const
        {
            ::CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo( outHandle_, &info );
            return info.wAttributes;
        }

        int Read() const
        {
            wchar_t ch;
            ::DWORD count;

            CHK_WBOOL( ::ReadConsoleW( inHandle_, &ch, 1, &count, nullptr ));
            return count == 0 ? -1 : ch;
        }

        bool ReadLine( std::wstring& buffer ) const
        {
            ::DWORD count;
            buffer.resize( 256 );
            CHK_WBOOL(
                ::ReadConsoleW( inHandle_, &buffer[0], ( int )buffer.size(), &count, nullptr ));
            buffer.resize( count );
            return count > 0;
        }

#pragma prefast(push) // Begin disable Warning 38020 - ANSI API ('WriteConsoleA') should not be called from Unicode modules    
#pragma prefast(disable:38020, "Explicitly called by WriteLine and Write formatters")
        void WriteAsciiBuffer( __in_ecount( ccLen ) char const * buf, ::size_t ccLen )
        {
            DWORD unused = 0;
            if(consoleOutput_)
            {
                ::WriteConsoleA( outHandle_, buf, static_cast< int >( ccLen ), &unused, nullptr );
            }
            else
            {
                WriteFile( outHandle_, buf, static_cast< int >( ccLen ), &unused, nullptr );
            }
        }
#pragma prefast(pop) // end disable Warning 38020 - ANSI API ('WriteConsoleA') should not be called from Unicode modules

        void WriteUnicodeBuffer( __in_ecount( ccLen ) wchar_t const * buf, ::size_t ccLen )
        {
            DWORD unused = 0;
            if(consoleOutput_)
            {
                ::WriteConsoleW( outHandle_, buf, static_cast< int >( ccLen ), &unused, nullptr );
            }
            else
            {
                std::string ansiVersion;
                StringUtility::UnicodeToAnsi( buf, ansiVersion );
                WriteFile( outHandle_, ansiVersion.c_str(), static_cast< DWORD >( ansiVersion.length() ), &unused, nullptr );
            }
        }

        void Flush()
        {
        }
    };
}
