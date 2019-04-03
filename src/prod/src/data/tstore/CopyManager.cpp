// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;
using namespace Common;

NTSTATUS CopyManager::Create(
    __in KStringView const & directory,
    __in StoreTraceComponent & traceComponent,
    __in KAllocator & allocator,
    __in StorePerformanceCountersSPtr & perfCounters,
    __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(COPY_MANAGER_TAG, allocator) CopyManager(directory, traceComponent, perfCounters);

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

CopyManager::CopyManager(
    __in KStringView const & directory,
    __in StoreTraceComponent & traceComponent,
    __in StorePerformanceCountersSPtr & perfCounters) :
    copyCompleted_(true), // Starting at true in case of empty copy
    currentCopyFileNameSPtr_(nullptr),
    currentCopyFileStreamSPtr_(nullptr),
    copyProtocolVersion_(InvalidCopyProtocolVersion),
    fileCount_(0),
    metadataTableSPtr_(nullptr),
    traceComponent_(&traceComponent),
    perfCounterWriter_(perfCounters)
{
    NTSTATUS status = KString::Create(workDirectorySPtr_, this->GetThisAllocator(), directory);
    Diagnostics::Validate(status);
}

CopyManager::~CopyManager()
{
}

ktl::Awaitable<void> CopyManager::AddCopyDataAsync(__in OperationData const & data)
{
    OperationData::CSPtr dataCSPtr = &data;
    ULONG32 receivedBytes = 0;
    for (ULONG32 i = 0; i < dataCSPtr->BufferCount; i++)
    {
        KBuffer::CSPtr bufferCSPtr = (*dataCSPtr)[i];
        STORE_ASSERT(bufferCSPtr != nullptr, "OperationData buffer is null");
        KBuffer::SPtr bufferSPtr = const_cast<KBuffer *>(&*bufferCSPtr);
        receivedBytes += co_await ProcessCopyOperationAsync(*workDirectorySPtr_, *bufferSPtr);
    }
    
    StoreEventSource::Events->CopyManagerAddCopyDataAsync(traceComponent_->PartitionId, traceComponent_->TraceTag, receivedBytes);
}

ktl::Awaitable<ULONG32> CopyManager::ProcessCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    KBuffer::SPtr dataSPtr = &data;
    STORE_ASSERT(dataSPtr->QuerySize() > 0, "OperationData buffer is empty");
    
    // The last byte indicates the operation type.
    byte* buffer = static_cast<byte *>(dataSPtr->GetBuffer());
    StoreCopyOperation::Enum operation = static_cast<StoreCopyOperation::Enum>(buffer[dataSPtr->QuerySize() - 1]);

    // Create a buffer with all the data except the last byte
    KBuffer::SPtr operationDataBuffer = nullptr;
    auto status = KBuffer::CreateOrCopyFrom(operationDataBuffer, *dataSPtr, 0, dataSPtr->QuerySize() - 1, GetThisAllocator());
    Diagnostics::Validate(status);

    switch (operation)
    {
    case StoreCopyOperation::Version: 
        co_await ProcessVersionCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::MetadataTable: 
        co_await ProcessMetadataTableCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::StartKeyFile: 
        co_await ProcessStartKeyFileCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::WriteKeyFile: 
        co_await ProcessWriteKeyFileCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::EndKeyFile: 
        co_await ProcessEndKeyFileCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::StartValueFile: 
        co_await ProcessStartValueFileCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::WriteValueFile: 
        co_await ProcessWriteValueFileCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::EndValueFile: 
        co_await ProcessEndValueFileCopyOperationAsync(directory, *operationDataBuffer); 
        break;
    case StoreCopyOperation::Complete: 
        co_await ProcessCompleteCopyOperationAsync(directory); 
        break;
    default: 
        STORE_ASSERT(false, "Invalid copy operation {1}", (byte)operation);
    }

    co_return dataSPtr->QuerySize();
}

ktl::Awaitable<void> CopyManager::ProcessVersionCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    copyCompleted_ = false; // Copy has actually started
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        StoreEventSource::Events->CopyManagerProcessVersionCopyOperationData(traceComponent_->PartitionId, traceComponent_->TraceTag, ToStringLiteral(directory), data.QuerySize());

        STORE_ASSERT(copyProtocolVersion_ == InvalidCopyProtocolVersion, "unexpected copy operation: Version received multiple times");
        STORE_ASSERT(data.QuerySize() == sizeof(ULONG32), "unexpected copy operation: version operation data has an unexpected size: {1}", data.QuerySize());

        ULONG32 copyVersion = *(static_cast<ULONG32 *>(data.GetBuffer()));
        if (copyVersion != CopyManager::CopyProtocolVersion)
        {
            StoreEventSource::Events->CopyManagerProcessVersionCopyOperationMsg(traceComponent_->PartitionId, traceComponent_->TraceTag, copyVersion);
            throw ktl::Exception(SF_STATUS_INVALID_OPERATION); // TODO: Use actual exception
        }

        copyProtocolVersion_ = copyVersion;

        StoreEventSource::Events->CopyManagerProcessVersionCopyOperationProtocol(traceComponent_->PartitionId, traceComponent_->TraceTag, ToStringLiteral(directory), copyVersion);
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessVersionCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessMetadataTableCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        StoreEventSource::Events->CopyManagerProcessMetadataTableCopyOperation(traceComponent_->PartitionId, traceComponent_->TraceTag, ToStringLiteral(directory), data.QuerySize());

        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: MetadataTable received before Version operation");
        STORE_ASSERT(metadataTableSPtr_ == nullptr, "unexpected copy operation: MetadataTable received multiple times");
        STORE_ASSERT(currentCopyFileStreamSPtr_ == nullptr, "unexpected copy operation: MetadataTable received when we are copying a checkpoint file");

        auto status = MetadataTable::Create(GetThisAllocator(), metadataTableSPtr_);
        Diagnostics::Validate(status);

        MemoryBuffer::SPtr memoryStreamSPtr = nullptr;
        status = MemoryBuffer::Create(data, GetThisAllocator(), memoryStreamSPtr);
        Diagnostics::Validate(status);
        co_await MetadataManager::OpenAsync(*metadataTableSPtr_, *memoryStreamSPtr, data.QuerySize(), GetThisAllocator(), *traceComponent_);
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcesMetadataTable", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessStartKeyFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;
    
    try
    {
        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: StartKeyFile received before Version operation");
        STORE_ASSERT(metadataTableSPtr_ != nullptr, "unexpected copy operation: key checkpoint file received before metadata table");
        STORE_ASSERT(currentCopyFileStreamSPtr_ == nullptr, "unexpected copy operation: StartKeyFile received when we are already copying a checkpoint file");

        KBuffer::SPtr dataSPtr = &data;

        KString::SPtr directorySPtr;
        auto status = KString::Create(directorySPtr, GetThisAllocator(), directory);
        Diagnostics::Validate(status);

        // Start writing a new filename
        KGuid fileGuid;
        fileGuid.CreateNew();
        KString::SPtr checkpointFileName = nullptr;
        status = KString::Create(checkpointFileName, this->GetThisAllocator(), KStringView::MaxGuidString);
        Diagnostics::Validate(status);
        BOOLEAN result = checkpointFileName->FromGUID(fileGuid);
        STORE_ASSERT(result == TRUE, "failed to load GUID into name");

        fileCount_++;

        status = KString::Create(currentCopyFileNameSPtr_, GetThisAllocator(), L"");
        Diagnostics::Validate(status);
        bool concatenated = currentCopyFileNameSPtr_->Concat(*checkpointFileName);
        STORE_ASSERT(concatenated, "Unable to concat checkpoint filename to path string");
        concatenated = currentCopyFileNameSPtr_->Concat(KeyCheckpointFile::GetFileExtension());
        STORE_ASSERT(concatenated, "Unable to concat checkpoint file extension to path string");

        // File id is kept as ULONG32 at the end of the buffer
        ULONG fileIdOffset = dataSPtr->QuerySize() - sizeof(ULONG32);
        ULONG32 fileId = CopyManager::GetULONG32(*dataSPtr, fileIdOffset);

        // Assert that is present in metadata table
        FileMetadata::SPtr metadataSPtr;
        bool found = metadataTableSPtr_->Table->TryGetValue(fileId, metadataSPtr);
        STORE_ASSERT(found == true, "expected to find file ID {1} in metadata table", fileId);

        // Filenames from GUIDs aren't null terminated which causes problems down the line
        KStringView nullTerminatedCheckpointFileName(*checkpointFileName);
        KString::SPtr filenameSPtr;
        status = KString::Create(filenameSPtr, this->GetThisAllocator(), nullTerminatedCheckpointFileName);
        Diagnostics::Validate(status);
        metadataSPtr->FileName = *filenameSPtr;

        auto fullCopyFileName = CombinePaths(directory, *currentCopyFileNameSPtr_);

        StoreEventSource::Events->CopyManagerProcessStartKeyFileCopyOperation(
            traceComponent_->PartitionId, traceComponent_->TraceTag,
            ToStringLiteral(*directorySPtr),
            ToStringLiteral(*fullCopyFileName),
            dataSPtr->QuerySize(),
            fileId);

        currentCopyFileSPtr_ = co_await OpenFileAsync(*fullCopyFileName);
        status = ktl::io::KFileStream::Create(currentCopyFileStreamSPtr_, GetThisAllocator());
        Diagnostics::Validate(status);

        status = co_await currentCopyFileStreamSPtr_->OpenAsync(*currentCopyFileSPtr_);
        STORE_ASSERT(NT_SUCCESS(status), "Unable to open file stream for file {1}", fullCopyFileName->operator LPCWSTR());

        // Write everything in the buffer up to the fileId
        perfCounterWriter_.StartMeasurement();
        status = co_await currentCopyFileStreamSPtr_->WriteAsync(*dataSPtr, 0, fileIdOffset);
        perfCounterWriter_.StopMeasurement(fileIdOffset);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessStartKeyFileCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessWriteKeyFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        ULONG32 receivedBytes = data.QuerySize();
        StoreEventSource::Events->CopyManagerProcessWriteKeyFileCopyOperation(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(directory), 
            ToStringLiteral(*currentCopyFileNameSPtr_),
            receivedBytes);

        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: WriteKeyFile received before Version operation");
        STORE_ASSERT(metadataTableSPtr_ != nullptr, "unexpected copy operation: WriteKeyFile received before metadata table");
        STORE_ASSERT(currentCopyFileStreamSPtr_ != nullptr, "unexpected copy operation: WriteKeyFile received before StartKeyFile");

        // Append the data to the existing checkpoint file stream
        
        perfCounterWriter_.StartMeasurement();
        auto status = co_await currentCopyFileStreamSPtr_->WriteAsync(data);
        perfCounterWriter_.StopMeasurement(receivedBytes);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessWriteKeyFileCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessEndKeyFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: EndKeyFile received before Version operation");
        STORE_ASSERT(currentCopyFileStreamSPtr_ != nullptr, "unexpected copy operation: EndKeyFile received when we aren't copying a checkpoint file");
        STORE_ASSERT(currentCopyFileNameSPtr_ != nullptr, "unexpected copy operation: EndKeyFile received when we don't have a valid checkpoint file");

        StoreEventSource::Events->CopyManagerProcessEndKeyFileCopyOperation(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(directory), 
            ToStringLiteral(*currentCopyFileNameSPtr_),
            currentCopyFileStreamSPtr_->Position);

        // Flush and close the current copied checkpoint file stream.
        perfCounterWriter_.StartMeasurement();
        auto status = co_await currentCopyFileStreamSPtr_->FlushAsync();
        perfCounterWriter_.StopMeasurement();
        Diagnostics::Validate(status);

        if (currentCopyFileStreamSPtr_ != nullptr)
        {
            status = co_await currentCopyFileStreamSPtr_->CloseAsync();
            Diagnostics::Validate(status);
            currentCopyFileStreamSPtr_ = nullptr;
        }

        if (currentCopyFileSPtr_ != nullptr)
        {
            currentCopyFileSPtr_->Close();
            currentCopyFileSPtr_ = nullptr;
        }
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessEndKeyFileCopyOperation", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessStartValueFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: StartValueFile received before Version operation");
        STORE_ASSERT(metadataTableSPtr_ != nullptr, "unexpected copy operation: value checkpoint file received before metadata table");
        STORE_ASSERT(currentCopyFileStreamSPtr_ == nullptr, "unexpected copy operation: StartValueFile received when we are already copying a checkpoint file");

        KBuffer::SPtr dataSPtr = &data;

        KString::SPtr directorySPtr;
        auto status = KString::Create(directorySPtr, GetThisAllocator(), directory);
        Diagnostics::Validate(status);

        // File id is kept as ULONG32 at the end of the buffer
        ULONG fileIdOffset = dataSPtr->QuerySize() - sizeof(ULONG32);
        ULONG32 fileId = CopyManager::GetULONG32(*dataSPtr, fileIdOffset);

        // Assert that is present in metadata table
        FileMetadata::SPtr metadataSPtr;
        bool found = metadataTableSPtr_->Table->TryGetValue(fileId, metadataSPtr);
        STORE_ASSERT(found == true, "expected to find file ID {1} in metadata table", fileId);

        status = KString::Create(currentCopyFileNameSPtr_, GetThisAllocator(), L"");
        Diagnostics::Validate(status);
        bool concatenated = currentCopyFileNameSPtr_->Concat(*metadataSPtr->FileName);
        STORE_ASSERT(concatenated, "Unable to concat checkpoint filename to path string");
        concatenated = currentCopyFileNameSPtr_->Concat(ValueCheckpointFile::GetFileExtension());
        STORE_ASSERT(concatenated, "Unable to concat checkpoint file extension to path string");

        auto fullCopyFileName = CombinePaths(directory, *currentCopyFileNameSPtr_);

        StoreEventSource::Events->CopyManagerProcessStartValueFileCopyOperation(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(*directorySPtr),
            ToStringLiteral(*fullCopyFileName),
            dataSPtr->QuerySize(),
            fileId);

        currentCopyFileSPtr_ = co_await OpenFileAsync(*fullCopyFileName);
        status = ktl::io::KFileStream::Create(currentCopyFileStreamSPtr_, GetThisAllocator());
        Diagnostics::Validate(status);

        status = co_await currentCopyFileStreamSPtr_->OpenAsync(*currentCopyFileSPtr_);
        STORE_ASSERT(NT_SUCCESS(status), "Unable to open file stream for file {1}", fullCopyFileName->operator LPCWSTR());

        // Write everything in the buffer up to the fileId
        perfCounterWriter_.StartMeasurement();
        status = co_await currentCopyFileStreamSPtr_->WriteAsync(*dataSPtr, 0, fileIdOffset);
        perfCounterWriter_.StopMeasurement(fileIdOffset);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessStartValueFileCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessWriteValueFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        ULONG32 receivedBytes = data.QuerySize();
        StoreEventSource::Events->CopyManagerProcessWriteValueFileCopyOperation(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(directory),
            ToStringLiteral(*currentCopyFileNameSPtr_),
            receivedBytes);

        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: WriteValueFile before Version operation");
        STORE_ASSERT(metadataTableSPtr_ != nullptr, "unexpected copy operation: WriteValueFile received before metadata table");
        STORE_ASSERT(currentCopyFileStreamSPtr_ != nullptr, "unexpected copy operation: WriteValueFile received before StartKeyFile");

        // Append the data to the existing checkpoint file stream
        perfCounterWriter_.StartMeasurement();
        auto status = co_await currentCopyFileStreamSPtr_->WriteAsync(data);
        perfCounterWriter_.StopMeasurement(receivedBytes);
        Diagnostics::Validate(status);
    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessWriteValueFileCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessEndValueFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data)
{
    SharedException::CSPtr exceptionCSPtr = nullptr;

    try
    {
        StoreEventSource::Events->CopyManagerProcessEndValueFileCopyOperation(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(directory),
            ToStringLiteral(*currentCopyFileNameSPtr_),
            currentCopyFileStreamSPtr_->Position);

        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: EndValueFile received before Version operation");
        STORE_ASSERT(currentCopyFileStreamSPtr_ != nullptr, "unexpected copy operation: EndValueFile received when we aren't copying a checkpoint file");
        STORE_ASSERT(currentCopyFileNameSPtr_ != nullptr, "unexpected copy operation: EndValueFile received when we don't have a valid checkpoint file");

        // Flush and close the current copied checkpoint file stream.
        perfCounterWriter_.StartMeasurement();
        auto status = co_await currentCopyFileStreamSPtr_->FlushAsync();
        perfCounterWriter_.StopMeasurement();
        Diagnostics::Validate(status);

        if (currentCopyFileStreamSPtr_ != nullptr)
        {
            status = co_await currentCopyFileStreamSPtr_->CloseAsync();
            Diagnostics::Validate(status);
            currentCopyFileStreamSPtr_ = nullptr;
        }

        if (currentCopyFileSPtr_ != nullptr)
        {
            currentCopyFileSPtr_->Close();
            currentCopyFileSPtr_ = nullptr;
        }
    }
    catch (Exception const & e)
    {
        TraceException(L"ProcessEndValueFileCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::ProcessCompleteCopyOperationAsync(__in KStringView const & directory)
{
    perfCounterWriter_.UpdatePerformanceCounter();
    SharedException::CSPtr exceptionCSPtr = nullptr;
    
    try
    {
        StoreEventSource::Events->CopyManagerProcessCompleteCopyOperation(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            ToStringLiteral(directory),
            perfCounterWriter_.AvgDiskTransferBytesPerSec);

        // Consistency checks
        STORE_ASSERT(copyProtocolVersion_ != InvalidCopyProtocolVersion, "unexpected copy operation: Complete received before Version operation");
        STORE_ASSERT(currentCopyFileStreamSPtr_ == nullptr, "unexpected copy operation: Complete received when we are copying a checkpoint file");
        STORE_ASSERT(metadataTableSPtr_ != nullptr, "unexpected copy operation: Complete received before MetadataTable");

        auto expectedCount = metadataTableSPtr_->Table->Count;
        STORE_ASSERT(fileCount_ == expectedCount, "wrong number of files copied. copied={1} expected={2}", fileCount_, expectedCount);

        copyCompleted_ = true;

        // Reset copy state
        currentCopyFileNameSPtr_ = nullptr;

        if (currentCopyFileStreamSPtr_ != nullptr)
        {
            NTSTATUS status = co_await currentCopyFileStreamSPtr_->CloseAsync();
            Diagnostics::Validate(status);
            currentCopyFileStreamSPtr_ = nullptr;
        }

        if (currentCopyFileSPtr_ != nullptr)
        {
            currentCopyFileSPtr_->Close();
            currentCopyFileSPtr_ = nullptr;
        }

        copyProtocolVersion_ = InvalidCopyProtocolVersion;

    }
    catch (ktl::Exception const & e)
    {
        TraceException(L"ProcessCompleteCopyOperationAsync", e);
        exceptionCSPtr = SharedException::Create(e, GetThisAllocator());
    }

    if (exceptionCSPtr != nullptr)
    {
        co_await CloseAsync();
        auto exec = exceptionCSPtr->Info;
        throw exec;
    }
}

ktl::Awaitable<void> CopyManager::CloseAsync()
{
    if (currentCopyFileStreamSPtr_ != nullptr)
    {
        NTSTATUS status = co_await currentCopyFileStreamSPtr_->CloseAsync();
        Diagnostics::Validate(status);
        currentCopyFileStreamSPtr_ = nullptr;
    }

    if (currentCopyFileSPtr_ != nullptr)
    {
        currentCopyFileSPtr_->Close();
        currentCopyFileSPtr_ = nullptr;
    }

    copyProtocolVersion_ = StoreCopyOperation::Enum::Version;
}

KString::SPtr CopyManager::CombinePaths(__in KStringView const & directory, __in KStringView const & filename)
{
    KString::SPtr filePathSPtr;
    auto status = KString::Create(filePathSPtr, GetThisAllocator(), L"");
    Diagnostics::Validate(status);
    bool result = filePathSPtr->Concat(directory);
    STORE_ASSERT(result, "Unable to concat directory to path string");
    result = filePathSPtr->Concat(Common::Path::GetPathSeparatorWstr().c_str());
    STORE_ASSERT(result, "Unable to concat path separator to path string");
    result = filePathSPtr->Concat(filename);
    STORE_ASSERT(result, "Unable to concat filename to path string");

    return filePathSPtr;
}

ktl::Awaitable<KBlockFile::SPtr> CopyManager::OpenFileAsync(__in KStringView const & filename)
{
    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    KWString pathWString(GetThisAllocator(), filename);

    auto createOptions = KBlockFile::CreateOptions::eShareRead | KBlockFile::CreateOptions::eShareWrite | KBlockFile::CreateOptions::eInheritFileSecurity;

    KBlockFile::SPtr blockFileSPtr = nullptr;

    try
    {
        auto status = co_await KBlockFile::CreateSparseFileAsync(
            pathWString,
            TRUE,
            KBlockFile::CreateDisposition::eCreateNew,
            static_cast<KBlockFile::CreateOptions>(createOptions),
            blockFileSPtr,
            nullptr,
            GetThisAllocator(),
            STORE_COPY_STREAM_TAG);

        STORE_ASSERT(NT_SUCCESS(status), "Unable to open file {1}", filename.operator LPCWSTR());
    }
    catch (ktl::Exception &)
    {
        if (blockFileSPtr != nullptr)
        {
            blockFileSPtr->Close();
            throw;
        }
    }

    co_return blockFileSPtr;
}

ULONG32 CopyManager::GetULONG32(__in KBuffer & buffer, __in ULONG offsetBytes)
{
    byte* data = static_cast<byte *>(buffer.GetBuffer());
    void* valueAddress = static_cast<void *>(&data[offsetBytes]);
    ULONG32* valuePtr = static_cast<ULONG32 *>(valueAddress);
    return *valuePtr;
}

void CopyManager::TraceException(__in KStringView const & methodName, __in ktl::Exception const & exception)
{
    UNREFERENCED_PARAMETER(methodName);
    KDynStringA stackString(this->GetThisAllocator());
    Diagnostics::GetExceptionStackTrace(exception, stackString);
    StoreEventSource::Events->CopyManagerException(
        traceComponent_->PartitionId,
        traceComponent_->TraceTag,
        ToStringLiteral(methodName),
        ToStringLiteral(stackString),
        exception.GetStatus());
}

