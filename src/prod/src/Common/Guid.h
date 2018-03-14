// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <ifdef.h>

#include "Serialization.h"

namespace Common
{
    struct Guid
    {
        struct Hasher
        {
            size_t operator() (Guid const & key) const
            {
                return (key.GetHashCode());
            }
            bool operator() (Guid const & left, Guid const & right) const
            {
                return (left == right);
            }
        };
        
        Guid() { ZeroMemory(&guid, sizeof(guid)); }
        explicit Guid(const ::GUID& rhs) : guid(rhs) {}
        Guid(Guid const & rhs) : guid(rhs.guid) {}

        Guid(int data1,
            ::USHORT data2, 
            ::USHORT data3, 
            byte data4_0,
            byte data4_1,
            byte data4_2,
            byte data4_3,
            byte data4_4,
            byte data4_5,
            byte data4_6,
            byte data4_7
            ) 
        {
            // Validate is_blittable trait below.
            static_assert(sizeof(Guid) == sizeof(GUID), "sizeof(Guid) != sizeof(GUID)");

            guid.Data1 = data1;
            guid.Data2 = data2;    
            guid.Data3 = data3;    
            guid.Data4[0] = data4_0; 
            guid.Data4[1] = data4_1; 
            guid.Data4[2] = data4_2; 
            guid.Data4[3] = data4_3; 
            guid.Data4[4] = data4_4; 
            guid.Data4[5] = data4_5; 
            guid.Data4[6] = data4_6; 
            guid.Data4[7] = data4_7; 
        }

        Guid(std::wstring const & value);

        explicit Guid(std::vector<byte> const & value);

        static Guid& FromKGuid(KGuid& from) {
            return *((Guid*)&from);
        }

        KGuid& ToKGuid() {
            return *((KGuid*)&guid);
        }

        static Guid NewGuid();

        static Guid const & Empty();

        static bool TryParse(std::wstring const & value, Common::Guid & guid);
        static bool TryParse(std::wstring const & value, std::wstring const & traceId, Common::Guid & guid);

        void assign(const Guid& rhs) {
            KMemCpySafe(&guid, sizeof(guid), &rhs.guid, sizeof(guid));
        }
               
        bool operator == (const Guid& rhs) const {
            return ::memcmp(&guid, &rhs.guid, sizeof(guid)) == 0;
        }

        bool operator != (const Guid& rhs) const {
            return !(*this == rhs);
        }

        bool operator < (const Guid& rhs) const {
            return ::memcmp(&guid, &rhs.guid, sizeof(guid)) < 0;
        }

        bool operator <= (const Guid& rhs) const {
            return (*this == rhs) || (*this < rhs);
        }

        bool operator > (const Guid& rhs) const {
            return ::memcmp(&guid, &rhs.guid, sizeof(guid)) > 0;
        }

        bool Equals(const Guid& rhs) const 
        {
            return ::memcmp(&guid, &rhs.guid, sizeof(guid)) == 0;
        }

        void WriteTo(TextWriter&, FormatOptions const &) const;

        std::wstring ToString() const;

        // Returns a string representation of the value of this Guid instance, according to the provided format specifier.
        // 
        // format: A single format specifier that indicates how to format the value of this Guid. 
        // The format parameter can be 'N', 'D'.
        // 'N': 32 digits: 00000000000000000000000000000000
        // 'D': 32 digits separated by hyphens: 00000000-0000-0000-0000-000000000000
        std::wstring ToString(char format) const;

        std::string ToStringA() const;

        void ToBytes(std::vector <byte> & bytes) const;

        ::GUID const & AsGUID() const
        {   
            return guid;
        }

        FABRIC_PRIMITIVE_FIELDS_01(guid);

        int Guid::GetHashCode() const;

        static Common::Guid Test_FromStringHashCode(std::wstring const &);

        static StringLiteral TraceCategory;

    private:
        ::GUID guid;

        static Guid empty_;
        
        static void Parse(std::wstring const & value, Guid & guid);
    };
    static_assert(sizeof(Common::Guid) == sizeof(KGuid), "KGuid and Common::Guid sizes mismatch");

    template<> struct is_blittable<Common::Guid> : public std::integral_constant<bool,true> { };

#if 0
    template <> struct SerializerTrait<Guid>
    {
        static void ReadFrom(SerializeReader & r, Guid& value, std::wstring const & name) 
        {
            std::wstring stringData;
            r.Read(stringData, name);
            value.assign(Guid(stringData));
        }

        static void WriteTo(SerializeWriter & w, Guid const & value, std::wstring const & name) 
        {
            w.Write(value.ToString(), name);
        }
    };

    template <> struct FieldEnumTrait<Guid>
    {
        static void ReadFields(Guid& val, Stream& r) 
        {
            std::vector<byte> bytes;
            FieldEnumTrait<std::vector<byte>>::ReadFields(bytes, r);
            val.assign(Guid(bytes));
        }
        static void WriteFields(Guid& val, Stream& w)
        {
            std::vector<byte> bytes;
            val.ToBytes(bytes);
            FieldEnumTrait<std::vector<byte>>::WriteFields(bytes, w);
        }
        static void WriteXmlTo(const Guid& val, TextWriter& w)
        {
            w.Write("<Guid>{0}</Guid>", val);
        }
    };
#endif

    class Luid 
    {
        const Luid& operator = (const Luid&);  // deny
    public:    
        Luid() { ZeroMemory(&luid, sizeof(luid)); }
        explicit Luid(const ::NET_LUID& rhs) : luid(rhs) {}
        Luid(const Luid& rhs) : luid(rhs.luid) {}
        explicit Luid(const ::ULONG64& val)   {luid.Value = val;}

        void assign(const Luid& rhs) {
            KMemCpySafe(&luid, sizeof(luid), &rhs.luid, sizeof(luid));
        }

        void assign(::NET_LUID rhs) {
            luid = rhs;
        }

        bool operator == (const Luid& rhs) const {
            return this->luid.Value == rhs.luid.Value;
        }

        bool operator < (const Luid& rhs) const {
            return this->luid.Value < rhs.luid.Value;
        }

        bool Equals(const Luid& rhs) const
        {
            return ::memcmp(&luid, &rhs.luid, sizeof(luid)) == 0;
        }

        int64 AsInt64() const
            // TODO: make this a property??
        {
            return this->luid.Value;
        }

        ::NET_LUID AsNetLuid() const
        {
            return this->luid;
        }

        void WriteTo(TextWriter&, FormatOptions const &) const;
    private:
        ::NET_LUID luid;
    };
};
