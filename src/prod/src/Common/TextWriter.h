// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/macro.h"
#include "Common/Formatter.h"

namespace Common
{
    class VariableArgument;

    struct TextWriterBase { TextWriterBase & base() { return *this; }  };

    class TextWriter : public TextWriterBase
    {
        DENY_COPY(TextWriter);

        struct Writer;

    public:
        TextWriter()
        {}

        void WriteBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen) { WriteAsciiBuffer(buf, ccLen); }
        void WriteBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen) { WriteUnicodeBuffer(buf, ccLen); }
        void WriteChar(wchar_t ch) { WriteBuffer(&ch, 1); }
        void WriteChar(char ch) { WriteBuffer(&ch, 1); }
        virtual void Flush() = 0;

        void WriteNumber(uint64 val, const FormatOptions& format, bool negative);

        void WriteListBegin(int & itemCount)    { WriteChar('('); itemCount = 0; }
        void WriteListEnd(int &)                { WriteChar(')'); }
        void WriteElementBegin(int & itemCount) { if ( itemCount++ > 0 ) WriteChar(' '); }
        void WriteElementEnd(int &)             {}

        void WriteString(std::string const & value);

        void Write(VariableArgument const & arg);
        void Write(StringLiteral format, VariableArgument const & arg0);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10, VariableArgument const & arg11);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10, VariableArgument const & arg11, VariableArgument const & arg12);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10, VariableArgument const & arg11, VariableArgument const & arg12, VariableArgument const & arg13);
        void Write(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10, VariableArgument const & arg11, VariableArgument const & arg12, VariableArgument const & arg13, VariableArgument const & arg14);

        void WriteLine();
        void WriteLine(VariableArgument const & arg);
        void WriteLine(StringLiteral format, VariableArgument const & arg0);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10);
        void WriteLine(StringLiteral format, VariableArgument const & arg0, VariableArgument const & arg1, VariableArgument const & arg2, VariableArgument const & arg3, VariableArgument const & arg4, VariableArgument const & arg5, VariableArgument const & arg6, VariableArgument const & arg7, VariableArgument const & arg8, VariableArgument const & arg9, VariableArgument const & arg10, VariableArgument const & arg11);

    protected:
        virtual ~TextWriter() {}
        virtual void WriteAsciiBuffer(char const * buf, size_t ccLen) = 0;
        virtual void WriteUnicodeBuffer(wchar_t const * buf, size_t ccLen) = 0;

    private:
        void InternalWrite(StringLiteral format, int argCount, VariableArgument const ** args);
    };

    class StringWriter : public TextWriter
    {
        DENY_COPY(StringWriter);

    public:
        StringWriter(std::wstring& buffer, size_t size = 256);
        virtual void WriteAsciiBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen);
        virtual void WriteUnicodeBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen);
        virtual void Flush() {}

    private:
        std::wstring& buffer_;
    };

    class StringWriterA : public TextWriter
    {
        DENY_COPY(StringWriterA);

    public:
        StringWriterA(std::string& buffer, size_t size = 256);
        virtual void WriteUnicodeBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen);
        virtual void WriteAsciiBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen);
        virtual void Flush() {}

    private:
        std::string& buffer_;
    };
}

#pragma region ( CommonTraits )

namespace Common
{
    namespace detail
    {
        template <int N> void safe_ui64toa(uint64 value, char (&str)[N], int radix)
        {
            _ui64toa_s(value, str, N, radix);
        }
        template <int N> void safe_i64toa(int64 value, char (&str)[N], int radix)
        {
            _i64toa_s(value, str, N, radix);
        }
    }

    TextWriter & operator << (TextWriter & w, VariableArgument const & value);
}
#pragma endregion
