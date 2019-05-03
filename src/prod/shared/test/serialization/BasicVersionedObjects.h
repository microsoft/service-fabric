// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct BasicObjectV1 : public FabricSerializable
{
    CHAR _char1;

    BasicObjectV1()
        : _char1('a')
    {
    }

    bool operator==(BasicObjectV1 const & rhs)
    {
        return this->_char1 == rhs._char1;
    }

    K_FIELDS_01(_char1);
};

struct BasicUnknownNestedObject : public FabricSerializable
{
    CHAR _char1;
    ULONG64 _ulong64;

    BasicUnknownNestedObject()
        : _char1('a')
        , _ulong64(5)
    {
    }

    BasicUnknownNestedObject(BasicUnknownNestedObject && rhs)
    {
        this->_char1 = rhs._char1;
        this->_ulong64 = rhs._ulong64;
    }

    BasicUnknownNestedObject& operator=(BasicUnknownNestedObject && rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        __super::operator=(Ktl::Move(rhs));

        this->_char1 = rhs._char1;
        this->_ulong64 = rhs._ulong64;

        return *this;
    }


    typedef KUniquePtr<BasicUnknownNestedObject> UPtr;

    bool operator==(BasicUnknownNestedObject const & rhs)
    {
        return this->_char1 == rhs._char1
            && this->_ulong64 == rhs._ulong64;
    }

    K_FIELDS_02(_char1, _ulong64);
};

struct BasicObjectV2 : public FabricSerializable
{
    CHAR _char1;
    ULONG64 _ulong64;
    SHORT _short1;
    KGuid _kguid;
    KArray<CHAR> _charArray;
    BasicUnknownNestedObject _basicUnknownNested;
    LONG _long1;
    BasicUnknownNestedObject::UPtr _basicUnknownNestedUPtr;
    ULONG _ulong1;

    static const ULONG64 DefaultULong64 = 0;

    BasicObjectV2()
        : _char1('a')
        , _ulong64(DefaultULong64)
        , _short1(0)
        , _charArray(Common::GetSFDefaultPagedKAllocator())
        , _long1(1)
        , _ulong1(0xf)
    {
        this->_kguid.CreateNew();
        this->SetConstructorStatus(this->_charArray.Status());
        this->SetConstructorStatus(this->_basicUnknownNested.Status());
    }

    bool operator==(BasicObjectV2 const & rhs)
    {
        return this->_char1 == rhs._char1
            && this->_ulong64 == rhs._ulong64
            && this->_short1 == rhs._short1
            && this->_kguid == rhs._kguid
            && IsArrayEqual(this->_charArray, rhs._charArray)
            && this->_basicUnknownNested == rhs._basicUnknownNested
            && this->_long1 == rhs._long1
            && IsPointerEqual<BasicUnknownNestedObject>(&(*this->_basicUnknownNestedUPtr), &(*rhs._basicUnknownNestedUPtr))
            && this->_ulong1 == rhs._ulong1;
    }

    K_FIELDS_09(_char1, _ulong64, _short1, _kguid, _charArray, _basicUnknownNested, _long1, _basicUnknownNestedUPtr, _ulong1);
};

struct BasicInheritedObjectV1 : public FabricSerializable
{
    CHAR _char1;

    BasicInheritedObjectV1()
        : _char1('a')
    {
    }

    virtual ~BasicInheritedObjectV1()
    {
    }

    bool operator==(BasicInheritedObjectV1 const & rhs)
    {
        return this->_char1 == rhs._char1;
    }

    K_FIELDS_01(_char1);
};

struct BasicInheritedObjectV1Child : public BasicInheritedObjectV1
{
    ULONG64 _ulong64;

    BasicInheritedObjectV1Child()
        : _ulong64(0)
    {
    }

    bool operator==(BasicInheritedObjectV1Child const & rhs)
    {
        bool equal = __super::operator==(rhs);

        return equal
            && this->_ulong64 == rhs._ulong64;
    }

    K_FIELDS_01(_ulong64);
};

struct BasicInheritedObjectV2 : public FabricSerializable
{
    CHAR _char1;
    KWString _newString;

    BasicInheritedObjectV2()
        : _char1('a')
        , _newString(Common::GetSFDefaultPagedKAllocator(), L"Stringgggg")
    {
    }

    virtual ~BasicInheritedObjectV2()
    {
    }

    bool operator==(BasicInheritedObjectV2 const & rhs)
    {
        return this->_char1 == rhs._char1
            && this->_newString.CompareTo(rhs._newString) == 0;
    }

    K_FIELDS_02(_char1, _newString);
};

struct BasicInheritedObjectV2Child : public BasicInheritedObjectV2
{
    ULONG64 _ulong64;
    SHORT _short1;
    KGuid _kguid;

    static const ULONG64 DefaultULong64 = 0;

    BasicInheritedObjectV2Child()
        : _ulong64(DefaultULong64)
        , _short1(0)
    {
        this->_kguid.CreateNew();
    }

    bool operator==(BasicInheritedObjectV2Child const & rhs)
    {
        bool equal = __super::operator==(rhs);

        return equal
            && this->_ulong64 == rhs._ulong64
            && this->_short1 == rhs._short1
            && this->_kguid == rhs._kguid;
    }

    K_FIELDS_03(_ulong64, _short1, _kguid);
};

struct BasicObjectWithArraysV1 : public FabricSerializable
{
    LONG64 _long64;

    BasicObjectWithArraysV1()
        : _long64(0xf7854)
    {
    }

    bool operator==(BasicObjectWithArraysV1 const & rhs)
    {
        return this->_long64 == rhs._long64;
    }

    K_FIELDS_01(_long64);
};

DEFINE_USER_ARRAY_UTILITY(BasicUnknownNestedObject);

struct BasicObjectWithArraysV2 : public FabricSerializable
{
    LONG64 _long64;
    KArray<bool> _boolArray;
    KArray<CHAR> _charArray;
    KArray<UCHAR> _ucharArray;
    KArray<SHORT> _shortArray;
    KArray<USHORT> _ushortArray;
    KArray<LONG> _longArray;
    KArray<ULONG> _ulongArray;
    KArray<LONG64> _long64Array;
    KArray<ULONG64> _ulong64Array;

    KArray<GUID> _guidArray;
    KArray<DOUBLE> _doubleArray;

    KArray<BasicUnknownNestedObject> _basicUnknownNestedObjectArray;
    KArray<BasicUnknownNestedObject::UPtr> _basicUnknownNestedPointerArray;

    // TODO: KArray<UniquePtr<T> isn't allowed, might have to test raw array
    //KArray<> _ulongArray;

    BasicObjectWithArraysV2()
        : _long64(5454)
        , _boolArray(Common::GetSFDefaultPagedKAllocator())
        , _charArray(Common::GetSFDefaultPagedKAllocator())
        , _ucharArray(Common::GetSFDefaultPagedKAllocator())
        , _shortArray(Common::GetSFDefaultPagedKAllocator())
        , _ushortArray(Common::GetSFDefaultPagedKAllocator())
        , _longArray(Common::GetSFDefaultPagedKAllocator())
        , _ulongArray(Common::GetSFDefaultPagedKAllocator())
        , _long64Array(Common::GetSFDefaultPagedKAllocator())
        , _ulong64Array(Common::GetSFDefaultPagedKAllocator())
        , _guidArray(Common::GetSFDefaultPagedKAllocator())
        , _doubleArray(Common::GetSFDefaultPagedKAllocator())
        , _basicUnknownNestedObjectArray(Common::GetSFDefaultPagedKAllocator())
        , _basicUnknownNestedPointerArray(Common::GetSFDefaultPagedKAllocator())
    {
        
        this->SetConstructorStatus(this->_boolArray.Status());
        this->SetConstructorStatus(this->_charArray.Status());
        this->SetConstructorStatus(this->_ucharArray.Status());
        this->SetConstructorStatus(this->_shortArray.Status());
        this->SetConstructorStatus(this->_ushortArray.Status());
        this->SetConstructorStatus(this->_longArray.Status());
        this->SetConstructorStatus(this->_ulongArray.Status());
        this->SetConstructorStatus(this->_long64Array.Status());
        this->SetConstructorStatus(this->_ulong64Array.Status());

        this->SetConstructorStatus(this->_guidArray.Status());
        this->SetConstructorStatus(this->_doubleArray.Status());
        this->SetConstructorStatus(this->_basicUnknownNestedObjectArray.Status());
    }

    bool operator==(BasicObjectWithArraysV2 const & rhs)
    {
        bool equal = this->_long64 == rhs._long64
            && IsArrayEqual(this->_boolArray, rhs._boolArray)
            && IsArrayEqual(this->_charArray, rhs._charArray)
            && IsArrayEqual(this->_ucharArray, rhs._ucharArray)
            && IsArrayEqual(this->_shortArray, rhs._shortArray)
            && IsArrayEqual(this->_ushortArray, rhs._ushortArray)
            && IsArrayEqual(this->_longArray, rhs._longArray)
            && IsArrayEqual(this->_ulongArray, rhs._ulongArray)
            && IsArrayEqual(this->_long64Array, rhs._long64Array)
            && IsArrayEqual(this->_ulong64Array, rhs._ulong64Array)
            && IsArrayEqual(this->_guidArray, rhs._guidArray)
            && IsArrayEqual(this->_doubleArray, rhs._doubleArray)
            && IsArrayEqual(this->_basicUnknownNestedObjectArray, rhs._basicUnknownNestedObjectArray)
            ;

        equal = equal
            && IsArrayEqual(this->_basicUnknownNestedPointerArray, rhs._basicUnknownNestedPointerArray)
            ;

        return equal;
    }

    K_FIELDS_14(_long64, _boolArray, _charArray, _ucharArray, _shortArray, _ushortArray, _longArray, _ulongArray, _long64Array, _ulong64Array, _guidArray, _doubleArray, _basicUnknownNestedObjectArray, _basicUnknownNestedPointerArray);
};
