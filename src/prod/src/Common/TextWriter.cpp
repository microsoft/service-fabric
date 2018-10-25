// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <codecvt>

#include "Common/TextWriter.h"

namespace Common
{
    StringWriter::StringWriter(std::wstring& buffer, size_t size)
        : buffer_(buffer)
    {
        buffer_.reserve(size);
    }

    StringWriterA::StringWriterA(std::string& buffer, size_t size) 
        : buffer_(buffer)
    {
        buffer_.reserve(size);
    }

    void StringWriter::WriteAsciiBuffer(char const * buf, size_t ccLen)
    {
        // LATER: asciify properly
        for (size_t i = 0; i < ccLen; ++i) buffer_.push_back(buf[i]);
    }
    void StringWriter::WriteUnicodeBuffer(wchar_t const * buf, size_t ccLen) 
    {
        buffer_.append(buf, ccLen); 
    }

    void StringWriterA::WriteUnicodeBuffer(wchar_t const * buf, size_t ccLen)
    {
        // LATER: unicodize properly
        for (size_t i = 0; i < ccLen; ++i) buffer_.push_back((char)buf[i]);
    }
    void StringWriterA::WriteAsciiBuffer(char const * buf, size_t ccLen)  
    { 
        buffer_.append(buf, ccLen);
    }

    // refactor: get rid of the pragma warning 4146

    void TextWriter::WriteNumber(uint64 val, const FormatOptions& formatOpt, bool negative)
    {
        static char HexSmall[] = "0123456789abcdef";
        static char HexCapitals[] = "0123456789ABCDEF";

        int base = 10;
        bool lowerCase = true;
        bool prefix = false;
        int pad = formatOpt.width;

        if(!formatOpt.formatString.empty())
        {
            switch (formatOpt.formatString[0])
            {
                case 'x': base = 16; lowerCase = true; negative = false; break;
                case 'X': base = 16; lowerCase = false; negative = false; break;
                case 'b': base = 2; break;
                case 'B': base = 2; prefix = true; break;
            }
        }

        const char* hex = lowerCase ? HexSmall : HexCapitals;
        const int N = 64;
        ASSERT_IFNOT((base == 2) || (base == 10) || (base == 16), "invalid base", base);
        ASSERT_IFNOT(pad >= -1 && pad < 33, "formatOpt width out of range", pad);

        char buf[N];
        int i = N;

        if (negative) 
        {
    #pragma warning(disable: 4146) // unary minus operator applied to unsigned type
            val = -val;
    #pragma warning(error: 4146)
        }

        if (2 == base && prefix) {
            buf[--i] = '\'';
            ++pad; // to compensate for the trailing '
        }

        for(;;) 
        {
            buf[--i] = hex[val % base];
            val /= base;
            if (0 == val) 
            {
                break;
            }
        }
        if (negative) 
        {
            buf[--i] = '-';
        }

        if (formatOpt.leadingZero)
        {
            for (int j = N - pad; j < i; ++j)
                buf[j] = '0';
        }
        else
        {
            for (int j = N - pad; j < i; ++j)
                buf[j] = ' ';
        }

        if (N - pad < i) 
            i = N - pad;

        if (16 == base) 
        {
            //buf[--i] = 'x';
            //buf[--i] = '0';
        }
        else if (2 == base && prefix)
        {
            buf[--i] = '\'';
            buf[--i] = 'b';
        }

        WriteAsciiBuffer(buf + i, 64 - i);

        if (formatOpt.formatString[0] == 'r')
        {
            switch(val % 10)
            {
            case 1: WriteAsciiBuffer("st",2); break;
            case 2: WriteAsciiBuffer("nd",2); break;
            case 3: WriteAsciiBuffer("rd",2); break;
            default: WriteAsciiBuffer("th",2); break;
            }
        }
    }

	void TextWriter::Write(VariableArgument const & arg)
	{
		arg.WriteTo(*this, null_format);
	}

	void TextWriter::WriteLine(VariableArgument const & arg)
	{
		Write(arg);
		WriteLine();
	}

    void TextWriter::WriteLine()
    {
        WriteBuffer("\r\n", 2);
        Flush();
    }

    void TextWriter::WriteString(std::string const & value)
    {
        WriteBuffer(value.c_str(), value.size());
    }

    // Restrictions of the formatting:
    // - A string cannot end on "{"
    // - "{" must be followed by a number
    // - Others
    void TextWriter::InternalWrite(StringLiteral format, int argCount, VariableArgument const ** args)
    {
        const char* chunk_beg = format.begin();
        const char* p = chunk_beg;

        while (*p)
        {
            if (*p == '{')
            {
                WriteBuffer(chunk_beg, p - chunk_beg);

                ++p;

                if (*p == 0)
                {
                    *this << " ** Bad Format String **";
                    return;
                }

                if (*p == '{') { 
                    chunk_beg = p;
                    p++;
                    continue;
                }

                Common::FormatOptions formatOpt(0, false, "");

                int index = *p - '0';
                if (index < 0 || index > 9)
                {
                    *this << " ** Bad insert {" << *p << "} **";
                    return;
                }

                ++p;
                while (*p >= '0' && *p <= '9')
                {
                    index = index * 10 + (*p - '0');
                    p++;
                }

                bool alignmentParsed = false;

                // =========== Read Alignment if exists
                if (*p == ',') {
                    int sign = 1;

                    ++p;

                    if (*p == '-') { ++p; sign = -1; }
                    else if (*p == '0') { ++p; formatOpt.leadingZero = true; }

                    while ( (*p >= '0') && (*p <= '9') )
                    {
                        formatOpt.width = formatOpt.width * 10 + (*p - '0');
                        ++p;
                    }
                    formatOpt.width *= sign;
                    alignmentParsed = true;
                }

                // ======== Combine width and type modifier
                if (*p == ':')
                {
                    if (!alignmentParsed)
                    {
                        int sign = 1;

                        ++p;

                        if (*p == '-') { ++p; sign = -1; }
                        else if (*p == '0') { ++p; formatOpt.leadingZero = true; }

                        while ( (*p >= '0') && (*p <= '9') )
                        {
                            formatOpt.width = formatOpt.width * 10 + (*p - '0');
                            ++p;
                        }

                        formatOpt.width *= sign;
                        --p; // code below will do ++p
                    }

                    const char* beg = ++p;
                    for ( ; *p != '}' ; ++p )
                    {
                        if (*p == 0) {
                            *this << "** Bad formatOpt string **";
                            return;
                        }
                    }
                    formatOpt.formatString.assign(beg, p);
                }

                if (*p != '}')
                {
                    *this << "** Bad insert {" << index << "} **";
                    return;
                }

                if (index < argCount && args[index]->IsValid())
                {
                    args[index]->WriteTo(*this, formatOpt);
                }
                else
                {
                    *this << ">> " << index << " ** insert index too big <<";
                }

                chunk_beg = p + 1;
            }
            ++p;
        } // for:

        WriteBuffer(chunk_beg, p - chunk_beg);
    }

    TextWriter & operator << (TextWriter & w, VariableArgument const & value) 
    {
        value.WriteTo(w, null_format);
        return w;
    }

	namespace detail
	{
		std::string format_handler::operator () (
			StringLiteral format,
			VariableArgument const & arg0,
			VariableArgument const & arg1,
			VariableArgument const & arg2,
			VariableArgument const & arg3,
			VariableArgument const & arg4,
			VariableArgument const & arg5,
			VariableArgument const & arg6,
			VariableArgument const & arg7,
			VariableArgument const & arg8,
			VariableArgument const & arg9,
			VariableArgument const & arg10,
			VariableArgument const & arg11,
			VariableArgument const & arg12,
			VariableArgument const & arg13) const
		{
			std::string result;
			result.reserve(64);
			StringWriterA(result).Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13);
			return result;
		}

		std::wstring wformat_handler::operator () (
			StringLiteral format,
			VariableArgument const & arg0,
			VariableArgument const & arg1,
			VariableArgument const & arg2,
			VariableArgument const & arg3,
			VariableArgument const & arg4,
			VariableArgument const & arg5,
			VariableArgument const & arg6,
			VariableArgument const & arg7,
			VariableArgument const & arg8,
			VariableArgument const & arg9,
			VariableArgument const & arg10,
			VariableArgument const & arg11,
			VariableArgument const & arg12,
			VariableArgument const & arg13) const
		{
			std::wstring result;
			result.reserve(64);
			StringWriter(result).Write(format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13);
			return result;
		}

		std::wstring wformat_handler::operator () (
			std::wstring const & wformat,
			VariableArgument const & arg0,
			VariableArgument const & arg1,
			VariableArgument const & arg2,
			VariableArgument const & arg3,
			VariableArgument const & arg4,
			VariableArgument const & arg5,
			VariableArgument const & arg6,
			VariableArgument const & arg7,
			VariableArgument const & arg8,
			VariableArgument const & arg9,
			VariableArgument const & arg10,
			VariableArgument const & arg11,
			VariableArgument const & arg12,
			VariableArgument const & arg13) const
		{
			std::string format;
			StringUtility::Utf16ToUtf8(wformat, format);
			StringLiteral lformat(format.c_str(), format.c_str() + format.size());

			std::wstring result;
			result.reserve(64);
			StringWriter(result).Write(lformat, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13);
			return result;
		}
	}
}