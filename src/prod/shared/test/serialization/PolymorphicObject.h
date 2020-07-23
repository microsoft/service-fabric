// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct PolymorphicObject : public FabricSerializable
{
    SHORT _short1;
    KArray<ULONG> _ulongArray1;
    KWString _string;

    PolymorphicObject()
        : _ulongArray1(Common::GetSFDefaultKtlSystem().NonPagedAllocator())
        , _string(Common::GetSFDefaultKtlSystem().NonPagedAllocator())
    {
        this->SetConstructorStatus(this->_ulongArray1.Status());

        this->SetConstructorStatus(this->_string.Status());
    }

    bool operator==(PolymorphicObject const & rhs)
    {
        return (
            this->_short1 == rhs._short1
            && IsArrayEqual<ULONG>(this->_ulongArray1, rhs._ulongArray1)
            && (this->_string.CompareTo(rhs._string) == 0));
    }

    virtual NTSTATUS GetTypeInformation(__out FabricTypeInformation & typeInformation) const
    {
        typeInformation.buffer = reinterpret_cast<UCHAR const *>(&PolymorphicObject::Id);
        typeInformation.length = sizeof(PolymorphicObject::Id);

        return STATUS_SUCCESS;
    }

    virtual ULONG64 GetId() const
    {
        return Id;
    }

    K_FIELDS_03(_short1, _ulongArray1, _string);

    static const ULONG64 Id = 0;

    static IFabricSerializable * Activator(FabricTypeInformation typeInformation);
};

struct PolymorphicObjectChild1 : public PolymorphicObject
{
    LONG c_long1;

    PolymorphicObjectChild1()
    {
    }

    bool operator==(PolymorphicObjectChild1 const & rhs)
    {
        return (__super::operator==(rhs) && (this->c_long1 == rhs.c_long1));
    }

    //bool operator==(PolymorphicObject const & rhs)
    //{
    //    if (typeid(rhs) != typeid(*this))
    //    {
    //        return false;
    //    }

    //    PolymorphicObjectChild1 * rhsPtr = static_cast<PolymorphicObjectChild1*>(&rhs);

    //    return (__super::operator==(rhs) && (this->c_long1 == rhsPtr->c_long1));
    //}

    virtual NTSTATUS GetTypeInformation(__out FabricTypeInformation & typeInformation) const
    {
        typeInformation.buffer = reinterpret_cast<UCHAR const *>(&PolymorphicObjectChild1::Id);
        typeInformation.length = sizeof(PolymorphicObjectChild1::Id);

        return STATUS_SUCCESS;
    }

    virtual ULONG64 GetId() const
    {
        return Id;
    }

    K_FIELDS_01(c_long1);

    static const ULONG64 Id = 1;
};


struct PolymorphicObjectChild2 : public PolymorphicObject
{
    LONG c_long2;

    PolymorphicObjectChild2()
    {
    }

    bool operator==(PolymorphicObjectChild2 const & rhs)
    {
        return (__super::operator==(rhs) && (this->c_long2 == rhs.c_long2));
    }

    virtual NTSTATUS GetTypeInformation(__out FabricTypeInformation & typeInformation) const
    {
        typeInformation.buffer = reinterpret_cast<UCHAR const *>(&PolymorphicObjectChild2::Id);
        typeInformation.length = sizeof(PolymorphicObjectChild2::Id);

        return STATUS_SUCCESS;
    }

    virtual ULONG64 GetId() const
    {
        return Id;
    }

    K_FIELDS_01(c_long2);

    static const ULONG64 Id = 2;
};

IFabricSerializable * PolymorphicObject::Activator(FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || typeInformation.length != sizeof(ULONG64))
    {
        return nullptr;
    }

    ULONG64 id = *(reinterpret_cast<ULONG64 const *>(typeInformation.buffer));

    switch (id)
    {
    case 0:
        return _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultKtlSystem().NonPagedAllocator()) PolymorphicObject();
    case 1:
        return _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultKtlSystem().NonPagedAllocator()) PolymorphicObjectChild1();
    case 2:
        return _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultKtlSystem().NonPagedAllocator()) PolymorphicObjectChild2();
    default:
        return nullptr;
    }
}

