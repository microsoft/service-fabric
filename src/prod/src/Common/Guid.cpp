// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <ctype.h>
#include <rpc.h>


#include "Common/Guid.h"

namespace Common
{
    StringLiteral Guid::TraceCategory("Guid");

    Guid Guid::empty_ = Guid();

    inline wchar_t HexVal(int ch) {
        return wchar_t( (ch >= '0' && ch <= '9') ? ch - '0' : ( (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 : ch - 'A' + 10 ) ); 
    }

    ::UINT HexHelper(const wchar_t* s, int n)
    {
        ::UINT res = 0;
        while(n--) {
            res = res * 16 + HexVal(*s++);
        }
        return res;
    }

    Guid Guid::NewGuid()
    {
        ::GUID guid;
        CHK_W32(UuidCreate(&guid));

        return Guid(guid);
    }

    Guid const & Guid::Empty()
    {
        return Guid::empty_;
    }

    Guid::Guid(std::wstring const & value) {
        coding_error_assert( value.size() == 36 );

        Parse(value, *this);
    }

    Guid::Guid(std::vector<byte> const & bytes)
    {
        ASSERT_IF(bytes.size() != sizeof(guid), "Guid size needs to be 16 bytes");
        KMemCpySafe(&guid, sizeof(guid), bytes.data(), sizeof(guid));
    }

    void Guid::ToBytes(std::vector <byte> & bytes) const
    {
        bytes.resize(sizeof(guid));
        KMemCpySafe(bytes.data(), bytes.size(), &guid, sizeof(guid));
    }

    std::wstring Guid::ToString() const
    {
        std::wstring result;
        result.reserve(39);
        StringWriter(result).Write(*this);
        return result;
    }

    std::wstring Guid::ToString(char format) const
    {
        switch (format)
        {
        case 'N':
        {
            return wformatString(
                "{0,08:x}{1,04:x}{2,04:x}{3,02:x}{4,02:x}{5,02:x}{6,02:x}{7,02:x}{8,02:x}{9,02:x}{10,02:x}",
                guid.Data1,
                guid.Data2,
                guid.Data3,
                guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        }
        case 'D':
        {
            return wformatString(
                "{0,08:x}-{1,04:x}-{2,04:x}-{3,02:x}{4,02:x}-{5,02:x}{6,02:x}{7,02:x}{8,02:x}{9,02:x}{10,02:x}",
                guid.Data1,
                guid.Data2,
                guid.Data3,
                guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        }
        default:
        {
            CODING_ASSERT("Unexpected format {0}", format);
        }
        }
    }

    std::string Guid::ToStringA() const
    {
        std::string result;
        result.reserve(39);
        Common::StringWriterA(result).Write(*this);
        return result;
    }

    void Guid::WriteTo (TextWriter& w, FormatOptions const&) const
    {
        w.Write("{0,08:x}-{1,04:x}-{2,04:x}-{3,02:x}{4,02:x}-",
            guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1]);
        w.Write("{0,02:x}{1,02:x}{2,02:x}{3,02:x}{4,02:x}{5,02:x}",
            guid.Data4[2], guid.Data4[3], guid.Data4[4], 
            guid.Data4[5], guid.Data4[6], guid.Data4[7]); 
    }

    void Luid::WriteTo (TextWriter& w, FormatOptions const &) const
    {   //TODO: print all the LUID, not only lower part.  
        w.Write("<luid>{0:x}'{1:x}'{2:x}</luid>", 
            luid.Info.Reserved, luid.Info.NetLuidIndex, luid.Info.IfType);
    }

    // Taken from the .NET Framework implementation of System.Guid.GetHashCode
    int Guid::GetHashCode() const
    {
        DWORD a = guid.Data1;
        USHORT b = guid.Data2;
        USHORT c = guid.Data3;
        BYTE f = guid.Data4[2];
        BYTE k = guid.Data4[7];

        return (a ^ ((b << 0x10) | c)) ^ ((f << 0x18) | k);
    }

    // The only purpose of this is to allow running multiple clusters concurrently
    // on the same machine (e.g. multiple instances of FabricTest) by ensuring
    // a unique KTL shared log ID for different cluster instances.
    //
    // Just use a simple weak hash for now. Use a stronger hash if this isn't
    // good enough for testing purposes.
    //
    Guid Guid::Test_FromStringHashCode(std::wstring const & path)
    {
        auto hashCode = StringUtility::GetHash(path);

        int data1 = 0;
        USHORT data2 = 0;
        USHORT data3 = 0;

        byte data4[8];
        for (auto ix=0; ix<8; ++ix)
        {
            data4[ix] = (hashCode >> ((7-ix) * 8)) & 0xFF;
        }

        return Guid(data1, data2, data3, data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7]);
    }

    bool Guid::TryParse(std::wstring const & value, Guid & guid)
    {
        if (value.length() != 36) return false;
        for (int i = 0; i < 36; i++) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                if (value[i] != '-') return false;
            }
            else {
                if (isxdigit(value[i]) == 0) return false;
            }
        }
        
        Parse(value, guid);
        
        return true;
    }

    bool Guid::TryParse(std::wstring const & value, std::wstring const & traceId, Guid & guid)
    {
        if (!Guid::TryParse(value, guid))
        {
            Trace.WriteWarning(Guid::TraceCategory, traceId, "Invalid guid - {0}", value);
            return false;
        }

        return true;
    }
    
    void Guid::Parse(std::wstring const & value, Guid & guid)
    {
        guid.guid.Data1 = HexHelper(value.c_str() +  0, 8);
        guid.guid.Data2 = ::USHORT(HexHelper(value.c_str() +  9, 4));
        guid.guid.Data3 = ::USHORT(HexHelper(value.c_str() + 14, 4));
        guid.guid.Data4[0] = byte(HexHelper(value.c_str() + 19, 2));
        guid.guid.Data4[1] = byte(HexHelper(value.c_str() + 21, 2));
        guid.guid.Data4[2] = byte(HexHelper(value.c_str() + 24, 2));
        guid.guid.Data4[3] = byte(HexHelper(value.c_str() + 26, 2));
        guid.guid.Data4[4] = byte(HexHelper(value.c_str() + 28, 2));
        guid.guid.Data4[5] = byte(HexHelper(value.c_str() + 30, 2));
        guid.guid.Data4[6] = byte(HexHelper(value.c_str() + 32, 2));
        guid.guid.Data4[7] = byte(HexHelper(value.c_str() + 34, 2));
    }

} // end namespace Common
