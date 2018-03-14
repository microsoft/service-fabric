// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Formatter.h"

namespace Common
{

    //
    // Definitions:
    //
    // Coding error - something horrible has happened due to a coding error and the system will crash.  In unit test environments, will fail the test by throwing an exception but will
    // not crash (right now - eventually may crash in unit test environments too).  Calling fail_coding_error with a literal is the safest method of terminating the program.  Other methods
    // will result in (at least) copying the message string to a new buffer.
    //
    // System error - something exceptional has happened, but the exception will be caught at a higher level.
    //
    __declspec(noreturn) void fail_coding_error(char const * message);
    __declspec(noreturn) void fail_coding_error(std::string const & message);
    __declspec(noreturn) void fail_coding_error(std::wstring const & message);

    __declspec(noreturn) void throw_system_error(char const * message);
    __declspec(noreturn) void throw_system_error(std::string const & message, std::error_code error);
    __declspec(noreturn) void throw_system_error(std::wstring const & message, std::error_code error);

    __declspec(nothrow) void log_system_error(char const *message, std::error_code error);

    template <class Handle> Handle CheckHandle(Handle badVal, Handle val,  char const * message)
    {
        if ( val == badVal )
        {
            Common::Assert::CodingError("{0} : Last error {1}", message, microsoft::GetLastErrorCode());
        }

        return val;
    }

    template<class T> bool is_in_range(T expr, T a, T b)
    {
        return (a <= expr) && (expr <= b);
    }

#   define FAIL_CODING_ERROR(str) Common::fail_coding_error((std::string)__FUNCTION__ + ":" COMMON_STRINGIFY(__LINE__) ": " str)
#   define THROW_SYSTEM_ERROR(error, str) Common::throw_system_error((std::string)__FUNCTION__  + ":" COMMON_STRINGIFY(__LINE__) ": " str, error)
#   define THROW_WINDOWS_ERROR(error, str) Common::throw_system_error((std::string)__FUNCTION__ + ":" COMMON_STRINGIFY(__LINE__) ": " str, microsoft::MakeWindowsErrorCode(error))
#   define LOG_SYSTEM_ERROR(error, str) Common::log_system_error((std::string)__FUNCTION__ + ":" COMMON_STRINGIFY(__LINE__) ": " str, error)

#   define FAIL_CODING_ERROR_FMT(str , ...)                                         \
    {                                                                               \
        std::wstring finalMessage;                                                  \
        Common::StringWriter sw(finalMessage);                                         \
        sw.Write(str, __VA_ARGS__);                                                 \
        sw.Write(" {0}:{1}:{2}" , __FILE__, __FUNCTION__, COMMON_STRINGIFY(__LINE__ ));\
        Common::fail_coding_error(finalMessage); \
    }

#   define THROW_SYSTEM_ERROR_FMT(ec, str , ...)                              \
    {                                                                               \
        std::wstring finalMessage;                                                  \
        Common::StringWriter sw(finalMessage);                                         \
        sw.Write(str, __VA_ARGS__);                                                 \
        sw.Write(" {0}:{1}:{2}" , __FILE__, __FUNCTION__, COMMON_STRINGIFY(__LINE__)); \
        Common::throw_system_error(finalMessage, ec);                                  \
    }

#   define coding_error_if_fmt(cond, str, ...) if (!(cond)); else FAIL_CODING_ERROR_FMT(str, __VA_ARGS__)
#   define coding_error_if_not_fmt(cond, str, ...) if (cond); else FAIL_CODING_ERROR_FMT(str, __VA_ARGS__)
#   define system_error_if_fmt(cond, ec, str, ...) if (!(cond)); else THROW_SYSTEM_ERROR_FMT(ec, str,__VA_ARGS__)
#   define system_error_if_not_fmt(cond, ec, str, ...) if (cond); else THROW_SYSTEM_ERROR_FMT(ec, str, __VA_ARGS__)

#   define coding_error_if(cond, str, ...) if (!(cond)); else FAIL_CODING_ERROR(str)
#   define coding_error_if_not(cond, str, ...) if (cond); else FAIL_CODING_ERROR(str)
#   define system_error_if(cond, ec, str, ...) if ( !( cond)); else THROW_SYSTEM_ERROR(ec, str)
#   define system_error_if_not(cond, ec, str, ...) if (( cond)); else THROW_SYSTEM_ERROR(ec, str)

#   define coding_error_crash(str) FAIL_CODING_ERROR( str )
#   define coding_error(str) FAIL_CODING_ERROR( str )
#   define coding_error_assert(expr) coding_error_if_not(expr, COMMON_STRINGIFY( expr ) " is false")
#   define coding_error_if_null(expr) coding_error_if_not(expr, COMMON_STRINGIFY( expr ) " is null")


#   if DBG
#       define debug_assert( expr ) coding_error_assert( expr )
#   else
#       define debug_assert( expr )
#   endif

#   define CHK_PARAMETER( param, expr )    if ( expr ) ; else THROW_WINDOWS_ERROR( ERROR_INVALID_PARAMETER, #param )
#   define CHK_PARAMETER_NOT_NULL( param ) if (( param ) != nullptr ) ; else THROW_WINDOWS_ERROR( ERROR_INVALID_PARAMETER, #param )

#   define CHK_HANDLE_NOT_ZERO( expr ) Common::CheckHandle( ::HANDLE( 0 ), expr, #expr )
#   define CHK_HMODULE( expr ) Common::CheckHandle( ::HMODULE( 0 ), expr, #expr )

#   define CHK_HANDLE_VALID(__expr) Common::CheckHandle(INVALID_HANDLE_VALUE, __expr, #__expr)
#   define CHK_SOCKET_HANDLE(__expr) Common::CheckHandle(INVALID_SOCKET,__expr, #__expr)

#   define CHK_W32( expr ) for ( DWORD __status = expr; __status; ) THROW_SYSTEM_ERROR( microsoft::MakeWindowsErrorCode( __status ), COMMON_STRINGIFY( expr ))
#   define CHK_WBOOL( expr ) if ( expr ) ; else THROW_SYSTEM_ERROR( microsoft::GetLastErrorCode(), COMMON_STRINGIFY( expr ))
#   define CHK_WSA( expr ) if ( 0 == expr ) ; else THROW_SYSTEM_ERROR( microsoft::GetLastErrorCode(), COMMON_STRINGIFY( expr ))
#   define CHK_HR( expr ) for ( int __hr = expr; FAILED(__hr); ) THROW_SYSTEM_ERROR( microsoft::MakeWindowsErrorCode( __hr ), #expr )   //TODO: Create its own error category?

#   define LOG_WSA(expr) if (0 == expr) ; else LOG_SYSTEM_ERROR(microsoft::GetLastErrorCode(), COMMON_STRINGIFY(expr))

    // TODO: remove and replace with better one
#   define THROW(ec, str) THROW_SYSTEM_ERROR(ec, str)
#   define THROW_ARG(ec, str, arg) THROW_SYSTEM_ERROR(ec, str)
#   define WinError(expr) microsoft::MakeWindowsErrorCode(expr)

#   define argument_in_range(expr, a, b) coding_error_if_not(is_in_range( expr, a, b), \
        COMMON_STRINGIFY(expr) " is out of range [" COMMON_STRINGIFY(a) " .. " COMMON_STRINGIFY(b) "]");

#define THROW_LAST_ERROR(str) \
    THROW_WINDOWS_ERROR(microsoft::GetLastErrorCode().value(), str)

#define THROW_IF(__condition, __err, __reason) \
    if (!(__condition)) ; else THROW(__err,   __reason  )

#define THROW_IF_FMT(__condition, __err, __str, ...)           \
    if (__condition) {                                  \
        THROW_SYSTEM_ERROR_FMT(__err,  __str, __VA_ARGS__);       \
    }

#define THROW_FMT( __err, __formatString, ... )        \
    {                                                  \
        THROW_SYSTEM_ERROR_FMT(__err,  __formatString, __VA_ARGS__);  \
    }


//#define THROW_FMT1( __err, __formatString, __arg1 ) THROW_FMT(__err, __formatString, __arg1)
//#define THROW_FMT2( __err, __formatString, __arg1, __arg2 ) THROW_FMT(__err, __formatString, __arg1, __arg2)
//#define THROW_FMT3( __err, __formatString, __arg1, __arg2, __arg3 ) THROW_FMT(__err, __formatString, __arg1, __arg2, __arg3)
//#define THROW_FMT4( __err, __formatString, __arg1, __arg2, __arg3, __arg4 ) THROW_FMT(__err, __formatString, __arg1, __arg2, __arg3, __arg4)
//#define THROW_FMT5( __err, __formatString, __arg1, __arg2, __arg3, __arg4, __arg5 ) THROW_FMT(__err, __formatString, __arg1, __arg2, __arg3, __arg4, __arg5)

}

