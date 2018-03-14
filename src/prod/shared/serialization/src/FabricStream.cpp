// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <Serialization.h>

using namespace Serialization;


NTSTATUS FabricStream::Create(__out IFabricStream ** outPtr, ULONG maxGrowBy)
{
    FabricStream * ptr = _new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) FabricStream(maxGrowBy);
    *outPtr = nullptr;

    if (ptr == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = ptr->Status();

    if (NT_SUCCESS(status))
    {
        *outPtr = ptr;
    }
    else
    {
        _delete(ptr);
        ptr = nullptr;
    }

    return status;
}

NTSTATUS FabricStream::Create(__out IFabricStream ** outPtr, __in KMemChannel::UPtr && byteStream, __in KArray<FabricIOBuffer> && extensionBuffers)
{
    FabricStream * ptr = _new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) FabricStream(Ktl::Move(byteStream), Ktl::Move(extensionBuffers));
    *outPtr = nullptr;

    if (ptr == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = ptr->Status();

    if (NT_SUCCESS(status))
    {
        *outPtr = ptr;
    }
    else
    {
        _delete(ptr);
        ptr = nullptr;
    }

    return status;
}

FabricStream::FabricStream(ULONG maxGrowBy)
    : byteStream_()
    , regularDataCursor_(0)
    , extensionDataCursor_(0)
    , extensionBuffers_(FabricSerializationCommon::GetPagedKtlAllocator())
    , completionCallbacks_(FabricSerializationCommon::GetPagedKtlAllocator())
    , RefCount_(1)
    , cursorOnExtensionData_(false)
{
    this->SetConstructorStatus(this->extensionBuffers_.Status());
    this->SetConstructorStatus(this->completionCallbacks_.Status());

    if (NT_SUCCESS(this->Status()))
    {
        if (maxGrowBy == 0)
        {
            byteStream_ = KMemChannel::UPtr(_new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) KMemChannel(FabricSerializationCommon::GetPagedKtlAllocator(), 128));
        }
        else
        {
            byteStream_ = KMemChannel::UPtr(_new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) KMemChannel(FabricSerializationCommon::GetPagedKtlAllocator(), 128, 4096, maxGrowBy));
        }

        if (byteStream_)
        {
            this->SetConstructorStatus(byteStream_->Status());
        }
        else
        {
            this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
}

FabricStream::FabricStream(__in KMemChannel::UPtr && byteStream, __in KArray<FabricIOBuffer> && extensionBuffers)
    : byteStream_(Ktl::Move(byteStream))
    , regularDataCursor_(0)
    , extensionDataCursor_(0)
    , extensionBuffers_(Ktl::Move(extensionBuffers))
    , completionCallbacks_(FabricSerializationCommon::GetPagedKtlAllocator())
    , RefCount_(1)
    , cursorOnExtensionData_(false)
{
    this->SetConstructorStatus(this->extensionBuffers_.Status());
    this->SetConstructorStatus(this->completionCallbacks_.Status());

    if (NT_SUCCESS(this->Status()))
    {
        if (this->byteStream_)
        {
            this->SetConstructorStatus(this->byteStream_->Status());
        }
        else
        {
            this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
}

FabricStream::~FabricStream()
{
}

NTSTATUS FabricStream::QueryInterface(
    __in  GUID &,
    __out VOID **Interface
    )
{
    *Interface = nullptr;

    return STATUS_SUCCESS;
}

LONG FabricStream::AddRef()
{
    return InterlockedIncrement(&RefCount_);
}

LONG FabricStream::Release()
{
    LONG result = InterlockedDecrement(&RefCount_);

    ASSERT(result >= 0);

    if (result == 0)
    {
        _delete(this);
    }

    return result;
}

NTSTATUS FabricStream::WriteBytes(__in ULONG size, __in VOID const * source)
{       
    NTSTATUS status = this->byteStream_->Write(size, source);

    // TODO: if status failed, return converted error

    return status;
}

NTSTATUS FabricStream::WriteBytesNoCopy(__in ULONG size, __in VOID const * source)
{
    // TODO: add buffer to a karray

    // TODO: shouldnt be using const cast
    return this->extensionBuffers_.Append(FabricIOBuffer(static_cast<UCHAR*>(const_cast<VOID*>(source)), size));

    //return STATUS_SUCCESS;
}

NTSTATUS FabricStream::ReadBytes(__in ULONG size, __out VOID * destination)
{
    ULONG actuallyRead;
    NTSTATUS status = this->byteStream_->Read(size, destination, &actuallyRead);

    // TODO: if status failed, return converted error

    if (actuallyRead != size)
    {
        return K_STATUS_COULD_NOT_READ_STREAM;
        // TODO: stream not long enough, return error
    }

    return status;
}

NTSTATUS FabricStream::ReadBytesNoCopy(__in ULONG index, __out FabricIOBuffer & buffer)
{
    if (index < this->extensionBuffers_.Count())
    {
        buffer = this->extensionBuffers_[index];

        return STATUS_SUCCESS;
    }

    return K_STATUS_NO_MORE_EXTENSION_BUFFERS;
}

_Use_decl_annotations_
NTSTATUS FabricStream::GetAllBuffers(FabricIOBuffer* bufferArray, size_t& count) const
{
    if (count < (byteStream_->GetBlockCount() + extensionBuffers_.Count()))
    {
        count = byteStream_->GetBlockCount() + extensionBuffers_.Count();
        return K_STATUS_BUFFER_TOO_SMALL;
    }

    size_t outputIdx = 0;
    for (int i = 0; i < static_cast<int>(byteStream_->GetBlockCount()); ++i)
    {
        KMemRef memRef;
        NTSTATUS status = byteStream_->GetBlock(i, memRef);
        if (!NT_SUCCESS(status)) return status;

        bufferArray[outputIdx].buffer = (UCHAR*)(memRef._Address);
        bufferArray[outputIdx].length = memRef._Param;
        ++outputIdx;
    }

    for (int i = 0; i < static_cast<int>(extensionBuffers_.Count()); ++i)
    {
        bufferArray[outputIdx++] = extensionBuffers_[i];
    }

    count = outputIdx;
    return STATUS_SUCCESS;
}

NTSTATUS FabricStream::GetNextBuffer(__out FabricIOBuffer & buffer, __out bool & isExtensionData)
{
    buffer.Clear();

    if (!this->cursorOnExtensionData_)
    {
        KMemRef memoryReference;

        if (this->regularDataCursor_ < this->byteStream_->GetBlockCount())
        {
            NTSTATUS status = this->byteStream_->GetBlock(this->regularDataCursor_, memoryReference);

            ++this->regularDataCursor_;

            if (NT_SUCCESS(status))
            {
                buffer.buffer = static_cast<UCHAR*>(memoryReference._Address);
                buffer.length = memoryReference._Param;

                isExtensionData = false;
                return status;
            }
            else
            {
                return status;
            }
        }

        // done with regular buffers, return extension buffers
        this->cursorOnExtensionData_ = true;
    }

    if (this->extensionBuffers_.Count() > this->extensionDataCursor_)
    {
        isExtensionData = true;

        buffer = this->extensionBuffers_[this->extensionDataCursor_];
        ++this->extensionDataCursor_;

        return STATUS_SUCCESS;
    }

    return K_STATUS_NO_MORE_EXTENSION_BUFFERS;
}

NTSTATUS FabricStream::Seek(__in ULONG position)
{
    NTSTATUS status = this->byteStream_->SetCursor(position, KMemChannel::eFromBegin);

    // TODO: check and filter error

    return status;
}

NTSTATUS FabricStream::SeekToEnd()
{
    NTSTATUS status = this->byteStream_->SetCursor(0, KMemChannel::eBeforeEnd);
    // TODO: remove this, don't think it is needed

    return status;
}

ULONG FabricStream::get_Position() const
{
    return this->byteStream_->GetCursor();
}

ULONG FabricStream::Size() const
{
    return this->byteStream_->Size();
}

NTSTATUS FabricStream::AddCompletionCallback(__in FabricCompletionCallback completionCallback, __in VOID * state)
{
    _CallbackInfo info;
    info.callback_ = completionCallback;
    info.state_ = state;

    NTSTATUS status = this->completionCallbacks_.Append(info);

    // TODO: if failed return some error

    return status;
}

//NTSTATUS FabricStream::AddCompletionCallback(IFabricCompletionCallback const & completionCallback, void * state) const;

NTSTATUS FabricStream::InvokeCompletionCallbacks()
{
    ULONG callbackCount = this->completionCallbacks_.Count();

    for (ULONG i = 0; i < callbackCount; ++i)
    {
        _CallbackInfo info = this->completionCallbacks_[i];

        info.Invoke();
    }

    return STATUS_SUCCESS;
}
