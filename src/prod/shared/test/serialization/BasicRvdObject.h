// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct BasicRvdObject : public FabricSerializable, public KShared<BasicRvdObject>
{
    CHAR _char1;
    SHORT _short1;

    KWString _string;
    KArray<ULONG64> _array;
    KArray<FabricIOBuffer> _buffers;

    GUID _guid;

private:
    BasicRvdObject()
        : _string(Common::GetSFDefaultKtlSystem().NonPagedAllocator())
        , _array(Common::GetSFDefaultKtlSystem().NonPagedAllocator())
        , _buffers(Common::GetSFDefaultKtlSystem().NonPagedAllocator())
    {
        this->SetConstructorStatus(this->_array.Status());
        this->SetConstructorStatus(this->_buffers.Status());
    }

public:

    ~BasicRvdObject()
    {
        for (ULONG i = 0; i < this->_buffers.Count(); ++i)
        {
            FabricIOBuffer buffer = this->_buffers[i];

            _deleteArray<UCHAR>(buffer.buffer);
        }
    }

    static void BuffersWrittenCallback(VOID * state)
    {
        BasicRvdObject * object = reinterpret_cast<BasicRvdObject*>(state);

        if (object != nullptr)
        {
            object->Release();
        }
    }

    typedef KSharedPtr<BasicRvdObject> SPtr;

    static NTSTATUS Create(__out BasicRvdObject::SPtr & result)
    {
        result = _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultKtlSystem().NonPagedAllocator()) BasicRvdObject();

        if (result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NTSTATUS status = result->Status();
        if (!NT_SUCCESS(status))
        {
            result.Reset();
        }

        return status;
    }

    NTSTATUS AllocateLargeBuffer(ULONG size, UCHAR fill)
    {
        FabricIOBuffer buffer;

        UCHAR * bytes = new UCHAR[size];

        if (bytes == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        memset(bytes, fill, size);

        buffer.buffer = bytes;
        buffer.length = size;

        NTSTATUS status = this->_buffers.Append(buffer);

        if (NT_ERROR(status))
        {
            delete [] bytes;
        }

        return status;
    }

    static IFabricSerializable * Activator(FabricTypeInformation typeInformation);

    bool operator==(BasicRvdObject const & rhs)
    {
        bool equal = this->_short1 == rhs._short1
                  && this->_char1 == rhs._char1
                  && (this->_string.CompareTo(rhs._string) == 0)
                  && IsArrayEqual(this->_array, rhs._array);

        if (equal)
        {
            equal &= (memcmp(&this->_guid, &rhs._guid, sizeof(GUID)) == 0);
        }

        if (equal)
        {
            equal &= AreBuffersEqual(this->_buffers, rhs._buffers);
        }

        return equal;
    }

    virtual NTSTATUS GetTypeInformation(__out FabricTypeInformation & typeInformation) const
    {
        typeInformation.buffer = reinterpret_cast<UCHAR const *>(&BasicRvdObject::Id);
        typeInformation.length = sizeof(BasicRvdObject::Id);

        return STATUS_SUCCESS;
    }

    virtual NTSTATUS Write(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Write(stream);
        if (!NT_SUCCESS(status)) return status;

        KSerializationHelper streamHelper(stream);

        status = streamHelper.WriteStartType();
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Write(_char1);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Write(_short1);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Write(_string);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Write(_array);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Write(_guid);
        if (!NT_SUCCESS(status)) return status;

        status = stream->WriteByteArrayNoCopy(this->_buffers.Data(), this->_buffers.Count(), 0);
        if (!NT_SUCCESS(status)) return status;

        // TODO: We need to make sure that this callback is ALWAYS called
        this->AddRef();
        return streamHelper.WriteEndType(BasicRvdObject::BuffersWrittenCallback, this);
    }

    virtual NTSTATUS Read(__in IFabricSerializableStream * stream)
    {
        NTSTATUS status;

        status = __super::Read(stream);
        if (NT_ERROR(status)) return status;

        KSerializationHelper streamHelper(stream);

        status = streamHelper.ReadStartType();
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Read(_char1);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Read(_short1);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Read(_string);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Read(_array);
        if (!NT_SUCCESS(status)) return status;

        status = streamHelper.Read(_guid);
        if (!NT_SUCCESS(status)) return status;

        status = this->ReadByteArray(stream);
        if (!NT_SUCCESS(status)) return status;

        return stream->ReadEndType();
    }

    NTSTATUS ReadByteArray(__in IFabricSerializableStream * stream)
    {
        ULONG size = this->_buffers.Max();
        NTSTATUS status = stream->ReadByteArrayNoCopy(this->_buffers.Data(), size);

        if (K_STATUS_BUFFER_TOO_SMALL == status)
        {
            this->_buffers = KArray<FabricIOBuffer>(Common::GetSFDefaultKtlSystem().NonPagedAllocator(), size, size, 50);  // Create a new array with correct size
            status = this->_buffers.Status();

            if (NT_ERROR(status))
            {
                return status;
            }
        }
        else if (NT_SUCCESS(status))
        {
            BOOL setCountBool = FALSE;
            setCountBool = this->_buffers.SetCount(size);
            ASSERT(setCountBool == TRUE);

            return STATUS_SUCCESS;
        }
        else
        {
            return status;
        }

        size = this->_buffers.Max();

        status = stream->ReadByteArrayNoCopy(this->_buffers.Data(), size);

        if (NT_ERROR(status))
        {
            return status;
        }

        return status;
    }

    static const GUID Id;
};

// {AE05B0EA-ACC0-4D47-84A3-40B5E98D9A4A}
const GUID BasicRvdObject::Id = { 0xae05b0ea, 0xacc0, 0x4d47, { 0x84, 0xa3, 0x40, 0xb5, 0xe9, 0x8d, 0x9a, 0x4a } };

IFabricSerializable * BasicRvdObject::Activator(FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr || typeInformation.length != sizeof(GUID))
    {
        return nullptr;
    }

    GUID id = *(reinterpret_cast<GUID const *>(typeInformation.buffer));

    if (memcmp(&id, &BasicRvdObject::Id, sizeof(GUID)) == 0)
    {
        BasicRvdObject::SPtr object;
        if (NT_SUCCESS(BasicRvdObject::Create(object)))
        {
            return object.Detach();
        }
    }

    return nullptr;
}
