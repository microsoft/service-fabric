// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace Data::Utilities;
using namespace ktl;

Common::StringLiteral const TraceComponent("BackupMetadataFile");

NTSTATUS BackupMetadataFile::Create(
    __in PartitionedReplicaId const & traceId,
    __in KWString const & filePath,
    __in KAllocator & allocator,
    __out SPtr & result)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    BackupMetadataFileProperties::SPtr propertiesSPtr = nullptr;
    status = BackupMetadataFileProperties::Create(allocator, propertiesSPtr);
    RETURN_ON_FAILURE(status);

    result = _new(BACKUP_METADATA_FILE_TAG, allocator) BackupMetadataFile(
        traceId,
        filePath,
        *propertiesSPtr);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return result->Status();
}

// Write the backup metadata file
// Algorithm:
//  1. Create the BackupMetadataFileProperties
//  2. Write the properties
//  3. Write the footer
ktl::Awaitable<NTSTATUS> BackupMetadataFile::WriteAsync(
    __in FABRIC_BACKUP_OPTION backupOption,
    __in KGuid const & parentBackupId,
    __in KGuid const & backupId,
    __in KGuid const & partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in TxnReplicator::Epoch const & startingEpoch,
    __in FABRIC_SEQUENCE_NUMBER startingLSN,
    __in TxnReplicator::Epoch const & backupEpoch,
    __in FABRIC_SEQUENCE_NUMBER backupLSN,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    Common::Guid commonBackupId(backupId);

    // Create the BackupMetadataFileProperties
    // TODO: Create BackupMetadataFileProperties with all settings. Now
    // set the properties later to avoid the compiler bug.
    BackupMetadataFileProperties::SPtr backupMetadataFilePropertiesSPtr = nullptr;
    status = BackupMetadataFileProperties::Create(
        GetThisAllocator(), 
        backupMetadataFilePropertiesSPtr);
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: BackupMetadataFileProperties::Create failed.", commonBackupId, status);

    backupMetadataFilePropertiesSPtr->BackupOption = backupOption;
    backupMetadataFilePropertiesSPtr->ParentBackupId = parentBackupId;
    backupMetadataFilePropertiesSPtr->BackupId = backupId;
    backupMetadataFilePropertiesSPtr->PartitionId = partitionId;
    backupMetadataFilePropertiesSPtr->ReplicaId = replicaId;
    backupMetadataFilePropertiesSPtr->StartingEpoch = startingEpoch;
    backupMetadataFilePropertiesSPtr->StartingLSN = startingLSN;
    backupMetadataFilePropertiesSPtr->BackupEpoch = backupEpoch;
    backupMetadataFilePropertiesSPtr->BackupLSN = backupLSN;

    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath_,
        FALSE,
        KBlockFile::eCreateAlways,
        KBlockFile::eInheritFileSecurity,
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());

    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: CreateSparseFileAsync failed.", commonBackupId, status);
    KFinally([&] {fileSPtr->Close(); });

    io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), GetThisAllocationTag());
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: KFileStream::Create failed.", commonBackupId, status);

    status = co_await fileStreamSPtr->OpenAsync(*fileSPtr);
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: KFileStream::OpenAsync failed.", commonBackupId, status);

    // Write the properties.
    BlockHandle::SPtr propertiesHandleSPtr = nullptr;
    FileBlock<BackupMetadataFileProperties::SPtr>::SerializerFunc propertiesSerializerFunction(backupMetadataFilePropertiesSPtr.RawPtr(), &BackupMetadataFileProperties::Write);
    status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(*fileStreamSPtr, propertiesSerializerFunction, GetThisAllocator(), cancellationToken, propertiesHandleSPtr);
    if (NT_SUCCESS(status) == false)
    {
        NTSTATUS closeStatus = co_await fileStreamSPtr->CloseAsync();
        ASSERT_IFNOT(NT_SUCCESS(closeStatus), "{0}: WriteAsync: FileStream CloseAsync failed. {1:x}", TraceId, closeStatus);
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: WriteBlockAsync failed.", commonBackupId, status);
    }
    
    // Write the footer.
    status = FileFooter::Create(*propertiesHandleSPtr, Version, GetThisAllocator(), footerSPtr_);
    if (NT_SUCCESS(status) == false)
    {
        NTSTATUS closeStatus = co_await fileStreamSPtr->CloseAsync();
        ASSERT_IFNOT(NT_SUCCESS(closeStatus), "{0}: WriteAsync: FileStream CloseAsync failed. {1:x}", TraceId, closeStatus);
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: Footer create failed.", commonBackupId, status);
    }

    BlockHandle::SPtr returnBlockHandle;
    FileBlock<FileFooter::SPtr>::SerializerFunc footerSerializerFunction(footerSPtr_.RawPtr(), &FileFooter::Write);
    status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(*fileStreamSPtr, footerSerializerFunction, GetThisAllocator(), cancellationToken, returnBlockHandle);
    if (NT_SUCCESS(status) == false)
    {
        NTSTATUS closeStatus = co_await fileStreamSPtr->CloseAsync();
        ASSERT_IFNOT(NT_SUCCESS(closeStatus), "{0}: WriteAsync: FileStream CloseAsync failed. {1:x}", TraceId, closeStatus);
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: WriteBlockAsync for footer failed.", commonBackupId, status);
    }

    // Flush to underlying stream.
    status = co_await fileStreamSPtr->FlushAsync();
    if (NT_SUCCESS(status) == false)
    {
        NTSTATUS closeStatus = co_await fileStreamSPtr->CloseAsync();
        ASSERT_IFNOT(NT_SUCCESS(closeStatus), "{0}: WriteAsync: FileStream CloseAsync failed. {1:x}", TraceId, closeStatus);
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::WriteAsync: FlushAsync for footer failed.", commonBackupId, status);
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(NT_SUCCESS(status), "{0}: CreateAsync: FileStream CloseAsync failed. {1:x}", TraceId, status);

    co_return status;
}

// Read the backup metadata file
// Algorithm:
//  1. Read the footer
//  2. Read the properties
ktl::Awaitable<NTSTATUS> BackupMetadataFile::ReadAsync(
    __in KGuid const & readId,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    Common::Guid commonReadId(readId);

    ASSERT_IF(filePath_.Length() < 2, "{0}: Invalid current checkpoint name.", TraceId);

    if (!Common::File::Exists(static_cast<LPCWSTR>(filePath_)))
    {
        BM_TRACE_AND_CO_RETURN(L"BackupMetadataFile::ReadAsync: Backup file does not exist.", commonReadId, STATUS_INTERNAL_DB_CORRUPTION);
    }

    // Open the file.
    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath_,
        FALSE,                      // IsWriteThrough. False because this only to read.
        KBlockFile::eOpenExisting,  // CreateDisposition
        KBlockFile::eRandomAccess,  // CreateOptions. Random access because we start from then end and then move up.
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::ReadAsync: CreateSparseFileAsync failed.", commonReadId, status);

    KFinally([&] {fileSPtr->Close(); });

    io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = io::KFileStream::Create(
        fileStreamSPtr,
        GetThisAllocator(),
        GetThisAllocationTag());
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::ReadAsync: KFileStream::Create failed.", commonReadId, status);

    status = co_await fileStreamSPtr->OpenAsync(*fileSPtr);
    BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::ReadAsync: KFileStream::OpenAsync failed.", commonReadId, status);

    // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
    status = co_await this->ReadFooterAsync(*fileStreamSPtr, cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
        // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
        NTSTATUS closeStatus = co_await fileStreamSPtr->CloseAsync();
        ASSERT_IFNOT(NT_SUCCESS(closeStatus), "{0}: ReadAsync: FileStream CloseAsync failed. {1:x}", TraceId, closeStatus);
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::ReadAsync: ReadFooterAsync failed.", commonReadId, status);
    }

    // Read and validate the properties section.
    status = co_await this->ReadPropertiesAsync(*fileStreamSPtr, cancellationToken);
    if (NT_SUCCESS(status) == false)
    {
        // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
        // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
        NTSTATUS closeStatus = co_await fileStreamSPtr->CloseAsync();
        ASSERT_IFNOT(NT_SUCCESS(closeStatus), "{0}: ReadAsync: FileStream CloseAsync failed. {1:x}", TraceId, closeStatus);
        BM_TRACE_AND_CO_RETURN_ON_FAILURE(L"BackupMetadataFile::ReadAsync: ReadPropertiesAsync failed.", commonReadId, status);
    }

    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(NT_SUCCESS(status), "{0}: ReadAsync: FileStream CloseAsync failed. {1:x}", TraceId, status);

    co_return status;
}

// Read the footer section from the file stream 
ktl::Awaitable<NTSTATUS> BackupMetadataFile::ReadFooterAsync(
    __in ktl::io::KFileStream & fileStream,
    __in ktl::CancellationToken const & cancellationToken) noexcept
{
    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    io::KFileStream::SPtr fileStreamSPtr(&fileStream);

    BlockHandle::SPtr footerHandle = nullptr;
    LONG64 tmpSize = fileStreamSPtr->GetLength();

    ASSERT_IFNOT(tmpSize >= 0, "{0}: ReadFooterAsync: Read file size from fileStream, the size must be positive number.", TraceId);

    ULONG64 fileSize = static_cast<ULONG64>(tmpSize);
    ULONG32 footerSize = FileFooter::SerializedSize();
    if (fileSize < (footerSize + static_cast<ULONG64>(CheckSumSectionSize)))
    {
        // File is too small to contain even a footer.
        co_return STATUS_INTERNAL_DB_CORRUPTION;
    }

    // Create a block handle for the footer.
    // Note that size required is footer size + the checksum (sizeof(ULONG64))
    ULONG64 footerStartingOffset = fileSize - footerSize - CheckSumSectionSize;
    status = BlockHandle::Create(
        footerStartingOffset,
        footerSize,
        GetThisAllocator(),
        footerHandle);
    CO_RETURN_ON_FAILURE(status);

    FileBlock<FileFooter::SPtr>::DeserializerFunc deserializerFunction(&FileFooter::Read);
    try
    {
        footerSPtr_ = co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(
            *fileStreamSPtr,
            *footerHandle,
            deserializerFunction,
            GetThisAllocator(),
            cancellationToken);
    }
    catch (Exception const & e)
    {
        co_return e.GetStatus();
    }

    // Port note: do not assert on the version. It makes the code not forward compatibile.
    co_return status;
}

// Read properties section
ktl::Awaitable<NTSTATUS> BackupMetadataFile::ReadPropertiesAsync(
    __in io::KFileStream & fileStream,
    __in CancellationToken const & cancellationToken) noexcept
{
    KCoShared$ApiEntry();

    io::KFileStream::SPtr fileStreamSPtr(&fileStream);
    BlockHandle::SPtr propertiesHandle = footerSPtr_->PropertiesHandle;
    FileBlock<BackupMetadataFileProperties::SPtr>::DeserializerFunc propertiesDeserializerFunction(&BackupMetadataFileProperties::Read);
    BackupMetadataFileProperties::SPtr temp = nullptr;

    try
    {
        propertiesSPtr_ = co_await FileBlock<BackupMetadataFileProperties::SPtr>::ReadBlockAsync(
            *fileStreamSPtr,
            *propertiesHandle,
            propertiesDeserializerFunction,
            GetThisAllocator(),
            cancellationToken);
    }
    catch (Exception const & exception)
    {
        co_return exception.GetStatus();
    }

    co_return STATUS_SUCCESS;
}

bool BackupMetadataFile::Equals(__in BackupMetadataFile const & other) const noexcept
{
    return BackupOption == other.BackupOption
        && BackupId == other.BackupId
        && PropertiesPartitionId == other.PropertiesPartitionId
        && PropertiesReplicaId == other.PropertiesReplicaId
        && BackupEpoch.DataLossVersion == other.BackupEpoch.DataLossVersion
        && BackupEpoch.ConfigurationVersion == other.BackupEpoch.ConfigurationVersion
        && BackupLSN == other.BackupLSN;
}

KWString const BackupMetadataFile::get_FileName() const { return filePath_; }

TxnReplicator::Epoch BackupMetadataFile::get_BackupEpoch() const { return propertiesSPtr_->BackupEpoch; }

KGuid BackupMetadataFile::get_BackupId() const { return propertiesSPtr_->BackupId; }

FABRIC_SEQUENCE_NUMBER BackupMetadataFile::get_BackupLSN() const { return propertiesSPtr_->BackupLSN; }

FABRIC_BACKUP_OPTION BackupMetadataFile::get_BackupOption() const { return propertiesSPtr_->BackupOption; }

KGuid BackupMetadataFile::get_ParentBackupId() const { return propertiesSPtr_->ParentBackupId; }

KGuid BackupMetadataFile::get_PropertiesPartitionId() const { return propertiesSPtr_->PartitionId; }

FABRIC_REPLICA_ID BackupMetadataFile::get_PropertiesReplicaId() const { return propertiesSPtr_->ReplicaId; }

TxnReplicator::Epoch BackupMetadataFile::get_StartingEpoch() const { return propertiesSPtr_->StartingEpoch; }

FABRIC_SEQUENCE_NUMBER BackupMetadataFile::get_StartingLSN() const { return propertiesSPtr_->StartingLSN; }

BackupMetadataFile::BackupMetadataFile(
    __in PartitionedReplicaId const& traceId,
    __in KWString const & filePath,
    __in BackupMetadataFileProperties & properties) noexcept
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , filePath_(filePath)
    , footerSPtr_(nullptr)
    , propertiesSPtr_(&properties)
{
}

BackupMetadataFile::~BackupMetadataFile()
{
}
