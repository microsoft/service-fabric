// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

IFabricSerializable * MyActivator(FabricTypeInformation)
{
    return _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) BasicObject();
}

struct BasicObjectWithPointers : public FabricSerializable
{
    BasicObject * _basicObject1;
    BasicObject * _basicObject2;

    BasicObjectWithPointers()
        : _basicObject1(nullptr)
        , _basicObject2(nullptr)
    {
    }

    bool operator==(BasicObjectWithPointers const & rhs)
    {
        return IsPointerEqual(this->_basicObject1, rhs._basicObject1)
            && IsPointerEqual(this->_basicObject2, rhs._basicObject2);
    }

    virtual NTSTATUS Write(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Write(stream);

        if (NT_ERROR(status)) return status;

        status = stream->WriteStartType();

        if (NT_ERROR(status)) return status;

        status = stream->WritePointer(this->_basicObject1);

        if (NT_ERROR(status)) return status;

        status = stream->WritePointer(this->_basicObject2);

        if (NT_ERROR(status)) return status;

        stream->WriteEndType();

        return status;
    }

    virtual NTSTATUS Read(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Read(stream);

        if (NT_ERROR(status)) return status;

        status = stream->ReadStartType();

        if (NT_ERROR(status)) return status;

        IFabricSerializable* ptr[1];

        status = stream->ReadPointer(ptr, MyActivator);

        this->_basicObject1 = static_cast<BasicObject*>(ptr[0]);

        if (NT_ERROR(status)) return status;

        status = stream->ReadPointer(ptr, MyActivator);

        this->_basicObject2 = static_cast<BasicObject*>(ptr[0]);

        if (NT_ERROR(status)) return status;

        status = stream->ReadEndType();

        return status;
    }
};
