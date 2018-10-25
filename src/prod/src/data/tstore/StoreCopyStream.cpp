// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;
using namespace Common;

ULONG32 StoreCopyStream::CopyChunkSize = 500 * 1024;

NTSTATUS StoreCopyStream::Create(
    __in IStoreCopyProvider & copyProvider,
    __in StoreTraceComponent & traceComponent,
    __in KAllocator & allocator,
    __in StorePerformanceCountersSPtr & perfCounters,
    __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(STORE_COPY_STREAM_TAG, allocator) StoreCopyStream(
        copyProvider,
        traceComponent,
        perfCounters);

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

StoreCopyStream::StoreCopyStream(
    __in IStoreCopyProvider & copyProvider,
    __in StoreTraceComponent & traceComponent,
    __in StorePerformanceCountersSPtr & perfCounters) :
    copyProviderSPtr_(&copyProvider),
    copyStage_(CopyStage::Enum::Version),
    snapshotOfMetadataTableSPtr_(nullptr),
    snapshotOfMetadataTableEnumeratorSPtr_(nullptr),
    currentFileStreamSPtr_(nullptr),
    copyDataBufferSPtr_(nullptr),
    isClosed_(false),
    traceComponent_(&traceComponent),
    perfCounterWriter_(perfCounters)
{
    ULONG bufferSize = CopyChunkSize + sizeof(ULONG32) + 1;
    NTSTATUS status = KBuffer::Create(bufferSize, copyDataBufferSPtr_, this->GetThisAllocator());
    Diagnostics::Validate(status);
}

StoreCopyStream::~StoreCopyStream()
{
}

ktl::Awaitable<NTSTATUS> StoreCopyStream::GetNextAsync(
    __in CancellationToken const & token,
    __out OperationData::CSPtr & result) noexcept
{
    result = nullptr;
    NTSTATUS status = STATUS_SUCCESS;

    try 
    {
        // Take a snapshot of the metadata table on first Copy operation.
        if (snapshotOfMetadataTableSPtr_ == nullptr)
        {
            snapshotOfMetadataTableSPtr_ = co_await copyProviderSPtr_->GetMetadataTableAsync();
            STORE_ASSERT(snapshotOfMetadataTableSPtr_ != nullptr, "IStoreCopyProvider.CurrentMetadataTableSPtr returned a null metadata table");
            snapshotOfMetadataTableEnumeratorSPtr_ = snapshotOfMetadataTableSPtr_->Table->GetEnumerator();
        }

        switch (copyStage_)
        {
        case CopyStage::Enum::Version:
        {
            result = co_await OnCopyStageVersionAsync();
            break;
        }
        case CopyStage::Enum::MetadataTable:
        {
            result = co_await OnCopyStageMetadataTableAsync();
            break;
        }
        case CopyStage::Enum::KeyFile:
        {
            result = co_await OnCopyStageKeyFileAsync();
            break;
        }
        case CopyStage::Enum::ValueFile:
        {
            result = co_await OnCopyStageValueFileAsync();
            break;
        }
        case CopyStage::Enum::Complete:
        {
            result = co_await OnCopyStageCompleteAsync();
            break;
        }
        case CopyStage::Enum::None:
        {
            // Finished copying. Dispose immediately
            co_await CloseAsync();
            break;
        }
        default: break;
        }
    }
    catch (ktl::Exception const & e)
    {
        status = e.GetStatus();
    }
    
    co_return status;
}

ktl::Awaitable<void> StoreCopyStream::CloseAsync()
{
    KShared$ApiEntry();

    try
    {
        if (isClosed_)
        {
            co_return;
        }

        copyProviderSPtr_ = nullptr;
        if (currentFileStreamSPtr_ != nullptr)
        {
            NTSTATUS status = co_await currentFileStreamSPtr_->CloseAsync();
            Diagnostics::Validate(status);
            currentFileStreamSPtr_ = nullptr;
        }

        if (currentFileSPtr_ != nullptr)
        {
            currentFileSPtr_->Close();
        }

        snapshotOfMetadataTableEnumeratorSPtr_ = nullptr;

        if (snapshotOfMetadataTableSPtr_ != nullptr)
        {
            co_await snapshotOfMetadataTableSPtr_->ReleaseReferenceAsync();
            snapshotOfMetadataTableSPtr_ = nullptr;
        }

        isClosed_ = true;

        copyStage_ = CopyStage::Enum::None;

        co_return;
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"CloseAsync", e);

        KDynStringA stackString(this->GetThisAllocator());
        Diagnostics::GetExceptionStackTrace(e, stackString);
        STORE_ASSERT(
            false,
            "UnexpectedException: StoreCopyStream.CloseAsync Code:{1}\nStack: {2}",
            ToStringLiteral(stackString),
            e.GetStatus());
    }
}

void StoreCopyStream::Dispose()
{
    copyProviderSPtr_ = nullptr;
    // TODO: This method should be async once SM fixes the interface to call async dispose or close
    // TODO: Once async, co_await CloseAsync
    ktl::Awaitable<void> closeAwaitable = CloseAsync();
    ktl::Task closeTask = ToTask(closeAwaitable);
    STORE_ASSERT(closeTask.IsTaskStarted(), "Expected store copy stream close task to start");
}

ktl::Awaitable<OperationData::CSPtr> StoreCopyStream::OnCopyStageVersionAsync()
{
    SharedException::CSPtr exceptionCSPtr = nullptr;
    try
    {
        StoreEventSource::Events->StoreCopyStreamCopyStarting(traceComponent_->PartitionId, traceComponent_->TraceTag, ToStringLiteral(*copyProviderSPtr_->WorkingDirectoryCSPtr));

        // Next copy stage.
        copyStage_ = CopyStage::Enum::MetadataTable;

        BinaryWriter writer(GetThisAllocator());

        ULONG32 CopyProtocolVersion = CopyManager::CopyProtocolVersion;
        byte CopyOperationVersion = StoreCopyOperation::Enum::Version;

        // Write the copy protocol version number
        writer.Write(CopyProtocolVersion);

        // Write a byte indicating the operation type is the copy protocol version
        writer.Write(CopyOperationVersion);

        StoreEventSource::Events->StoreCopyStreamCopyStageVersion(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            CopyProtocolVersion,
            writer.Position);

        OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
        resultSPtr->Append(*writer.GetBuffer(0));

        OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
        co_return resultCSPtr;
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"OnCopyStageVersionAsync", e);
        // Defer exception in order to close async
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<OperationData::CSPtr> StoreCopyStream::OnCopyStageMetadataTableAsync()
{
    SharedException::CSPtr exceptionCSPtr = nullptr;
    try
    {
        // Consistency checks.
        STORE_ASSERT(snapshotOfMetadataTableSPtr_ != nullptr, "Unexpected copy error. Master table to be copied is null.");

        // Next copy stage
        if (snapshotOfMetadataTableEnumeratorSPtr_->MoveNext())
        {
            copyStage_ = CopyStage::Enum::KeyFile;
        }
        else
        {
            copyStage_ = CopyStage::Enum::Complete;
        }

        MemoryBuffer::SPtr memoryStream = nullptr;
        auto status = MemoryBuffer::Create(GetThisAllocator(), memoryStream);
        Diagnostics::Validate(status);

        // Write the full metadata table (this will typically be small - even with 1000 tracked files, this will be under 64 KB).
        co_await MetadataManager::WriteAsync(*snapshotOfMetadataTableSPtr_, *memoryStream, GetThisAllocator());

        // Write a byte indicating the operation type is the full metadata table.
        co_await memoryStream->WriteAsync(static_cast<byte>(StoreCopyOperation::Enum::MetadataTable));

        StoreEventSource::Events->StoreCopyStreamCopyStageMetadataTable(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(*copyProviderSPtr_->WorkingDirectoryCSPtr),
            memoryStream->Position);

        OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
        resultSPtr->Append(*memoryStream->GetBuffer());

        OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
        co_return resultCSPtr;
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"OnCopyStageMetadataTable", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<OperationData::CSPtr> StoreCopyStream::OnCopyStageKeyFileAsync()
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try 
    {
        auto shortFileName = snapshotOfMetadataTableEnumeratorSPtr_->Current().Value->FileName;
        auto keyCheckpointFilePath = GetKeyCheckpointFilePath(*shortFileName);

        byte copyOperationStartKeyFile = StoreCopyOperation::Enum::StartKeyFile;
        byte copyOperationWriteKeyFile = StoreCopyOperation::Enum::WriteKeyFile;
        byte copyOperationEndKeyFile = StoreCopyOperation::Enum::EndKeyFile;

        bool completed = false;
        auto operationData = co_await CreateCheckpointFileChunkOperationData(*keyCheckpointFilePath, copyOperationStartKeyFile, copyOperationWriteKeyFile, copyOperationEndKeyFile, completed);

        // CreateCheckpointFileChunkOperationData will set completed to true if there is no more data to transmit
        if (completed)
        {
            copyStage_ = CopyStage::Enum::ValueFile;
        }

        co_return operationData;
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"OnCopyStageKeyFileAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<OperationData::CSPtr> StoreCopyStream::OnCopyStageValueFileAsync()
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try {
        auto shortFileName = snapshotOfMetadataTableEnumeratorSPtr_->Current().Value->FileName;
        auto valueCheckpointFilePath = GetValueCheckpointFilePath(*shortFileName);

        byte copyOperationStartValueFile = StoreCopyOperation::Enum::StartValueFile;
        byte copyOperationWriteValueFile = StoreCopyOperation::Enum::WriteValueFile;
        byte copyOperationEndValueFile = StoreCopyOperation::Enum::EndValueFile;

        bool completed = false;
        auto operationData = co_await CreateCheckpointFileChunkOperationData(*valueCheckpointFilePath, copyOperationStartValueFile, copyOperationWriteValueFile, copyOperationEndValueFile, completed);

        // CreateCheckpointFileChunkOperationData will set completed to true if there is no more data to transmit
        if (completed)
        {
            // Check if the are more files
            if (snapshotOfMetadataTableEnumeratorSPtr_->MoveNext())
            {
                // More files
                copyStage_ = CopyStage::Enum::KeyFile;
            }
            else
            {
                copyStage_ = CopyStage::Enum::Complete;
            }
        }

        co_return operationData;
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"OnCopyStageValueFileAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<OperationData::CSPtr> StoreCopyStream::OnCopyStageCompleteAsync()
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    perfCounterWriter_.UpdatePerformanceCounter();

    try
    {
        // Next copy stage.
        copyStage_ = CopyStage::Enum::None;

        // Indicate the copy operation is complete
        // TODO: Trace
        KBuffer::SPtr operationDataBufferSPtr;
        auto status = KBuffer::Create(sizeof(byte), operationDataBufferSPtr, GetThisAllocator());
        Diagnostics::Validate(status);

        byte* data = static_cast<byte *>(operationDataBufferSPtr->GetBuffer());
        *data = StoreCopyOperation::Enum::Complete;

        StoreEventSource::Events->StoreCopyStreamCopyStageCompleted(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(*copyProviderSPtr_->WorkingDirectoryCSPtr),
            perfCounterWriter_.AvgDiskTransferBytesPerSec);

        // Send the end of file operation data
        OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
        resultSPtr->Append(*operationDataBufferSPtr);

        OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
        co_return resultCSPtr;
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"OnCopyStageCompleteAsyc", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

KString::SPtr StoreCopyStream::CombineWithWorkingDirectoryPath(__in KStringView & filename)
{
    KString::SPtr filepath;
    auto status = KString::Create(filepath, this->GetThisAllocator(), L"");
    Diagnostics::Validate(status);
    bool result = filepath->Concat(*copyProviderSPtr_->WorkingDirectoryCSPtr);
    STORE_ASSERT(result, "Unable to concat path string");
    result = filepath->Concat(Common::Path::GetPathSeparatorWstr().c_str());
    STORE_ASSERT(result, "Unable to concat path string");
    result = filepath->Concat(filename);
    STORE_ASSERT(result, "Unable to concat path string");

    return filepath;
}

KString::SPtr StoreCopyStream::GetKeyCheckpointFilePath(__in KStringView & filename)
{
    KString::SPtr filepath = CombineWithWorkingDirectoryPath(filename);
    KStringView extension = KeyCheckpointFile::GetFileExtension();
    bool result = filepath->Concat(extension);
    STORE_ASSERT(result, "Unable to concat path string");
    return filepath;
}

KString::SPtr StoreCopyStream::GetValueCheckpointFilePath(__in KStringView & filename)
{
    KString::SPtr filepath = CombineWithWorkingDirectoryPath(filename);
    KStringView extension = ValueCheckpointFile::GetFileExtension();
    bool result = filepath->Concat(extension);
    STORE_ASSERT(result, "Unable to concat path string");
    return filepath;
}

ktl::Awaitable<KBlockFile::SPtr> StoreCopyStream::OpenFileAsync(__in KStringView & filename)
{
    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    KWString pathWString(GetThisAllocator(), filename);
    KBlockFile::SPtr blockFileSPtr;
    
    auto createOptions = KBlockFile::CreateOptions::eShareRead | KBlockFile::CreateOptions::eShareWrite | KBlockFile::CreateOptions::eInheritFileSecurity;

    try
    {
        auto status = co_await KBlockFile::CreateSparseFileAsync(
            pathWString,
            TRUE,
            KBlockFile::CreateDisposition::eOpenExisting,
            static_cast<KBlockFile::CreateOptions>(createOptions),
            blockFileSPtr,
            nullptr,
            GetThisAllocator(),
            STORE_COPY_STREAM_TAG);

        STORE_ASSERT(NT_SUCCESS(status), "Unable to open file {1}", filename.operator LPCWSTR());
    }
    catch (ktl::Exception const &)
    {
        if (blockFileSPtr != nullptr)
        {
            blockFileSPtr->Close();
        }

        throw;
    }

    co_return blockFileSPtr;
}

ktl::Awaitable<OperationData::CSPtr> StoreCopyStream::CreateCheckpointFileChunkOperationData(
    __in KStringView & filename,
    __in byte startMarker,
    __in byte writeMarker,
    __in byte endMarker,
    __out bool & completed)
{
    KString::SPtr filenameSPtr = nullptr;
    KString::Create(filenameSPtr, GetThisAllocator(), filename);

    ULONG bytesRead = 0;

    completed = false;

    // If we don't have the current file stream opened, this is the first table chunk.
    if (currentFileStreamSPtr_ == nullptr)
    {
        STORE_ASSERT(File::Exists(filenameSPtr->operator LPCWSTR()), "Unexpected copy error. Expected file {1} does not exist", filenameSPtr->operator LPCWSTR());

        StoreEventSource::Events->StoreCopyStreamCopyStageCheckpointChunkOpen(traceComponent_->PartitionId, traceComponent_->TraceTag, ToStringLiteral(*filenameSPtr));

        currentFileSPtr_ = co_await OpenFileAsync(*filenameSPtr);
        ktl::io::KFileStream::Create(currentFileStreamSPtr_, GetThisAllocator());
        NTSTATUS status = co_await currentFileStreamSPtr_->OpenAsync(*currentFileSPtr_);
        STORE_ASSERT(NT_SUCCESS(status), "Unable to open file stream for file {1}", filename.operator LPCWSTR());

        // Send the start of file operation data.
        perfCounterWriter_.StartMeasurement();
        status = co_await currentFileStreamSPtr_->ReadAsync(*copyDataBufferSPtr_, bytesRead, 0, CopyChunkSize);
        perfCounterWriter_.StopMeasurement(bytesRead);
        STORE_ASSERT(NT_SUCCESS(status), "Unable to read start of file stream for file {1}", filenameSPtr->operator LPCWSTR());

        MemoryBuffer::SPtr streamSPtr = nullptr;
        status = MemoryBuffer::Create(*copyDataBufferSPtr_, GetThisAllocator(), streamSPtr);
        Diagnostics::Validate(status);
        streamSPtr->Position = bytesRead;

        auto filemetaDataSPtr = snapshotOfMetadataTableEnumeratorSPtr_->Current().Value;
        status = co_await streamSPtr->WriteAsync(filemetaDataSPtr->FileId);
        STORE_ASSERT(NT_SUCCESS(status), "Unable to write file ID {1} for file {2} to in-memory stream", filemetaDataSPtr->FileId, filenameSPtr->operator LPCWSTR());

        status = co_await streamSPtr->WriteAsync(startMarker);
        STORE_ASSERT(NT_SUCCESS(status), "Unable to write start marker {1} for file {2} to in-memory stream", startMarker, filenameSPtr->operator LPCWSTR());

        StoreEventSource::Events->StoreCopyStreamCopyStageCheckpointChunkStart(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(*filenameSPtr),
            startMarker,
            bytesRead + sizeof(ULONG32) + 1,
            filemetaDataSPtr->FileId);

        KBuffer::SPtr operationDataBufferSPtr;
        status = KBuffer::CreateOrCopyFrom(operationDataBufferSPtr, *copyDataBufferSPtr_, 0, bytesRead + sizeof(ULONG32) + 1, GetThisAllocator());
        Diagnostics::Validate(status);

        OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
        resultSPtr->Append(*operationDataBufferSPtr);

        OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
        co_return resultCSPtr;
    }

    // The start of the current file has been sent. Check if there are more chunks to be sent (if the stream is at the end, this will return zero).
    perfCounterWriter_.StartMeasurement();
    auto status = co_await currentFileStreamSPtr_->ReadAsync(*copyDataBufferSPtr_, bytesRead, 0, CopyChunkSize);
    perfCounterWriter_.StopMeasurement(bytesRead);
    STORE_ASSERT(NT_SUCCESS(status), "Unable to read chunk of file stream for file {1}", filenameSPtr->operator LPCWSTR());

    if (bytesRead > 0)
    {
        // Send the partial table file operation data
        byte* data = static_cast<byte *>(copyDataBufferSPtr_->GetBuffer());
        data[bytesRead] = writeMarker;

        StoreEventSource::Events->StoreCopyStreamCopyStageCheckpointChunkWrite(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(*filenameSPtr),
            writeMarker,
            bytesRead + 1);

        KBuffer::SPtr operationDataBufferSPtr;
        auto lStatus = KBuffer::CreateOrCopyFrom(operationDataBufferSPtr, *copyDataBufferSPtr_, 0, bytesRead + 1, GetThisAllocator());
        Diagnostics::Validate(lStatus);

        OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
        resultSPtr->Append(*operationDataBufferSPtr);

        OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
        co_return resultCSPtr;
    }

    // There is no more data in the current file. Send the end of file marker
    status = co_await currentFileStreamSPtr_->CloseAsync();
    Diagnostics::Validate(status);
    currentFileStreamSPtr_ = nullptr;
    currentFileSPtr_->Close();
    currentFileSPtr_ = nullptr;
    completed = true;

    KBuffer::SPtr operationDataBufferSPtr;
    status = KBuffer::Create(sizeof(byte), operationDataBufferSPtr, GetThisAllocator());
    Diagnostics::Validate(status);

    byte* data = static_cast<byte *>(operationDataBufferSPtr->GetBuffer());
    *data = endMarker;

    StoreEventSource::Events->StoreCopyStreamCopyStageCheckpointChunkEnd(
        traceComponent_->PartitionId,
        traceComponent_->TraceTag,
        ToStringLiteral(*filenameSPtr),
        endMarker);
     
    // Send the end of file operation data
    OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
    resultSPtr->Append(*operationDataBufferSPtr);

    OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
    co_return resultCSPtr;
}

void StoreCopyStream::TraceException(__in KStringView const & methodName, __in ktl::Exception const & exception)
{
    KDynStringA stackString(this->GetThisAllocator());
    Diagnostics::GetExceptionStackTrace(exception, stackString);
    StoreEventSource::Events->StoreCopyStreamException(
        traceComponent_->PartitionId,
        traceComponent_->TraceTag,
        ToStringLiteral(methodName),
        ToStringLiteral(stackString),
        exception.GetStatus());
}
