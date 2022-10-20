// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct BasicObject : public FabricSerializable
{
    CHAR _char1;
    UCHAR _uchar1;
    SHORT _short1;
    USHORT _ushort1;
    bool _bool1;
    ULONG64 _ulong64_1;
    LONG64 _long64_1;
    
    double _double;

    static const ULONG StringSize = 20;
    wchar_t _string[StringSize];

    ULONG _ulong64ArraySize;
    ULONG64 * _ulong64Array;

    GUID _guid;

    BasicObject()
        : _ulong64ArraySize(0)
        , _ulong64Array(nullptr)
    {
        memset(this->_string, 0, StringSize * sizeof(wchar_t));
    }

    ~BasicObject()
    {
        if (this->_ulong64Array != nullptr)
        {
            delete [] this->_ulong64Array;
        }
    }

    bool operator==(BasicObject const & rhs)
    {
        bool equal = __super::operator==(rhs)
                && this->_short1 == rhs._short1
                && this->_ushort1 == rhs._ushort1
                && this->_bool1 == rhs._bool1
                && this->_uchar1 == rhs._uchar1
                && this->_char1 == rhs._char1
                && this->_ulong64_1 == rhs._ulong64_1
                && this->_long64_1 == rhs._long64_1
                && this->_double == rhs._double
                && this->_ulong64ArraySize == rhs._ulong64ArraySize;

        if (equal)
        {
            equal &= (memcmp(this->_string, rhs._string, 20 * 2) == 0);

            equal &= (memcmp(&this->_guid, &rhs._guid, sizeof(GUID)) == 0);

            if (this->_ulong64ArraySize != 0)
            {
                equal &= (memcmp(this->_ulong64Array, rhs._ulong64Array, sizeof(ULONG64) * _ulong64ArraySize) == 0);
            }

        }

        return equal;
    }

    virtual NTSTATUS Write(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Write(stream);

        if (NT_ERROR(status)) return status;

        status = stream->WriteStartType();

        if (NT_ERROR(status)) return status;

        status = stream->WriteShort(this->_short1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteUShort(this->_ushort1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteBool(this->_bool1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteUChar(this->_uchar1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteChar(this->_char1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteInt64(this->_long64_1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteUInt64(this->_ulong64_1);

        if (NT_ERROR(status)) return status;

        status = stream->WriteString(this->_string, 20);

        if (NT_ERROR(status)) return status;

        status = stream->WriteDouble(this->_double);

        if (NT_ERROR(status)) return status;

        status = stream->WriteGuid(this->_guid);

        if (NT_ERROR(status)) return status;

        status = stream->WriteUInt64Array(this->_ulong64Array, this->_ulong64ArraySize);

        if (NT_ERROR(status)) return status;

        return stream->WriteEndType();
    }

    virtual NTSTATUS Read(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Read(stream);

        if (NT_ERROR(status)) return status;

        status = stream->ReadStartType();

        if (NT_ERROR(status)) return status;

        status = stream->ReadShort(this->_short1);

        if (NT_ERROR(status)) return status;

        status = stream->ReadUShort(this->_ushort1);

        if (NT_ERROR(status)) return status;

        status = stream->ReadBool(this->_bool1);

        if (NT_ERROR(status)) return status;

        status = stream->ReadUChar(this->_uchar1);

        if (NT_ERROR(status)) return status;

        status = stream->ReadChar(this->_char1);

        if (NT_ERROR(status)) return status;

        status = stream->ReadInt64(this->_long64_1);

        if (NT_ERROR(status)) return status;

        status = stream->ReadUInt64(this->_ulong64_1);

        if (NT_ERROR(status)) return status;

        ULONG count = StringSize;
        status = stream->ReadString(this->_string, count);

        if (NT_ERROR(status)) return status;

        status = stream->ReadDouble(this->_double);

        if (NT_ERROR(status)) return status;

        status = stream->ReadGuid(this->_guid);

        if (NT_ERROR(status)) return status;

        if (this->_ulong64ArraySize > 0)
        {
            // delete old one if it exists
            this->_ulong64ArraySize = 0;
            delete [] this->_ulong64Array;
        }

        ULONG size = 0;
        status = stream->ReadUInt64Array(nullptr, size);

        if (NT_ERROR(status)) return status;

        if (status == K_STATUS_BUFFER_TOO_SMALL)
        {
            this->_ulong64Array = new ULONG64[size];

            status = stream->ReadUInt64Array(this->_ulong64Array, size);

            if (NT_ERROR(status))
            {
                delete [] this->_ulong64Array;
                this->_ulong64ArraySize = 0;
            }

            this->_ulong64ArraySize = size;
        }

        if (NT_ERROR(status)) return status;

        return stream->ReadEndType();
    }
};

struct BasicObjectChild : public BasicObject
{
    LONG c_long1;

    BasicObjectChild()
    {
    }

    bool operator==(BasicObjectChild const & rhs)
    {
        bool equal = __super::operator==(rhs);

        if (equal)
        {
            equal = this->c_long1 == rhs.c_long1;
        }

        return equal;
    }

    virtual NTSTATUS Write(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Write(stream);

        if (NT_ERROR(status)) return status;

        status = stream->WriteStartType();

        if (NT_ERROR(status)) return status;

        status = stream->WriteInt32(this->c_long1);

        if (NT_ERROR(status)) return status;

        stream->WriteEndType();

        return status;
    }

    virtual NTSTATUS Read(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Read(stream);

        if (NT_ERROR(status)) return status;

        status = stream->WriteStartType();

        if (NT_ERROR(status)) return status;

        status = stream->ReadInt32(this->c_long1);

        if (NT_ERROR(status)) return status;

        stream->WriteEndType();

        return status;
    }
};

struct BasicObjectUsingMacro : public FabricSerializable
{
    ULONG _ulong;
    bool _bool;

    BasicObjectUsingMacro()
    {
    }

    BasicObjectUsingMacro(BasicObjectUsingMacro && rhs)
    {
        this->_ulong = rhs._ulong;
        this->_bool = rhs._bool;
    }

    BasicObjectUsingMacro& operator=(BasicObjectUsingMacro && rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        __super::operator=(Ktl::Move(rhs));

        this->_ulong = rhs._ulong;
        this->_bool = rhs._bool;

        return *this;
    }

    bool operator==(__in BasicObjectUsingMacro const & rhs)
    {                  
        return __super::operator==(rhs)
            && this->_ulong == rhs._ulong
            && this->_bool == rhs._bool;
    }

    K_FIELDS_02(_ulong, _bool);
};

struct BasicChildObjectUsingMacro : public BasicObjectUsingMacro
{
    SHORT _short;
    KGuid _guid;

    BasicChildObjectUsingMacro()
        : _short(1)
    {
    }

    virtual bool HasDefaultValues()
    {
        return this->_short == 1
            && this->_guid == KGuid();
    }

    bool operator==(__in BasicChildObjectUsingMacro const & rhs)
    {
        bool equal = __super::operator==(rhs);

        return equal
            && this->_short == rhs._short
            && this->_guid == rhs._guid;
    }

    K_FIELDS_02(_short, _guid);
};

struct BasicChild2ObjectUsingMacro : public BasicChildObjectUsingMacro
{
    CHAR _char;
    LONG64 _long64;

    BasicChild2ObjectUsingMacro()
        : _char('\0')
        , _long64(0xFAB)
    {
    }

    virtual bool HasDefaultValues()
    {
        return __super::HasDefaultValues()
            && this->_char == '\0'
            && this->_long64 == 0xFAB;
    }

    bool operator==(__in BasicChild2ObjectUsingMacro const & rhs)
    {
        bool equal = __super::operator==(rhs);

        return equal
            && this->_char == rhs._char
            && this->_long64 == rhs._long64;
    }

    K_FIELDS_02(_char, _long64);
};
