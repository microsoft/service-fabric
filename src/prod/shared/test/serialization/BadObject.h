// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct BadObject : public FabricSerializable
{
    double _double;

    BadObject()
        : _double(0)
    {
    }

    bool operator==(BadObject const & rhs)
    {
        return this->_double == rhs._double;
    }

    virtual NTSTATUS Write(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Write(stream);

        if (NT_ERROR(status)) return status;

        status = stream->WriteStartType();

        if (NT_ERROR(status)) return status;

        status = stream->WriteDouble(this->_double);

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

        status = stream->ReadDouble(this->_double);

        if (NT_ERROR(status)) return status;

        return stream->ReadEndType();
    }
};

struct BadObjectV2 : public FabricSerializable
{
    double _double;
    bool _writeByteArray;
    KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> _byteBufferUPtr;

    BadObjectV2(bool writeByteArray = false)
        : _double(0)
        , _writeByteArray(writeByteArray)
    {
    }

    virtual NTSTATUS Write(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Write(stream);

        if (NT_ERROR(status)) return status;

        status = stream->WriteStartType();

        if (NT_ERROR(status)) return status;

        status = stream->WriteDouble(this->_double);

        if (NT_ERROR(status)) return status;

        if (this->_writeByteArray)
        {
            UCHAR * byteBuffer = _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultPagedKAllocator()) UCHAR[1024];

            if (byteBuffer == nullptr)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            this->_byteBufferUPtr = byteBuffer;
            FabricIOBuffer buffer;
            buffer.buffer = byteBuffer;
            buffer.length = 1024;

            status = stream->WriteByteArrayNoCopy(&buffer, 1, 0);
        }
        else
        {
            status = stream->WriteByteArrayNoCopy(nullptr, 0, 0);
        }

        if (NT_ERROR(status)) return status;

        return stream->WriteEndType();
    }

    virtual NTSTATUS Read(__in IFabricSerializableStream *)
    {
        // Not needed for this test
        return STATUS_CANCELLED;
    }
};

