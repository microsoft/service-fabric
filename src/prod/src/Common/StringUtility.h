// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <cwctype>
#include <string>
#include <objbase.h>

#include "Common/TextWriter.h"

namespace Common
{

//
// Use TRY_COM_PARSE_PUBLIC_* macros to parse public strings from COM wrapper classes
//

#define TRY_COM_PARSE_PUBLIC_STRING_ALLOW_NULL( InputPtr ) TRY_COM_PARSE_PUBLIC_STRING_OUT( InputPtr, parsed_##InputPtr, true )

#define TRY_COM_PARSE_PUBLIC_STRING_ALLOW_NULL2( InputPtr ) TRY_COM_PARSE_PUBLIC_STRING_OUT2( InputPtr, parsed_##InputPtr, true )

#define TRY_COM_PARSE_PUBLIC_STRING( InputPtr ) TRY_COM_PARSE_PUBLIC_STRING_OUT( InputPtr, parsed_##InputPtr, false )

#define TRY_COM_PARSE_PUBLIC_STRING2( InputPtr ) TRY_COM_PARSE_PUBLIC_STRING_OUT2( InputPtr, parsed_##InputPtr, false )

#define TRY_COM_PARSE_PUBLIC_STRING_OUT( InputPtr, OutputVar, AllowNull ) \
    std::wstring OutputVar; \
    { \
        auto parseHResult = StringUtility::LpcwstrToWstring( InputPtr, AllowNull, OutputVar ); \
        ON_PUBLIC_COM_PARSE_RESULT( InputPtr, OutputVar, parseHResult ) \
    } \

#define TRY_COM_PARSE_PUBLIC_STRING_OUT2( InputPtr, OutputVar, AllowNull ) \
    std::wstring OutputVar; \
    { \
        auto parseError = StringUtility::LpcwstrToWstring2( InputPtr, AllowNull, OutputVar ); \
        ON_PUBLIC_COM_PARSE_RESULT2( InputPtr, OutputVar, parseError ) \
    } \

#define TRY_COM_PARSE_PUBLIC_LONG_STRING( InputPtr ) \
    std::wstring parsed_##InputPtr; \
    { \
        auto parseHResult = StringUtility::LpcwstrToWstring( InputPtr, false, Common::ParameterValidator::MinStringSize, CommonConfig::GetConfig().MaxLongStringSize, parsed_##InputPtr ); \
        ON_PUBLIC_COM_PARSE_RESULT( InputPtr, parsed_##InputPtr, parseHResult ) \
    } \

#define TRY_COM_PARSE_PUBLIC_FILEPATH( InputPtr ) TRY_COM_PARSE_PUBLIC_FILEPATH_OUT( InputPtr, parsed_##InputPtr )

#define TRY_COM_PARSE_PUBLIC_FILEPATH_OUT( InputPtr, OutputVar ) \
    std::wstring OutputVar; \
    { \
        auto parseHResult = StringUtility::LpcwstrToFilePath( InputPtr, OutputVar ); \
        ON_PUBLIC_COM_PARSE_RESULT( InputPtr, OutputVar, parseHResult ) \
    } \

//
// Use TRY_PARSE_PUBLIC_* macros to parse public strings from internal native classes.
// Typically in FromPublicApi() member functions.
//

#define TRY_PARSE_PUBLIC_STRING_ALLOW_NULL( InputPtr, OutputVar ) TRY_PARSE_PUBLIC_STRING_OUT( InputPtr, OutputVar, true )

#define TRY_PARSE_PUBLIC_STRING_ALLOW_NULL2( InputPtr, OutputVar ) \
    wstring OutputVar; \
    TRY_PARSE_PUBLIC_STRING_OUT( InputPtr, OutputVar, true ) \

#define TRY_PARSE_PUBLIC_STRING( InputPtr, OutputVar ) TRY_PARSE_PUBLIC_STRING_OUT( InputPtr, OutputVar, false )

#define TRY_PARSE_PUBLIC_STRING2( InputPtr, OutputVar ) \
    wstring OutputVar; \
    TRY_PARSE_PUBLIC_STRING_OUT( InputPtr, OutputVar, false ) \

#define TRY_PARSE_PUBLIC_STRING_OUT( InputPtr, OutputVar, AllowNull ) \
    { \
        auto parseError = StringUtility::LpcwstrToWstring2( InputPtr, AllowNull, OutputVar ); \
        ON_PUBLIC_PARSE_RESULT( InputPtr, OutputVar, parseError ) \
    } \

#define TRY_PARSE_PUBLIC_STRING_PER_SIZE_NOT_NULL( InputPtr, OutputVar, MaxSize ) \
    TRY_PARSE_PUBLIC_STRING_PER_SIZE( InputPtr, OutputVar, false, ParameterValidator::MinStringSize, MaxSize)

#define TRY_PARSE_PUBLIC_STRING_PER_SIZE( InputPtr, OutputVar, AllowNull, MinSize, MaxSize ) \
     { \
        auto parseError = StringUtility::LpcwstrToWstring2( InputPtr, AllowNull, MinSize, MaxSize, OutputVar ); \
        ON_PUBLIC_PARSE_RESULT( InputPtr, OutputVar, parseError ) \
    } \

//
// Helper macros
//

#define ON_PUBLIC_COM_PARSE_RESULT( InputPtr, OutputVar, HResult ) \
    if (FAILED(HResult)) \
    { \
        TRACE_PARSE_FAILURE( InputPtr, OutputVar, HResult ) \
        return Common::ComUtility::OnPublicApiReturn(HResult); \
    } \

#define ON_PUBLIC_COM_PARSE_RESULT2( InputPtr, OutputVar, Error ) \
    if (!Error.IsSuccess()) \
    { \
        TRACE_PARSE_FAILURE( InputPtr, OutputVar, Error ) \
        return Common::ComUtility::OnPublicApiReturn(std::move(Error)); \
    } \

#define ON_PUBLIC_PARSE_RESULT( InputPtr, OutputVar, Error ) \
    if (!Error.IsSuccess()) \
    { \
        TRACE_PARSE_FAILURE( InputPtr, OutputVar, wformatString("{0} details='{1}'", Error, Error.Message) ) \
        return Error; \
    } \

#define TRACE_PARSE_FAILURE( InputPtr, OutputVar, Error ) \
    Trace.WriteWarning("StringUtility", "Function '{0}': Parse({1}, __out {2}) failed: {3}", __FUNCTION__, #InputPtr, #OutputVar, Error); \

    struct StringUtility
    {
    public:
        // -----------------------------------------------------
        // Case insensitive string comparison
        // -----------------------------------------------------
        inline static bool AreEqualCaseInsensitive(std::wstring const & a, std::wstring const & b)
        {
            return _wcsicmp(a.c_str(), b.c_str()) == 0;
        }

        inline static bool AreEqualCaseInsensitive(const wchar_t * const a, const wchar_t * const b)
        {
            return _wcsicmp(a, b) == 0;
        }

        inline static bool AreEqualCaseInsensitive(std::string const & a, std::string const & b)
        {
            return _stricmp(a.c_str(), b.c_str()) == 0;
        }

        inline static bool AreEqualCaseInsensitive(const char * const a, const char * const b)
        {
            return _stricmp(a, b) == 0;
        }

        // -----------------------------------------------------
        // Case insensitive string comparison for the prefix part and case sensitive for suffix part
        // -----------------------------------------------------
        template <typename StringType, typename CharType>
        static bool AreEqualPrefixPartCaseInsensitive(StringType const & a, StringType const & b, CharType const c)
        {
            StringType prefix1;
            StringType prefix2;
            StringType suffix1;
            StringType suffix2;
            SplitOnce(a, prefix1, suffix1, c);
            SplitOnce(b, prefix2, suffix2, c);
            return AreEqualCaseInsensitive(prefix1, prefix2) && suffix1 == suffix2;
        }

        inline static int CompareCaseInsensitive(std::wstring const & a, std::wstring const & b)
        {
            return _wcsicmp(a.c_str(), b.c_str());
        }

        inline static int CompareCaseInsensitive(std::string const & a, std::string const & b)
        {
            return _stricmp(a.c_str(), b.c_str());
        }

        inline static int CompareCaseInsensitive(const wchar_t * const a, const wchar_t * const b)
        {
            return _wcsicmp(a, b);
        }

        inline static int Compare(std::wstring const & a, std::wstring const & b)
        {
            return wcscmp(a.c_str(), b.c_str());
        }

        inline static int Compare(std::string const & a, std::string const & b)
        {
            return strcmp(a.c_str(), b.c_str());
        }

        inline static int Compare(const wchar_t * const a, const wchar_t * const b)
        {
            return wcscmp(a, b);
        }

        inline static int CompareDigitsAsNumbers(const wchar_t * a, const wchar_t * b)
        {
            if (!a || !b)
            {
                return !b - !a;
            }

            while (*a && *b)
            {
                if (!a || !b)
                {
                    return !b - !a;
                }

                while (*a && *b)
                {
                    if (std::iswdigit(*a) && std::iswdigit(*b))
                    {
                        const wchar_t * a1 = a;
                        const wchar_t * b1 = b;
                        for (; *a && std::iswdigit(*a); a1 = ((*a == '0' && a == a1) ? a1 + 1 : a1), a++);
                        for (; *b && std::iswdigit(*b); b1 = ((*b == '0' && b == b1) ? b1 + 1 : b1), b++);
                        if (a - a1 != b - b1)
                        {
                            return signum((a - a1), (b - b1));
                        }

                        auto cnt = (a - a1);
                        for (; (a1 != a) && (*a1 == *b1) && cnt; a1++, b1++, cnt--);
                        if (cnt != 0)
                        {
                            return signum(*a1, *b1);
                        }
                    }
                    else if (*a != *b)
                    {
                        return signum(*a, *b);
                    }
                    else
                    {
                        a++; b++;
                    }
                }

                return signum(*a, *b);
            }

            return signum(*a, *b);
        }

        inline static bool ContainsNewline(std::string const & a)
        {
            return a.find("\r", 0) != std::string::npos || a.find("\n", 0) != std::string::npos;
        }

        inline static bool ContainsNewline(std::wstring const & a)
        {
            return a.find(L"\r", 0) != std::string::npos || a.find(L"\n", 0) != std::string::npos;
        }

        inline static bool IsLessCaseInsensitive(std::wstring const & a, std::wstring const & b)
        {
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
        }

        inline static bool IsLessCaseSensitive(std::wstring const & a, std::wstring const & b)
        {
            return wcscmp(a.c_str(), b.c_str()) < 0;
        }

        inline static bool IsLessCaseInsensitive(std::string const & a, std::string const & b)
        {
            return _stricmp(a.c_str(), b.c_str())<0;
        }

        // -----------------------------------------------------
        // ToUpper / ToLower functions
        // -----------------------------------------------------
        inline static char ToUpperChar(char c)
        {
            return static_cast<char>(toupper(c));
        }

        inline static wchar_t ToUpperCharW(wchar_t c)
        {
            return static_cast<wchar_t>(towupper(c));
        }

        inline static char ToLowerChar(char c)
        {
            return static_cast<char>(tolower(c));
        }

        inline static wchar_t ToLowerCharW(wchar_t c)
        {
            return static_cast<wchar_t>(towlower(c));
        }

        static void ToUpper(std::string & str);

        static void ToUpper(std::wstring & str);

        static void ToLower(std::string & str);

        static void ToLower(std::wstring & str);

        static void GenerateRandomString(int length, std::wstring & randomString);

        static size_t GetHash(std::wstring const & str);

        // -----------------------------------------------------
        // Contains (case sensitive and insensitive)
        // -----------------------------------------------------
        template <typename StringType>
        static bool Contains(StringType const & bigStr, StringType const & smallStr)
        {
            return StringType::npos != bigStr.find(smallStr);
        }

        template <typename StringType>
        static bool ContainsCaseInsensitive(StringType const & bigStr, StringType const & smallStr)
        {
            StringType bigStrToLower(bigStr);
            StringType smallStrToLower(smallStr);
            ToLower(bigStrToLower);
            ToLower(smallStrToLower);
            return StringType::npos != bigStrToLower.find(smallStrToLower);
        }

        // -----------------------------------------------------
        // StartsWith (case sensitive and insensitive)
        // -----------------------------------------------------
        template <typename StringType>
        static bool StartsWith(StringType const & bigStr, StringType const & smallStr)
        {
            return (bigStr.substr(0,smallStr.size()).compare(smallStr) == 0);
        }

        static bool StartsWith(std::wstring const & bigStr, WStringLiteral const & smallStr);

        template <typename StringType>
        static bool StartsWithCaseInsensitive(StringType const & bigStr, StringType const & smallStr)
        {
            StringType bigStrToLower(bigStr);
            StringType smallStrToLower(smallStr);
            ToLower(bigStrToLower);
            ToLower(smallStrToLower);
            return (StartsWith(bigStrToLower, smallStrToLower));
        }

        // -----------------------------------------------------
        // EndsWith (case sensitive and insensitive)
        // -----------------------------------------------------
        template <typename StringType>
        static bool EndsWith(StringType const & bigStr, StringType const & smallStr)
        {
            return (bigStr.size() >= smallStr.size() &&
                bigStr.substr(bigStr.size() - smallStr.size(), smallStr.size()).compare(smallStr) == 0);
        }

        template <typename StringType>
        static bool EndsWithCaseInsensitive(StringType const & bigStr, StringType const & smallStr)
        {
            StringType bigStrToLower(bigStr);
            StringType smallStrToLower(smallStr);
            ToLower(bigStrToLower);
            ToLower(smallStrToLower);
            return (EndsWith(bigStrToLower, smallStrToLower));
        }

        // -----------------------------------------------------
        // Trim characters from the beginning and end of the string
        // -----------------------------------------------------
        template <typename StringType>
        static void TrimLeading(StringType & str, StringType const & trimChars)
        {
            size_t index = str.find_first_not_of(trimChars);
            if (index != StringType::npos)
            {
                str.erase(0, index);
            }
        }

        template <typename StringType>
        static void TrimTrailing(StringType & str, StringType const & trimChars)
        {
            size_t index = str.find_last_not_of(trimChars);
            str.resize(index + 1);
        }

        template <typename StringType>
        static void Trim(StringType & str, StringType const & trimChars)
        {
            TrimLeading<StringType>(str, trimChars);

            TrimTrailing<StringType>(str, trimChars);
        }

        // Eliminates spaces from the beginning and end of the string
        static void TrimSpaces(std::wstring & str);

        static void TrimSpaces(std::string & str);

        // Eliminates all white spaces from the beginning and end of the string
        static void TrimWhitespaces(std::wstring & str);

        static void TrimWhitespaces(std::string & str);

        // Converts a wstring to a string
        static void Utf16ToUtf8(std::wstring const & utf16String, std::string & utf8String);
        static void Utf8ToUtf16(std::string const & utf8String, std::wstring & utf16String);
        static std::string Utf16ToUtf8(std::wstring const & utf16String);
        static std::wstring Utf8ToUtf16(std::string const & utf8String);
        static void UnicodeToAnsi(std::wstring const &uniString, std::string &dst);

        // -----------------------------------------------------
        // Split string into two tokens based on the delimiter.
        // -----------------------------------------------------

        template <typename StringType, typename CharType>
        static void SplitOnce(StringType const & str, StringType & token1, StringType & token2, CharType const & delimiter)
        {
            typename StringType::size_type startTokenPos = str.find_first_of(delimiter);
            typename StringType::size_type endTokenPos = str.length();

            if (StringType::npos != startTokenPos)
            {

                token1 = str.substr(0, startTokenPos);
                startTokenPos = startTokenPos + 1;
                if (startTokenPos < endTokenPos)
                {
                    token2 = str.substr(startTokenPos, endTokenPos - startTokenPos);
                }
            }
            else
            {
                token1 = str.substr(0, endTokenPos);
            }
        }

        // -----------------------------------------------------
        // Split string in multiple tokens based on delimiters.
        // This function treats the delimiters as an array and
        // matches any one character to split the string.
        // -----------------------------------------------------
        template <typename StringType>
        static void Split(StringType const & str, std::vector<StringType> & tokens, StringType const & delimiters, bool skipEmptyTokens = true)
        {
            typename StringType::size_type startTokenPos = 0;
            typename StringType::size_type endTokenPos = str.length() ? 0 : StringType::npos;

            // Till end of string is reached
            while(StringType::npos != endTokenPos)
            {
                endTokenPos = str.find_first_of(delimiters, startTokenPos);
                if(StringType::npos != startTokenPos)
                {
                    StringType token = str.substr(startTokenPos, endTokenPos - startTokenPos);
                    if (!token.empty() || !skipEmptyTokens)
                    {
                        tokens.push_back(token);
                    }

                    startTokenPos = endTokenPos + 1;
                }
            }
        }

        // ---------------------------------------------------------
        // Split string in multiple tokens based on stringDelimiter
        // ---------------------------------------------------------
        template <typename StringType>
        static void SplitOnString(StringType const & str, std::vector<StringType> & tokens, StringType const & stringDelimiter, bool skipEmptyTokens = true)
        {
            typename StringType::size_type startTokenPos = 0;
            typename StringType::size_type endTokenPos = str.length() ? 0 : StringType::npos;

            // Till end of string is reached
            while(StringType::npos != endTokenPos)
            {
                endTokenPos = str.find(stringDelimiter, startTokenPos);
                if(StringType::npos != startTokenPos)
                {
                    StringType token = str.substr(startTokenPos, endTokenPos - startTokenPos);
                    if(!token.empty() || !skipEmptyTokens)
                    {
                        tokens.push_back(token);
                    }

                    startTokenPos = endTokenPos + stringDelimiter.size();
                }
            }
        }

        template <typename StringType>
        static void Replace(StringType & inputString, StringType const & stringToReplace, StringType const & stringToInsert, size_t startIndex)
        {
            size_t i = inputString.find(stringToReplace, startIndex);
            while (i != StringType::npos)
            {
                inputString.replace(i, stringToReplace.size(), stringToInsert);
                i = inputString.find(stringToReplace, i + stringToInsert.size());
            }
        }

        template <typename StringType>
        static void Replace(StringType & inputString, StringType const & stringToReplace, StringType const & stringToInsert)
        {
            StringUtility::Replace<StringType>(inputString, stringToReplace, stringToInsert, 0);
        }

        // -----------------------------------------------------
        // Convert basic types to wstring
        // -----------------------------------------------------
        template <typename T>
        static std::wstring ToWString(T item)
        {
            std::wstring result;
            StringWriter(result).Write(item);
            return result;
        }

        static HRESULT LpcwstrToWstring(LPCWSTR ptr, bool acceptNull, __inout std::wstring & output);
        static HRESULT LpcwstrToWstring(LPCWSTR ptr, bool acceptNull, size_t minSize, size_t maxSize, __inout std::wstring & output);
        static ErrorCode LpcwstrToWstring2(LPCWSTR ptr, bool acceptNull, __inout std::wstring & output);
        static ErrorCode LpcwstrToWstring2(LPCWSTR ptr, bool acceptNull, size_t minSize, size_t maxSize, __inout std::wstring & output);
        static HRESULT LpcwstrToFilePath(LPCWSTR ptr, __out std::wstring & path);
        static ErrorCode LpcwstrToTruncatedWstring(LPCWSTR ptr, bool acceptNull, size_t minSize, size_t maxSize, __inout std::wstring & output);

        static bool TruncateIfNeeded(__in std::wstring & s, size_t maxSize);
        static void ReplaceEndWithTruncateMarker(__in std::wstring & s);
        static std::wstring LimitNumberOfCharacters(__in const std::wstring & s, wchar_t token, size_t maxSize);
        static bool TruncateStartIfNeeded(__in std::wstring & s, size_t maxSize);
        static void ReplaceStartWithTruncateMarker(__in std::wstring & s);

        // -----------------------------------------------------
        // Convert from wstring to basic types
        // -----------------------------------------------------

        template <typename T>
        static bool TryFromWString(
            std::wstring const & string,
            __out T & parsed)
        {
            return TryFromWString(string, parsed, 0);
        }

        template <>
        static bool TryFromWString<bool>(
            std::wstring const & string,
            __out bool & parsed)
        {
            if (StringUtility::AreEqualCaseInsensitive(string, L"true"))
            {
                parsed = true;
                return true;
            }
            else if (StringUtility::AreEqualCaseInsensitive(string, L"false"))
            {
                parsed = false;
                return true;
            }
            else
            {
                std::wstringstream inputStream(string);
                return TraceParseReturn(
                    !(inputStream >> parsed).fail(),
                    __FUNCTION__,
                    string,
                    "bool");
            }
        }

        template <>
        static bool TryFromWString<double>(
            std::wstring const & string,
            __out double & parsed)
        {
            return TraceParseReturn(
                TryParseDouble(string, parsed),
                __FUNCTION__,
                string,
                "double");
        }

        template <typename T>
        struct IsSignedInt //std::is_signed causes compile error somehow
        {
            static const bool value =
                std::is_same<T, char>::value ||
                std::is_same<T, short>::value ||
                std::is_same<T, int>::value ||
                std::is_same<T, long>::value ||
                std::is_same<T, int64>::value ||
                std::is_same<T, __int64>::value;
        };

        template <typename T>
        static bool TryFromWString(
            std::wstring const & string,
            __out T & parsed,
            uint base)
        {
            static_assert(std::is_integral<T>::value, "must be integral types");
            return TraceParseReturn(
                ParseIntegralString<T, IsSignedInt<T>::value>::Try(string, parsed, base),
                __FUNCTION__,
                string,
                typeid(T).name(),
                base);
        }

        template <typename TInt, bool IsSigned>
        struct ParseIntegralString
        {
            static bool Try(
                std::wstring const & string,
                __out TInt & parsed,
                uint base);
        };

        template <typename TInt>
        struct ParseIntegralString<TInt, false>
        {
            static bool Try(
                std::wstring const & string,
                __out TInt & parsed,
                uint base)
            {
                uint64 value = 0;
                auto parseSucceeded = TryParseUInt64(string, value, base);
                parsed = (TInt)value;
                return parseSucceeded && (std::numeric_limits<TInt>::lowest() <= value) && (value <= std::numeric_limits<TInt>::max());
            }
        };

        template <typename TInt>
        struct ParseIntegralString<TInt, true>
        {
            static bool Try(
                std::wstring const & string,
                __out TInt & parsed,
                uint base)
            {
                int64 value = 0;
                auto parseSucceeded = TryParseInt64(string, value, base);
                parsed = (TInt)value;
                return parseSucceeded && (std::numeric_limits<TInt>::lowest() <= value) && (value <= std::numeric_limits<TInt>::max());
            }
        };

        struct ParseBool
        {
            static bool Try(
                std::wstring const & string,
                __out bool & parsed)
            {
                if (AreEqualCaseInsensitive(string, L"true"))
                {
                    parsed = true;
                    return true;
                }
                else if (AreEqualCaseInsensitive(string, L"false"))
                {
                    parsed = false;
                    return true;
                }

                return false;
            }
        };

        // -----------------------------------------------------
        // Convert from NATIVE type
        // -----------------------------------------------------
        static HRESULT FromLPCWSTRArray(ULONG count, LPCWSTR * items, __out std::vector<std::wstring> & output);

        static ULONG GetDataLength(LPCWSTR data)
        {
            // +1 to include trailing null character.
            // http://msdn.microsoft.com/en-us/library/78zh94ax.aspx
            return (data == NULL) ? 0 : static_cast<ULONG>((wcslen(data) + 1) * sizeof(wchar_t));
        }

        // -----------------------------------------------------
        // Case insensitive string collection comparison
        // -----------------------------------------------------
        static bool AreStringCollectionsEqualCaseInsensitive(std::vector<std::wstring> const & v1, std::vector<std::wstring> const & v2);

    private:

        template<typename T>
        inline static int signum(T a, T b)
        {
            return a == b ? 0 : (a > b ? 1 : -1);
        }

        static Common::Global<std::locale> USLocale;
    };

    template<class StringType>
    struct IsLessCaseInsensitiveComparer
    {
        bool operator()(const StringType & a, const StringType & b) const
        {
            return Common::StringUtility::IsLessCaseInsensitive(a,b);
        }
    };

    template<class StringType>
    struct FindCaseInsensitive
    {
        StringType stringType_;
        FindCaseInsensitive(StringType const& stringType) : stringType_(stringType) {}

        bool operator () (StringType const& a) const
        {
            return Common::StringUtility::AreEqualCaseInsensitive(a, stringType_);
        }
    };

    bool operator==(StringLiteral const & lhs, StringLiteral const & rhs);
    bool operator!=(StringLiteral const & lhs, StringLiteral const & rhs);
    bool operator==(StringLiteral const & lhs, std::string const & rhs);
    bool operator==(std::string const & lhs, StringLiteral const & rhs);
    bool operator!=(StringLiteral const & lhs, std::string const & rhs);
    bool operator!=(std::string const & lhs, StringLiteral const & rhs);

    bool operator==(WStringLiteral const & lhs, WStringLiteral const & rhs);
    bool operator!=(WStringLiteral const & lhs, WStringLiteral const & rhs);
    bool operator==(WStringLiteral const & lhs, std::wstring const & rhs);
    bool operator==(std::wstring const & lhs, WStringLiteral const & rhs);
    bool operator!=(WStringLiteral const & lhs, std::wstring const & rhs);
    bool operator!=(std::wstring const & lhs, WStringLiteral const & rhs);

    bool operator==(GlobalString const & lhs, GlobalString const & rhs);
    bool operator!=(GlobalString const & lhs, GlobalString const & rhs);
    bool operator==(GlobalString const & lhs, std::string const & rhs);
    bool operator==(std::string const & lhs, GlobalString const & rhs);
    bool operator!=(GlobalString const & lhs, std::string const & rhs);
    bool operator!=(std::string const & lhs, GlobalString const & rhs);

    bool operator==(GlobalWString const & lhs, GlobalWString const & rhs);
    bool operator!=(GlobalWString const & lhs, GlobalWString const & rhs);
    bool operator==(GlobalWString const & lhs, std::wstring const & rhs);
    bool operator==(std::wstring const & lhs, GlobalWString const & rhs);
    bool operator!=(GlobalWString const & lhs, std::wstring const & rhs);
    bool operator!=(std::wstring const & lhs, GlobalWString const & rhs);
} // end namespace
