// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "KtlSF.Common.h"

namespace Common
{
    ErrorCode FabricSerializer::Serialize(Serialization::IFabricSerializable const * serializable, std::vector<byte> & bytes)
    {
        return ErrorCode::FromNtStatus(FabricSerializer::InnerSerialize(serializable, bytes));
    }

    ErrorCode FabricSerializer::Serialize(Serialization::IFabricSerializable const * serializable, Serialization::IFabricSerializableStreamUPtr & streamUPtr, ULONG maxGrowBy)
    {
        return ErrorCode::FromNtStatus(FabricSerializer::InnerSerialize(serializable, streamUPtr, maxGrowBy));
    }

    ErrorCode FabricSerializer::Deserialize(Serialization::IFabricSerializable & serializable, std::vector<byte> & bytes)
    {
        return ErrorCode::FromNtStatus(InnerDeserialize(serializable, static_cast<ULONG>(bytes.size()), bytes.data()));
    }

    ErrorCode FabricSerializer::Deserialize(Serialization::IFabricSerializable & serializable, ULONG size, void * buffer)
    {
        return ErrorCode::FromNtStatus(InnerDeserialize(serializable, size, buffer));
    }

    ErrorCode FabricSerializer::Deserialize(Serialization::IFabricSerializable & serializable, ULONG size, std::vector<Serialization::FabricIOBuffer> const & buffers)
    {
        return ErrorCode::FromNtStatus(InnerDeserialize(serializable, size, buffers)); 
    }

    ErrorCode FabricSerializer::GetOperationBuffersFromSerializableStream(Serialization::IFabricSerializableStreamUPtr const & streamUPtr, std::vector<FABRIC_OPERATION_DATA_BUFFER> & buffers, ULONG & totalSize)
    {
        return ErrorCode::FromNtStatus(InnerGetOperationBuffersFromSerializableStream(streamUPtr, buffers, totalSize));
    }

    NTSTATUS FabricSerializer::InnerGetOperationBuffersFromSerializableStream(Serialization::IFabricSerializableStreamUPtr const & streamUPtr, std::vector<FABRIC_OPERATION_DATA_BUFFER> & buffers, ULONG & totalSize)
    {
        Serialization::FabricIOBuffer buffer;
        bool isExtensionBuffer;

        NTSTATUS status = streamUPtr->GetNextBuffer(buffer, isExtensionBuffer);

        while (NT_SUCCESS(status))
        {
            if (isExtensionBuffer)
            {
                Common::Assert::TestAssert("Extension buffers not supported yet in fabric user mode"); 
                return K_STATUS_OTHER_SERIALIZATION_ERRORS;
            }

            FABRIC_OPERATION_DATA_BUFFER operationBuffer;
            operationBuffer.Buffer = static_cast<BYTE*>(buffer.buffer);
            operationBuffer.BufferSize = buffer.length;

            buffers.push_back(operationBuffer);
            totalSize += buffer.length;

            status = streamUPtr->GetNextBuffer(buffer, isExtensionBuffer);
        }

        if (status != (NTSTATUS)K_STATUS_NO_MORE_EXTENSION_BUFFERS)
        {
            Common::Assert::TestAssert("GetNextBuffer returned an error other than K_STATUS_NO_MORE_EXTENSION_BUFFERS: {0}", status);
            return status;                
        }

        if(*(buffers[0].Buffer) != Serialization::FabricSerializationTypes::Object)
        {
            Common::Assert::TestAssert("Extension buffers not supported yet in fabric user mode");
            return K_STATUS_OTHER_SERIALIZATION_ERRORS;
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS FabricSerializer::InnerSerialize(Serialization::IFabricSerializable const * serializable, Serialization::IFabricSerializableStreamUPtr & streamUPtr, ULONG maxGrowBy)
    {
        if(serializable == nullptr)
        {
            Common::Assert::TestAssert("serializable can't be null");
            return K_STATUS_NULL_REF_POINTER;
        }

        Serialization::IFabricStream * innerStream = nullptr;
        NTSTATUS status = Serialization::FabricStream::Create(&innerStream, maxGrowBy);

        if (NT_ERROR(status) || innerStream == nullptr)
        {
            Common::Assert::TestAssert("Insufficient resources constructing an inner stream");
            return NT_ERROR(status)?status:K_STATUS_NULL_REF_POINTER;
        }

        Serialization::IFabricSerializableStream * stream = nullptr;
        status = Serialization::FabricSerializableStream::Create(&stream, Serialization::IFabricStreamUPtr(innerStream));
        Serialization::IFabricSerializableStreamUPtr streamInternalUPtr(stream);

        if (NT_ERROR(status) || stream == nullptr)
        {
            Common::Assert::TestAssert("Insufficient resources constructing a stream");
            return NT_ERROR(status)?status:K_STATUS_NULL_REF_POINTER;
        }

        status = streamInternalUPtr->WriteSerializable(const_cast<Serialization::IFabricSerializable*>(serializable));

        if (NT_ERROR(status))
        {
            Common::Assert::TestAssert("Writing object to stream failed: {0}", status);
            return status;
        }

        streamUPtr = Ktl::Move(streamInternalUPtr);

        return STATUS_SUCCESS;
    }

    ErrorCode FabricSerializer::Serialize(Serialization::IFabricSerializable const * serializable, KBuffer::SPtr & bytes)
    {
        NTSTATUS status = KBuffer::Create(0, bytes, GetSFDefaultPagedKAllocator());
        if (NT_ERROR(status))
        {
            return ErrorCode::FromNtStatus(status);
        }

        return ErrorCode::FromNtStatus(Serialize(serializable, *bytes));
    }

    Serialization::IFabricSerializableStreamUPtr FabricSerializer::CreateSerializableStream()
    {
        Serialization::IFabricSerializableStream* stream = nullptr;
        NTSTATUS status = Serialization::FabricSerializableStream::Create(&stream);
        Serialization::IFabricSerializableStreamUPtr streamUPtr(stream);

        // todo, consider setting failure status on message instead of assert
        ASSERT_IF(
            (NT_ERROR(status) || stream == nullptr),
            "Failed to construct a stream: status = {0:x}",
            status);
        return streamUPtr;
    }

    NTSTATUS FabricSerializer::CreateKBufferFromStream(Serialization::IFabricSerializableStream const& stream, KBuffer::SPtr& buffer)
    {
        Serialization::FabricIOBuffer bufferListOnStack[16];
        Serialization::FabricIOBuffer* bufferList = bufferListOnStack;

        size_t count = sizeof(bufferListOnStack) / sizeof(Serialization::FabricIOBuffer);
        auto status = stream.GetAllBuffers(bufferList, count);
        if (!NT_SUCCESS(status))
        {
            Invariant(status == K_STATUS_BUFFER_TOO_SMALL);
            auto bufferListInHeap = std::make_unique<Serialization::FabricIOBuffer[]>(count);
            bufferList = bufferListInHeap.get();

            status = stream.GetAllBuffers(bufferList, count);
            Invariant(NT_SUCCESS(status));
        }

        status = KBuffer::Create(stream.Size(), buffer, Serialization::FabricSerializationCommon::GetPagedKtlAllocator());
        if (!NT_SUCCESS(status)) return status;

        byte* pointer = (byte*)buffer->GetBuffer();
        size_t size = 0;
        for (uint i = 0; i < count; ++i)
        {
            KMemCpySafe(pointer + size, buffer->QuerySize() - size, bufferList[i].buffer, bufferList[i].length);
            size += bufferList[i].length;
        }

        return status;
    }

    NTSTATUS FabricSerializer::Serialize(Serialization::IFabricSerializable const * serializable, KBuffer & bytes)
    {
        Serialization::IFabricSerializableStreamUPtr streamUPtr;
        NTSTATUS status = InnerSerialize(serializable, streamUPtr);
        if (NT_ERROR(status))
        {
            return status;
        }

        status = bytes.SetSize(streamUPtr->Size());
        if (NT_ERROR(status))
        {
            return status;
        }

        byte* pointer = (byte*)bytes.GetBuffer();
        ULONG size = 0;

        Serialization::FabricIOBuffer buffer;
        bool isExtensionBuffer;
        while (NT_SUCCESS(status = streamUPtr->GetNextBuffer(buffer, isExtensionBuffer)))
        {
            if (isExtensionBuffer)
            {
                Common::Assert::TestAssert("Extension buffers not supported yet in fabric user mode"); 
                return K_STATUS_OTHER_SERIALIZATION_ERRORS;
            }         

            KMemCpySafe(pointer + size, bytes.QuerySize() - size, buffer.buffer, buffer.length);
            size += buffer.length;
        }

        if (status != (NTSTATUS)K_STATUS_NO_MORE_EXTENSION_BUFFERS)
        {
            Common::Assert::TestAssert("GetNextBuffer returned an error other than K_STATUS_NO_MORE_EXTENSION_BUFFERS: {0}", status);
            return status;
        }

        byte* bytePtr = (byte*)bytes.GetBuffer();
        if(bytePtr[0] != Serialization::FabricSerializationTypes::Object)
        {
            Common::Assert::TestAssert("Extension buffers not supported yet in fabric user mode");
            return K_STATUS_OTHER_SERIALIZATION_ERRORS;
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS FabricSerializer::InnerSerialize(Serialization::IFabricSerializable const * serializable, std::vector<byte> & bytes)
    {
        Serialization::IFabricSerializableStreamUPtr streamUPtr;
        NTSTATUS status = InnerSerialize(serializable, streamUPtr);
        if (NT_ERROR(status))
        {
            return status;
        }

        bytes.resize(streamUPtr->Size());
        byte * pointer = bytes.data();
        ULONG size = 0;

        Serialization::FabricIOBuffer buffer;
        bool isExtensionBuffer;
        while (NT_SUCCESS(status = streamUPtr->GetNextBuffer(buffer, isExtensionBuffer)))
        {
            if (isExtensionBuffer)
            {
                Common::Assert::TestAssert("Extension buffers not supported yet in fabric user mode"); 
                return K_STATUS_OTHER_SERIALIZATION_ERRORS;
            }         

            KMemCpySafe(pointer + size, bytes.size() - size, buffer.buffer, buffer.length);
            size += buffer.length;
        }

        if (status != (NTSTATUS)K_STATUS_NO_MORE_EXTENSION_BUFFERS)
        {
            Common::Assert::TestAssert("GetNextBuffer returned an error other than K_STATUS_NO_MORE_EXTENSION_BUFFERS: {0}", status);
            return status;                
        }

        if(bytes[0] != Serialization::FabricSerializationTypes::Object)
        {
            Common::Assert::TestAssert("Extension buffers not supported yet in fabric user mode");
            return K_STATUS_OTHER_SERIALIZATION_ERRORS;
        }

        return STATUS_SUCCESS;
    }

    NTSTATUS FabricSerializer::InnerDeserialize(Serialization::IFabricSerializable & serializable, ULONG size, void * buffer)
    {
        Serialization::FabricIOBuffer ioBuffer;
        ioBuffer.buffer = static_cast<UCHAR*>(buffer);
        ioBuffer.length = size;
            
        std::vector<Serialization::FabricIOBuffer> ioBuffers;
        ioBuffers.push_back(ioBuffer);

        return InnerDeserialize(serializable, size, ioBuffers);
    }

    NTSTATUS FabricSerializer::InnerDeserialize(Serialization::IFabricSerializable & serializable, ULONG size, std::vector<Serialization::FabricIOBuffer> const & buffers)
    {
        KMemChannel::UPtr memoryStream = KMemChannel::UPtr(_new (WF_SERIALIZATION_TAG, GetSFDefaultNonPagedKAllocator()) KMemChannel(GetSFDefaultNonPagedKAllocator(), static_cast<ULONG>(size)));

        if (!memoryStream)
        {
            Common::Assert::TestAssert("Allocation of KMemChannel failed");
            return K_STATUS_NULL_REF_POINTER;                
        }

        NTSTATUS status = memoryStream->Status();

        if (NT_ERROR(status))
        {
            Common::Assert::TestAssert("KMemChannel constructor failed: {0}", status);
            return status;                
        }

        // Tell the stream not to delete what it is looking at
        memoryStream->DetachOnDestruct();

        status = memoryStream->StartCapture();

        if (NT_ERROR(status))
        {
            Common::Assert::TestAssert("StartCapture failed: {0}", status);
            return status;
        }

        for (Serialization::FabricIOBuffer const & buffer : buffers)
        {
            KMemRef memoryReference(buffer.length, buffer.buffer);
            memoryReference._Param = buffer.length;
                
            status = memoryStream->Capture(memoryReference);

            if (NT_ERROR(status))
            {
                Common::Assert::TestAssert("Capture failed: {0}", status);
                return status;                
            }
        }

        status = memoryStream->EndCapture();

        if (NT_ERROR(status))
        {
            Common::Assert::TestAssert("EndCapture failed: {0}", status);
            return status;                
        }

        KArray<Serialization::FabricIOBuffer> receivingExtensionBuffers(GetSFDefaultNonPagedKAllocator());

        status = receivingExtensionBuffers.Status();

        if (NT_ERROR(status))
        {
            Common::Assert::TestAssert("KArray constructor failed: {0}", status);
            return status;                
        }

        Serialization::IFabricStream * receiveByteStream;

        status = Serialization::FabricStream::Create(&receiveByteStream, Ktl::Move(memoryStream), Ktl::Move(receivingExtensionBuffers));

        Serialization::IFabricStreamUPtr receiveByteStreamUPtr(receiveByteStream);

        if (NT_ERROR(status) || receiveByteStream == nullptr)
        {
            Common::Assert::TestAssert("Allocation of IFabricStream failed");
            return NT_ERROR(status)?status:K_STATUS_NULL_REF_POINTER;                
        }

        Serialization::IFabricSerializableStream * readStream;
        status = Serialization::FabricSerializableStream::Create(&readStream, Ktl::Move(receiveByteStreamUPtr));

        Serialization::IFabricSerializableStreamUPtr readStreamUPtr(readStream);

        if (NT_ERROR(status) || readStream == nullptr)
        {
            Common::Assert::TestAssert("Allocation of FabricSerializableStream failed: {0}", status);
            return NT_ERROR(status)?status:K_STATUS_NULL_REF_POINTER;
        }

        status = readStreamUPtr->ReadSerializable(&serializable);

        if (NT_ERROR(status))
        {
            Common::Assert::TestAssert("Deserialization failed: {0}", status);
        }            

        return status;
    }
}
