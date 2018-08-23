// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace Data::Utilities;
using namespace ktl;

BackupLogFileAsyncEnumerator::SPtr BackupLogFileAsyncEnumerator::Create(
    __in KWString const & fileName,
    __in ULONG32 logRecordCount,
    __in BlockHandle & blockHandle,
    __in KAllocator & allocator)
{
    BackupLogFileAsyncEnumerator * pointer = _new(BACKUP_LOG_FILE_ASYNC_ENUMERATOR_TAG, allocator) BackupLogFileAsyncEnumerator(
        fileName,
        logRecordCount,
        blockHandle);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    THROW_ON_FAILURE(pointer->Status());

    return SPtr(pointer);
}

LogRecord::SPtr BackupLogFileAsyncEnumerator::GetCurrent()
{
    // currentIndex_ initialize with -1, but when GetCurrent got called, it must not be negative.
    ASSERT_IFNOT(currentIndex_ >= 0, "Index must not be negative number. Index: {0}, FileName: {1}.", currentIndex_, static_cast<LPCWSTR>(fileName_));

    return (*currentLogRecordsSPtr_)[currentIndex_];
}

// Move the async enumerator to see if more log records exist.
// Algorithm:
// 1. Open the file and the stream if not already open.
// 2. Read from the cache if avaiable.
// 3. Read the next block and populate the cache if available.
ktl::Awaitable<bool> BackupLogFileAsyncEnumerator::MoveNextAsync(
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    // Step 1: If not already done, open the file and the stream.
    // MCoskun: Uses default IOPriorityHint since this operation is called during Restore.
    if (fileStreamSPtr_ == nullptr)
    {
        KWString & tmpFileName = const_cast<KWString &>(fileName_);

        NTSTATUS status;
        status = co_await KBlockFile::CreateSparseFileAsync(
            tmpFileName,
            FALSE, // IsWriteThrough.
            KBlockFile::eOpenExisting,
            KBlockFile::eSequentialAccess,
            fileSPtr_,
            nullptr,
            GetThisAllocator(),
            GetThisAllocationTag());
        THROW_ON_FAILURE(status);

        ktl::io::KFileStream::SPtr localFileStream = nullptr;
        status = ktl::io::KFileStream::Create(
            localFileStream,
            GetThisAllocator(),
            GetThisAllocationTag());
        THROW_ON_FAILURE(status);

        status = co_await localFileStream->OpenAsync(*fileSPtr_);
        THROW_ON_FAILURE(status);

        // Create the local FileStream and set the member only if open file stream is successful.
        // This is used to avoid CloseAsync call on fileStream if Create succeed but OpenAsync failed.
        fileStreamSPtr_ = Ktl::Move(localFileStream);
    }

    // Step 2: If there are items in the cache that have not been returned yet,
    // move the cursor and return true.
    LONG32 count = static_cast<LONG32>(currentLogRecordsSPtr_->Count());
    if (currentIndex_ < count - 1)
    {
        currentIndex_++;
        co_return true;
    }

    // Step 3: Since there are no items in the cache, read the next block.
    // This call populates the cache (currentLogRecordsSPtr_) if there is another block.
    bool isDrained = co_await ReadBlockAsync();
    co_return isDrained;
}

void BackupLogFileAsyncEnumerator::Dispose()
{
    ASSERT_IFNOT(
        false,
        "Because filestream has to be closed asynchronously, CloseAsync should be used instead.");
}

void BackupLogFileAsyncEnumerator::Reset()
{
    throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
}

ktl::Awaitable<void> BackupLogFileAsyncEnumerator::CloseAsync()
{
    KShared$ApiEntry()

    if (isDisposed_ == true)
    {
        co_return;
    }

    // In the case of creating FileStream failed, fileStreamSPtr_ is nullptr, close call on it will AV.
    if (fileStreamSPtr_ != nullptr)
    {
        NTSTATUS status = co_await fileStreamSPtr_->CloseAsync();
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "BackupLogFileAsyncEnumerator: CloseAsync: Close file stream failed. Status: {0} FileName: {1}",
            status,
            static_cast<LPCWSTR>(fileName_));
    }

    // Note: Handle the case if KBlockFile throw exception, fileSPtr_ will be nullptr.
    if (fileSPtr_ != nullptr)
    {
        fileSPtr_->Close();
    }

    isDisposed_ = true;

    co_return;
}

// Reads the next block. If the block exists, updates the cache.
// Algorithm:
// 1. Check if another block exists. If not return.
// 2. Read the size of the block.
// 3. Read the block + checksum (Block includes the size)
// 4. Verify checksum
// 5. Reset the cache
// 6. Populate the cache.
ktl::Awaitable<bool> BackupLogFileAsyncEnumerator::ReadBlockAsync()
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG bytesRead = 0;
    LONG64 startingStreamPosition = fileStreamSPtr_->GetPosition();

    // Step 1: If we are at the end of the stream return false.
    if (static_cast<ULONG64>(startingStreamPosition) == blockHandeCSPtr_->EndOffset())
    {
        co_return false;
    }

    // Step 2: Read the size of the next block.
    KBuffer::SPtr buffer;
    status = KBuffer::Create(BlockSizeSectionSize, buffer, GetThisAllocator());
    THROW_ON_FAILURE(status);

    status = co_await fileStreamSPtr_->ReadAsync(*buffer, bytesRead, 0, BlockSizeSectionSize);
    THROW_ON_FAILURE(status);
    if (bytesRead != BlockSizeSectionSize)
    {
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    BinaryReader recordSizeReader(*buffer, GetThisAllocator());
    ULONG32 blockSize;
    recordSizeReader.Read(blockSize);

    // We need to do extra validation on the blockSize, because we haven't validated the bits
    // against the checksum and we need the blockSize to locate the checksum.
    ULONG32 blockSizeWithChecksum = blockSize + CheckSumSectionSize;
    if (static_cast<ULONG64>(startingStreamPosition) + blockSizeWithChecksum > blockHandeCSPtr_->EndOffset())
    {
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    // Step 3: Read the block + checksum (Block includes the size)
    status = KBuffer::Create(blockSizeWithChecksum, buffer, GetThisAllocator());
    THROW_ON_FAILURE(status);

    // Read the next block into memory (including the size and checksum).
    fileStreamSPtr_->SetPosition(startingStreamPosition);

    status = co_await fileStreamSPtr_->ReadAsync(*buffer, bytesRead, 0, blockSizeWithChecksum);
    THROW_ON_FAILURE(status);
    if (bytesRead != blockSizeWithChecksum)
    {
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    // Step 4: Verify checksum of the block.
    // Compute the checksum.
    ULONG64 computedChecksum = CRC64::ToCRC64(*buffer, 0, blockSize);

    // Read the checksum (checksum is after the block bytes).
    BinaryReader checksumReader(*buffer, GetThisAllocator());
    checksumReader.Position = blockSize;
    ULONG64 checksum;
    checksumReader.Read(checksum);

    if (checksum != computedChecksum)
    {
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    // Create a memory stream of the block read to avoid IO.
    MemoryStream::SPtr memoryStream = nullptr;
    status = MemoryStream::Create(*buffer, GetThisAllocator(), memoryStream);
    THROW_ON_FAILURE(status);

    // Jump over the block size since it is not part of the log record.
    memoryStream->SetPosition(BlockSizeSectionSize);

    // Step 5: Clear the cache.
    currentLogRecordsSPtr_->Clear();
    currentIndex_ = 0;

    // Step 6: Populate the cache.
    while(memoryStream->GetPosition() < blockSize)
    {
        LogRecord::SPtr logRecord = co_await LogRecord::ReadNextRecordAsync(
            *memoryStream,
            *invalidLogRecords_,
            GetThisAllocator(),
            false,  // isPhysicalRead
            true,   // useInvalidRecordPosition
            false); // setRecordLength

        status = currentLogRecordsSPtr_->Append(logRecord);
        THROW_ON_FAILURE(status);
    }

    co_return true;
}

BackupLogFileAsyncEnumerator::BackupLogFileAsyncEnumerator(
    __in KWString const & fileName,
    __in ULONG32 logRecordCount,
    __in BlockHandle & blockHandle) noexcept
    : KObject<BackupLogFileAsyncEnumerator>()
    , KShared<BackupLogFileAsyncEnumerator>()
    , fileName_(fileName)
    , logRecordCount_(logRecordCount)
    , blockHandeCSPtr_(&blockHandle)
    , invalidLogRecords_(InvalidLogRecords::Create(GetThisAllocator()))
{
    if (NT_SUCCESS(invalidLogRecords_->Status()) == false)
    {
        SetConstructorStatus(invalidLogRecords_->Status());
        return;
    }

    currentLogRecordsSPtr_ = _new(GetThisAllocationTag(), GetThisAllocator())KSharedArray<LogRecord::SPtr>;
    if (currentLogRecordsSPtr_ == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    SetConstructorStatus(currentLogRecordsSPtr_->Status());
}

BackupLogFileAsyncEnumerator::~BackupLogFileAsyncEnumerator()
{
    ASSERT_IFNOT(isDisposed_ == true, "BackupLogFileAsyncEnumerator must be disposed,");
}
