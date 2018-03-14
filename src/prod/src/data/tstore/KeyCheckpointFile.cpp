// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define KEYCHECKPOINTFILE_TAG 'fpCK'


using namespace Data::TStore;
using namespace Data::Utilities;

KeyCheckpointFile::KeyCheckpointFile(
    __in KString& filename,
    __in ULONG32 fileId,
    __in KBlockFile& file,
    __in bool isValueAReferenceType,
    __in StoreTraceComponent & component) : 
    isValueAReferenceType_(isValueAReferenceType),
    filenameSPtr_(&filename),
    fileSPtr_(&file),
    traceComponent_(&component)
{
    StreamPool::StreamFactoryType fileStreamFactoryDelegate;
    fileStreamFactoryDelegate.Bind(this, &KeyCheckpointFile::CreateFileStreamAsync);
    NTSTATUS status = StreamPool::Create(fileStreamFactoryDelegate, GetThisAllocator(), streamPool_);
    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    status = KeyCheckpointFileProperties::Create(GetThisAllocator(), propertiesSPtr_);
    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    propertiesSPtr_->FileId = fileId;
}

//reserved for openAsync
KeyCheckpointFile::KeyCheckpointFile(
    __in KString& filename,
    __in KBlockFile& file,
    __in bool isValueAReferenceType,
    __in StoreTraceComponent & traceComponent) : 
    isValueAReferenceType_(isValueAReferenceType),
    filenameSPtr_(&filename),
    fileSPtr_(&file),
    traceComponent_(&traceComponent)
{
    StreamPool::StreamFactoryType fileStreamFactoryDelegate;
    fileStreamFactoryDelegate.Bind(this, &KeyCheckpointFile::CreateFileStreamAsync);
    NTSTATUS status = StreamPool::Create(fileStreamFactoryDelegate, GetThisAllocator(), streamPool_);
    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
        return;
    }

    status = KeyCheckpointFileProperties::Create(GetThisAllocator(), propertiesSPtr_);
    if (!NT_SUCCESS(status))
    {
        this->SetConstructorStatus(status);
    }
}

KeyCheckpointFile::~KeyCheckpointFile()
{
}

//reserved for openAsync
NTSTATUS
KeyCheckpointFile::Create(
    __in StoreTraceComponent & traceComponent,
    __in KString& filename,
    __in bool isValueAReferenceType,
    __in KBlockFile& file,
    __in KAllocator& allocator,
    __out KeyCheckpointFile::SPtr& result)
{
    NTSTATUS status;

    SPtr output = _new(KEYCHECKPOINTFILE_TAG, allocator) KeyCheckpointFile(filename, file, isValueAReferenceType, traceComponent);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

NTSTATUS
KeyCheckpointFile::Create(
    __in StoreTraceComponent & traceComponent,
    __in KString& filename,
    __in bool isValueAReferenceType,
    __in ULONG32 fileId,
    __in KBlockFile& file,
    __in KAllocator& allocator,
    __out KeyCheckpointFile::SPtr& result)
{
    NTSTATUS status;

    SPtr output = _new(KEYCHECKPOINTFILE_TAG, allocator) KeyCheckpointFile(filename, fileId, file, isValueAReferenceType, traceComponent);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

ktl::Awaitable<KeyCheckpointFile::SPtr> KeyCheckpointFile::CreateAsync(
    __in StoreTraceComponent & traceComponent,
    __in KString& filename,
    __in bool isValueAReferenceType,
    __in KAllocator& allocator)
{
    KBlockFile::SPtr fileSPtr = nullptr;
    KeyCheckpointFile::SPtr checkpointFileSPtr = nullptr;
    try
    {
        KString::SPtr filenameSPtr = &filename;
        StoreTraceComponent::SPtr traceComponentSPtr = &traceComponent;

        fileSPtr = co_await KeyCheckpointFile::CreateBlockFileAsync(*filenameSPtr, allocator, KBlockFile::CreateDisposition::eOpenExisting);
        NTSTATUS status = KeyCheckpointFile::Create(*traceComponentSPtr, filename, isValueAReferenceType, *fileSPtr, allocator, checkpointFileSPtr);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const &)
    {
        if (checkpointFileSPtr != nullptr)
        {
            checkpointFileSPtr = nullptr;
        }

        if (fileSPtr != nullptr)
        {
            fileSPtr->Close();
        }

        throw;
    }

    co_return checkpointFileSPtr;
}

ktl::Awaitable<KeyCheckpointFile::SPtr> KeyCheckpointFile::CreateAsync(
    __in StoreTraceComponent & traceComponent,
    __in KString& filename,
    __in bool isValueAReferenceType,
    __in ULONG32 fileId,
    __in KAllocator& allocator)
{
    KBlockFile::SPtr fileSPtr = nullptr;
    KeyCheckpointFile::SPtr checkpointFileSPtr = nullptr;
    try
    {
        KString::SPtr filenameSPtr = &filename;
        StoreTraceComponent::SPtr traceComponentSPtr = &traceComponent;

        fileSPtr = co_await KeyCheckpointFile::CreateBlockFileAsync(*filenameSPtr, allocator, KBlockFile::CreateDisposition::eOpenAlways);
        NTSTATUS status = KeyCheckpointFile::Create(*traceComponentSPtr, *filenameSPtr, isValueAReferenceType, fileId, *fileSPtr, allocator, checkpointFileSPtr);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const &)
    {
        if (checkpointFileSPtr != nullptr)
        {
            checkpointFileSPtr = nullptr;
        }

        if (fileSPtr != nullptr)
        {
            fileSPtr->Close();
        }

        throw;
    }

    co_return checkpointFileSPtr;
}

ktl::Awaitable<KSharedPtr<KeyCheckpointFile>> KeyCheckpointFile::OpenAsync(
    __in KAllocator& allocator,
    __in KString& filename,
    __in StoreTraceComponent & traceComponent,
    __in bool isValueReferenceType)
{
    StoreEventSource::Events->KeyCheckpointFileOpenAsync(traceComponent.PartitionId, traceComponent.TraceTag, ToStringLiteral(filename), L"opening");
    bool exists = Common::File::Exists(filename.operator LPCWSTR());
    ASSERT_IFNOT(exists, "{0}: Expected Key Checkpoint file {1} to exist", traceComponent.AssertTag, ToStringLiteral(filename));

    KSharedPtr<KeyCheckpointFile> checkpointFileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;

    StoreTraceComponent::SPtr traceComponentSPtr = &traceComponent;
    KString::SPtr filenameSPtr = &filename;

    try
    {
        checkpointFileSPtr = co_await KeyCheckpointFile::CreateAsync(*traceComponentSPtr, *filenameSPtr, isValueReferenceType, allocator);
        co_await checkpointFileSPtr->ReadMetadataAsync();
        StoreEventSource::Events->KeyCheckpointFileOpenAsync(traceComponentSPtr->PartitionId, traceComponentSPtr->TraceTag, ToStringLiteral(*filenameSPtr), L"complete");
        co_return checkpointFileSPtr;
    }
    catch (ktl::Exception const& e)
    {
        // Ensure the checkpoint file get disposed quickly if we get an exception.
        exception = SharedException::Create(e, allocator);
    }

    if (checkpointFileSPtr != nullptr)
    {
        co_await checkpointFileSPtr->CloseAsync();
    }

    //clang compiler error, needs to assign before throw.
    auto ex = exception->Info;
    throw ex;
}


ktl::Awaitable<void> KeyCheckpointFile::FlushAsync(
    __in ktl::io::KFileStream& fileStream, 
    __in SharedBinaryWriter& writer)
{
    // If there's any buffered keys in memory, flush them to disk first.
    ktl::io::KFileStream::SPtr fileStreamSPtr(&fileStream);
    SharedBinaryWriter::SPtr memoryBufferSPtr(&writer);

    ULONG64 size = fileStreamSPtr->GetPosition() + memoryBufferSPtr->Position;

    if (memoryBufferSPtr->Position > 0)
    {
        co_await FlushMemoryBufferAsync(*fileStreamSPtr, *memoryBufferSPtr);
    }

    // Update the Properties for its KeysHandle.
    BlockHandle::SPtr keysHandleSPtr = nullptr;
    NTSTATUS status = BlockHandle::Create(0, size, GetThisAllocator(), keysHandleSPtr);
    Diagnostics::Validate(status);
    propertiesSPtr_->KeysHandle = *keysHandleSPtr;

    // Write the Properties.
    BlockHandle::SPtr propertiesHandleSPtr = nullptr;
    FileBlock<KeyCheckpointFileProperties::SPtr>::SerializerFunc propfunc(propertiesSPtr_.RawPtr(), &KeyCheckpointFileProperties::Write);
    status = co_await FileBlock<KeyCheckpointFileProperties::SPtr>::WriteBlockAsync(*fileStreamSPtr, propfunc, GetThisAllocator(), ktl::CancellationToken::None, propertiesHandleSPtr);
    STORE_ASSERT(NT_SUCCESS(status), "Failed to write file block for key checkpoint file properties. Status: {1}", status);

    // Write the Footer.
    status = FileFooter::Create(*propertiesHandleSPtr, FileVersion, GetThisAllocator(), footerSPtr_);
    Diagnostics::Validate(status);

    BlockHandle::SPtr blockHandleSPtr = nullptr;
    FileBlock<FileFooter::SPtr>::SerializerFunc footerFunc(footerSPtr_.RawPtr(), &FileFooter::Write);
    status = co_await FileBlock<FileFooter::SPtr>::WriteBlockAsync(*fileStreamSPtr, footerFunc, GetThisAllocator(), ktl::CancellationToken::None, blockHandleSPtr);
    STORE_ASSERT(NT_SUCCESS(status), "Failed to write block for file footer. Status: {1}", status);

    // Finally, flush to disk.
    status = co_await fileStreamSPtr->FlushAsync();
    STORE_ASSERT(NT_SUCCESS(status), "Failed to write to disk. Status: {1}", status);
}


ktl::Awaitable<void> KeyCheckpointFile::FlushMemoryBufferAsync(
    __in ktl::io::KFileStream& stream,
    __in SharedBinaryWriter& writer)
{
    SharedBinaryWriter::SPtr memoryBufferSPtr(&writer);
    //// Checksum the chunk.
    //ulong checksum = memoryBuffer.GetChecksum();
    //memoryBuffer.Write(checksum);
    ktl::io::KFileStream::SPtr fileStreamSPtr(&stream);

    // Write to disk.
    KBuffer::SPtr bufferSPtr = memoryBufferSPtr->GetBuffer(0);
    ULONG size = bufferSPtr->QuerySize();
    NTSTATUS status = co_await fileStreamSPtr->WriteAsync(*bufferSPtr, 0, size);
    STORE_ASSERT(NT_SUCCESS(status), "Failed to write to file stream. Status: {1}", status);

    // Reset the memory buffer.
    memoryBufferSPtr->Position = 0;
}


ktl::Awaitable<void> KeyCheckpointFile::ReadMetadataAsync()
{
    ktl::io::KFileStream::SPtr filestreamSPtr= nullptr;
    KBlockFile::SPtr fileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;
    
    try
    {
        filestreamSPtr = co_await streamPool_->AcquireStreamAsync();

        LONGLONG size = filestreamSPtr->GetLength();
        const LONGLONG minimumSize = FileFooter::SerializedSize() + sizeof(ULONG64);
        STORE_ASSERT(size >= minimumSize, "Invalied KeyCheckpointFile size={1}", size);

        filestreamSPtr->SetPosition(size);

        // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
        BlockHandle::SPtr footerHandleSPtr = nullptr;
        ULONG64 offset = filestreamSPtr->GetPosition() - FileFooter::SerializedSize() - sizeof(ULONG64);
        NTSTATUS status = BlockHandle::Create(offset, FileFooter::SerializedSize(), GetThisAllocator(), footerHandleSPtr);
        Diagnostics::Validate(status);

        FileBlock<FileFooter::SPtr>::DeserializerFunc footerFunc(&FileFooter::Read);

        footerSPtr_ = co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(*filestreamSPtr, *footerHandleSPtr, footerFunc, GetThisAllocator(), ktl::CancellationToken::None);

        // Verify we know how to deserialize this version of the checkpoint file.
        if (footerSPtr_->Version != FileVersion)
        {
            throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }

        // Read and validate the Properties section.
        BlockHandle::SPtr propertiesHandleSPtr = footerSPtr_->PropertiesHandle;

        FileBlock<KeyCheckpointFileProperties::SPtr>::DeserializerFunc propFunc(&FilePropertySection::Read);

        propertiesSPtr_ = co_await FileBlock<KeyCheckpointFileProperties::SPtr>::ReadBlockAsync(
            *filestreamSPtr,
            *propertiesHandleSPtr,
            propFunc,
            GetThisAllocator(),
            ktl::CancellationToken::None);
    }
    catch (ktl::Exception const& e)
    {
        exception = SharedException::Create(e, GetThisAllocator());
    }

    if (filestreamSPtr != nullptr && filestreamSPtr->IsOpen())
    {
        co_await streamPool_->ReleaseStreamAsync(*filestreamSPtr);
        filestreamSPtr = nullptr;
    }

    if (fileSPtr != nullptr)
    {
        fileSPtr->Close();
    }

    if (exception != nullptr)
    {
        //clang compiler error, needs to assign before throw.
        auto ex = exception->Info;
        throw ex;
    }
}

//
// Create new filestream. This should be replaced by stream pool once have it.
//
ktl::Awaitable<ktl::io::KFileStream::SPtr> KeyCheckpointFile::CreateFileStreamAsync()
{
    ktl::io::KFileStream::SPtr filestreamSPtr = nullptr;
    NTSTATUS status = ktl::io::KFileStream::Create(filestreamSPtr, this->GetThisAllocator(), KEYCHECKPOINTFILE_TAG);
    Diagnostics::Validate(status);

    status = co_await filestreamSPtr->OpenAsync(*fileSPtr_);
    STORE_ASSERT(NT_SUCCESS(status), "Error opening filestream: {1}", status);

    co_return filestreamSPtr;
}

ktl::Awaitable<void> KeyCheckpointFile::CloseAsync()
{
    co_await streamPool_->CloseAsync();
    if (fileSPtr_ != nullptr)
    {
        fileSPtr_->Close();
    }

    co_return;
}

ktl::Awaitable<KBlockFile::SPtr> KeyCheckpointFile::CreateBlockFileAsync(
    __in KString& filename, 
    __in KAllocator& allocator, 
    __in KBlockFile::CreateDisposition createType)
{
    KWString filePath(allocator, filename);

    NTSTATUS status;

    KBlockFile::SPtr fileSPtr = nullptr;
    KBlockFile::CreateOptions createOptions = static_cast<KBlockFile::CreateOptions>(KBlockFile::CreateOptions::eShareRead | KBlockFile::CreateOptions::eShareWrite | KBlockFile::CreateOptions::eInheritFileSecurity);

    try
    {
        status = co_await KBlockFile::CreateSparseFileAsync(
            filePath,
            FALSE,
            createType,
            createOptions,
            fileSPtr,
            nullptr,
            allocator,
            KEYCHECKPOINTFILE_TAG);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const &)
    {
        if (fileSPtr != nullptr)
        {
            fileSPtr->Close();
        }

        throw;
    }

    co_return fileSPtr;
}


