// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace TestCommon;

Common::StringLiteral const TraceComponent = "TestStateProvider";
Common::WStringLiteral const TestStateProvider::TypeName = L"TestStateProvider";
wstring const TestStateProvider::FabricTestStateProviderNamePrefix(L"fabrictest:/txrsp/");

TestStateProvider::SPtr TestStateProvider::Create(
    __in FactoryArguments const & factoryArguments,
    __in KAllocator & allocator)
{
    TestStateProvider * pointer = _new(TEST_STATE_PROVIDER_TAG, allocator) TestStateProvider(factoryArguments, false);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

TestStateProvider::SPtr TestStateProvider::Create(
    __in Data::StateManager::FactoryArguments const & factoryArguments,
    __in bool backCompatibleTest,
    __in KAllocator & allocator)
{
    TestStateProvider * pointer = _new(TEST_STATE_PROVIDER_TAG, allocator) TestStateProvider(factoryArguments, backCompatibleTest);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

Awaitable<void> TestStateProvider::AcquireLock(
    __in Common::TimeSpan const & timeout,
    wstring const & caller)
{
    KShared$ApiEntry();

    Trace.WriteNoise(
        TraceComponent,
        "{0}:{1}:{2}: Acquiring Lock in {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        caller);

    Common::Stopwatch s;
    s.Start();
    bool acquired = false;

    acquired = co_await lock_->AcquireAsync(timeout);

    s.Stop();

    if (acquired)
    {
        Trace.WriteNoise(
            TraceComponent,
            "{0}:{1}:{2}: Acquired Lock in {3} took {4}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            caller,
            s.Elapsed);
    }
    else
    {
        Trace.WriteInfo(
            TraceComponent,
            "{0}:{1}:{2}: Timedout while acquiring Lock in {3} after {4}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            caller,
            s.Elapsed);

        throw Exception(SF_STATUS_TIMEOUT);
    }
}

Awaitable<void> TestStateProvider::GetAsync(
    __out FABRIC_SEQUENCE_NUMBER & version,
    __out KString::SPtr & value)
{
    KShared$ApiEntry();
    co_await GetAsync(Common::TimeSpan::MaxValue, version, value);
    co_return;
}

Awaitable<void> TestStateProvider::GetAsync(
    __in Common::TimeSpan const & timeout,
    __out FABRIC_SEQUENCE_NUMBER & version,
    __out KString::SPtr & value)
{
    KShared$ApiEntry();

    // Check the read status before lock to avoid acquiring the lock in case of not readable.
    this->ThrowIfNotReadable();

    co_await AcquireLock(timeout, L"GetAsync");
    {
        version = currentVersion_;
        value = currentValue_;

        lock_->ReleaseLock();
    }

    // Check again to make sure it is still readable, put this call outside the 
    // lock to reduce the amount of work in the lock.
    this->ThrowIfNotReadable();

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2}:{3}: GetAsync: Version: {4} Value: {5}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(*name_),
        currentVersion_,
        ToStringLiteral(*currentValue_));

    co_return;
}

Awaitable<bool> TestStateProvider::TryUpdateAsync(
    __in Transaction & transaction,
    __in KStringView const & newValue,
    __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
    __in_opt FABRIC_SEQUENCE_NUMBER oldVersion)
{
    KShared$ApiEntry();
    
    bool result = co_await TryUpdateAsync(
        Common::TimeSpan::MaxValue,
        transaction,
        newValue,
        currentVersionIfFailed,
        oldVersion);

    co_return result;
}

Awaitable<bool> TestStateProvider::TryUpdateAsync(
    __in Common::TimeSpan const & timeout,
    __in Transaction & transaction,
    __in KStringView const & newValue,
    __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
    __in_opt FABRIC_SEQUENCE_NUMBER oldVersion)
{
    KShared$ApiEntry();

    NTSTATUS status;

    co_await AcquireLock(timeout, L"TryUpdateAsync");

    if (oldVersion != currentVersion_)
    {
        currentVersionIfFailed = currentVersion_;
        lock_->ReleaseLock();
        co_return false;
    }
    
    try
    {
        serializer_.Position = 0;
        serializer_.Write(newValue, UTF16);
        KBuffer::SPtr redoBuffer = serializer_.GetBuffer(0);

        OperationData::SPtr redoData = OperationData::Create(GetThisAllocator());
        redoData->Append(*redoBuffer);

        UndoOperationData::CSPtr undoData;
        status = UndoOperationData::Create(*PartitionedReplicaIdentifier, *currentValue_, currentVersion_, GetThisAllocator(), undoData);
        THROW_ON_FAILURE(status);

        status = transaction.AddOperation(
            nullptr,
            undoData.RawPtr(),
            redoData.RawPtr(),
            stateProviderId_,
            nullptr);
        THROW_ON_FAILURE(status);

        // Adding lock context as the last step so that we know no more failures can happen after this point
        TestLockContext::SPtr lockContext = TestLockContext::Create(*lock_, GetThisAllocator());
        status = transaction.AddLockContext(*lockContext);
        THROW_ON_FAILURE(status);

        Trace.WriteInfo(
            TraceComponent,
            "{0}:{1}:{2}:{3} TryUpdateAsync: TxnId: {4} Value: {5}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(*name_),
            transaction.TransactionId,
            ToStringLiteral(newValue));

        co_return true;
    }
    catch (Exception &)
    {
        lock_->ReleaseLock();

        throw;
    }
    catch (...)
    {
        CODING_ASSERT(
            "{0}:{1}:{2} TryUpdateAsync: Non-ktl exception type on TestStateProvider", 
            TracePartitionId,
            ReplicaId,
            StateProviderId);
    }
}

Awaitable<bool> TestStateProvider::TryRemoveMyself(
    __in Transaction & transaction,
    __in ITransactionalReplicator & replicator,
    __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
    __in_opt FABRIC_SEQUENCE_NUMBER oldVersion)
{
    KShared$ApiEntry();
    
    bool result = co_await TryRemoveMyself(
        Common::TimeSpan::MaxValue,
        transaction,
        replicator,
        currentVersionIfFailed,
        oldVersion);

    co_return result;
}

Awaitable<bool> TestStateProvider::TryRemoveMyself(
    __in Common::TimeSpan const & timeout,
    __in Transaction & transaction,
    __in ITransactionalReplicator & replicator,
    __out FABRIC_SEQUENCE_NUMBER & currentVersionIfFailed,
    __in_opt FABRIC_SEQUENCE_NUMBER oldVersion)
{
    KShared$ApiEntry();

    co_await AcquireLock(timeout, L"TryRemoveMyself");

    if (oldVersion != currentVersion_)
    {
        currentVersionIfFailed = currentVersion_;
        lock_->ReleaseLock();
        co_return false;
    }
    
    try
    {
        co_await replicator.RemoveAsync(
            transaction,
            *name_);

        // Adding lock context as the last step so that we know no more failures can happen after this point
        TestLockContext::SPtr lockContext = TestLockContext::Create(*lock_, GetThisAllocator());
        NTSTATUS status = transaction.AddLockContext(*lockContext);
        THROW_ON_FAILURE(status);

        Trace.WriteInfo(
            TraceComponent,
            "{0}:{1}:{2}:{3}: TryRemoveMyself: TxnId: {4}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(*name_),
            transaction.TransactionId);

        co_return true;
    }
    catch (Exception &)
    {
        lock_->ReleaseLock();
        throw;
    }
    catch (...)
    {
        CODING_ASSERT(
            "{0}:{1}:{2} TryRemoveMyself: Non-ktl exception type on TestStateProvider",
            TracePartitionId,
            ReplicaId,
            StateProviderId);
    }
}

KUriView const TestStateProvider::GetName() const
{
    ASSERT_IFNOT(
        name_ != nullptr,
        "{0}:{1}:{2} GetName: name_ should not be nullptr.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    return *name_;
}

OperationData::CSPtr TestStateProvider::GetInitializationParameters() const 
{
    return initializationParameters_;
}

KArray<StateProviderInfo> TestStateProvider::GetChildren(
    __in KUriView const & rootName)
{
    ASSERT_IFNOT(
        rootName.Compare(*name_) == TRUE,
        "{0}:{1}:{2} GetChildren: rootName and state provider name should be same, RootName: {3}, StateProviderName: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(rootName),
        ToStringLiteral(*name_));

    KArray<StateProviderInfo> children = KArray<StateProviderInfo>(GetThisAllocator());
    TraceAndThrowOnFailure(children.Status(), L"GetChildren: Create children array failed.");

    return children;
}

void TestStateProvider::Initialize(
    __in KWeakRef<ITransactionalReplicator> & transactionalReplicatorWRef,
    __in KStringView const& workFolder,
    __in KSharedArray<IStateProvider2::SPtr> const * const children)
{
    UNREFERENCED_PARAMETER(children);

    replicatorWRef_ = &transactionalReplicatorWRef;
    ASSERT_IFNOT(
        replicatorWRef_ != nullptr,
        "{0}:{1}:{2} Initialize: replicatorWRef should not be nullptr, current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));
    ASSERT_IFNOT(
        replicatorWRef_->IsAlive(),
        "{0}:{1}:{2} Initialize: replicatorWRef is not alive, current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    ASSERT_IFNOT(
        lifeState_ == Created,
        "{0}:{1}:{2} Initialize: lifeState should be Created, current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    hasPersistedState_ = transactionalReplicatorWRef.TryGetTarget()->HasPeristedState;

    KString::SPtr tmp = nullptr;
    NTSTATUS status = KString::Create(tmp, GetThisAllocator(), workFolder);
    TraceAndThrowOnFailure(status, L"Initialize: Create workFolder string failed.");

    workFolder_ = KString::CSPtr(tmp.RawPtr());

    if (hasPersistedState_ && !Common::Directory::Exists(static_cast<LPCWSTR>(workFolder)))
    {
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(workFolder));
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }

    tempCheckpointFilePath_ = GetCheckpointFilePath(TempCheckpointPrefix);
    currentCheckpointFilePath_ = GetCheckpointFilePath(CurrentCheckpointPrefix);
    backupCheckpointFilePath_ = GetCheckpointFilePath(BackupCheckpointPrefix);
    
    lifeState_ = Initialized;

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: InitializeAsync completed. Name: {3} WorkFolder: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(*name_),
        ToStringLiteral(workFolder));
}

Awaitable<void> TestStateProvider::OpenAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    ASSERT_IFNOT(
        lifeState_ == Initialized,
        "{0}:{1}:{2} OpenAsync: lifeState should be Initialized, current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    lifeState_ = Open;
    co_return;
}

Awaitable<void> TestStateProvider::ChangeRoleAsync(
    __in FABRIC_REPLICA_ROLE newRole,
    __in CancellationToken const& cancellationToken)
{
    KShared$ApiEntry();

    // Generic life cycle check.
    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} ChangeRoleAsync: current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        ASSERT_IFNOT(
            (lifeState_ == Open) || (lifeState_ == Secondary) || (lifeState_ == Idle),
            "{0}:{1}:{2} ChangeRoleAsync: current lifeState: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<int>(lifeState_));
        lifeState_ = Primary;
    }
    else if (newRole == FABRIC_REPLICA_ROLE_NONE)
    {
        ASSERT_IFNOT(
            (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
            "{0}:{1}:{2} ChangeRoleAsync: current lifeState: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<int>(lifeState_));
        lifeState_ = None;
    }
    else if (newRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        ASSERT_IFNOT(
            lifeState_ == Open,
            "{0}:{1}:{2} ChangeRoleAsync: current lifeState: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<int>(lifeState_));
        lifeState_ = Idle;
    }
    else if (newRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        ASSERT_IFNOT(
            (lifeState_ == Idle) || (lifeState_ == Primary),
            "{0}:{1}:{2} ChangeRoleAsync: current lifeState: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<int>(lifeState_));
        lifeState_ = Secondary;
    }
    else
    {
        throw Exception(STATUS_NOT_IMPLEMENTED);
    }

    cancellationToken.ThrowIfCancellationRequested();

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2}:{3} Api: ChangeRoleAsync completed. New Role: {4} Life State: {5}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(*name_),
        static_cast<int>(newRole),
        static_cast<int>(lifeState_));

    co_return;
}

Awaitable<void> TestStateProvider::CloseAsync(CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary) || (lifeState_ == None),
        "{0}:{1}:{2} CloseAsync: current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    // Note that we cannot assert that if lifeState_ == None, checkpoint state is removed.
    // This is because RemoveStateAsync may throw.
    lifeState_ = Closed;
    co_return;
}

void TestStateProvider::Abort() noexcept
{
    lifeState_ = Closed;
}

void TestStateProvider::PrepareCheckpoint(__in FABRIC_SEQUENCE_NUMBER checkpointLSN)
{
    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} PrepareCheckpoint: current lifeState: {3}, checkpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_),
        checkpointLSN);

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: PrepareCheckpoint {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        checkpointLSN);

    // The PrepareCheckpoint will take checkpointing on currentVersion_ and currentValue_. 
    prepareCheckpointLSN_ = checkpointLSN;

    VersionedState::CSPtr versionedState = VersionedState::Create(currentVersion_, *currentValue_, GetThisAllocator());
    lastPrepareCheckpointData_.Put(Ktl::Move(versionedState));
}

// PerformCheckpoint will create a actual file and save the information to the file
Awaitable<void> TestStateProvider::PerformCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} PerformCheckpointAsync: current lifeState: {3}, prepareCheckpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_),
        prepareCheckpointLSN_);

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: PerformCheckpointAsync {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        prepareCheckpointLSN_);

    performCheckpointLSN_ = prepareCheckpointLSN_;

    if (hasPersistedState_)
    {
        // Take checkpoint then set the value
        VersionedState::CSPtr snappedCheckpointData = lastPrepareCheckpointData_.Get();
        co_await CreateCheckpointFileAsync(*snappedCheckpointData->Value, snappedCheckpointData->Version, false);
    }

    VersionedState::CSPtr lastPrepareCheckpointData = lastPrepareCheckpointData_.Get();
    lastCheckpointedData_.Put(Ktl::Move(lastPrepareCheckpointData));

    lastPrepareCheckpointData_.Put(nullptr);

    co_return;
}

Awaitable<void> TestStateProvider::CompleteCheckpointAsync(__in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} CompleteCheckpointAsync: current lifeState: {3}, performCheckpointLSN: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_),
        performCheckpointLSN_);

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: CompleteCheckpointAsync {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        performCheckpointLSN_);

    if (Common::File::Exists(static_cast<LPCWSTR>(*tempCheckpointFilePath_)) == true)
    {
        co_await SafeFileReplaceAsync(
            *currentCheckpointFilePath_,
            *tempCheckpointFilePath_,
            *backupCheckpointFilePath_,
            cancellationToken,
            GetThisAllocator());

        completeCheckpointLSN_ = performCheckpointLSN_;
    }

    co_return;
}

// CheckpointFile will save currentVersion and currentValue
// The CheckpointFile have the structure:
//      [ CurrentVersion ][ CurrentValue ][ Checksum ]
//      [ TestStateProviderInfoHandle ][ Checksum ] -------- Properties
//      [ Footer ][ Checksum ]  ---------------------------- Footer
//
Awaitable<void> TestStateProvider::CreateCheckpointFileAsync(
    __in KString const & value,
    __in FABRIC_SEQUENCE_NUMBER version,
    __in bool writeCurrentFile)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KBlockFile::SPtr FileSPtr = nullptr;
    io::KFileStream::SPtr fileStreamSPtr = nullptr;

    KWString filePath = writeCurrentFile == true ? 
        KWString(GetThisAllocator(), currentCheckpointFilePath_->ToUNICODE_STRING()) : 
        KWString(GetThisAllocator(), tempCheckpointFilePath_->ToUNICODE_STRING());

    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        FALSE, // IsWriteThrough
        KBlockFile::eCreateAlways,
        KBlockFile::eInheritFileSecurity,
        FileSPtr,
        nullptr,
        GetThisAllocator(),
        TEST_STATE_PROVIDER_TAG);
    TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Create KBlockFile failed.");

    KFinally([&] {FileSPtr->Close(); });

    status = io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), TEST_STATE_PROVIDER_TAG);
    TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Create KFileStream failed.");

    status = co_await fileStreamSPtr->OpenAsync(*FileSPtr);
    TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Open KFileStream failed.");

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // First write the test state provider data: currentVersion_, currentValue_.
        BinaryWriter itemWriter(GetThisAllocator());
        itemWriter.Write(version);
        itemWriter.Write(value, UTF16);

        ULONG32 recordBlockSize = static_cast<ULONG32>(itemWriter.Position);
        ULONG64 recordChecksum = CRC64::ToCRC64(*itemWriter.GetBuffer(0), 0, recordBlockSize);
        itemWriter.Write(recordChecksum);
        ULONG32 recordBlockSizeWithChecksum = static_cast<ULONG32>(itemWriter.Position);

        // Updata the TestStateProviderProperties TestStateProviderInfoHandle
        TestStateProviderProperties::SPtr testStateProviderProperties_ = nullptr;
        TestStateProviderProperties::Create(GetThisAllocator(), testStateProviderProperties_);

        BlockHandle::SPtr testStateProviderInfoHandle = nullptr;
        status = BlockHandle::Create(0, recordBlockSize, GetThisAllocator(), testStateProviderInfoHandle);
        TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Create TestStateProviderInfoHandle failed.");

        testStateProviderProperties_->TestStateProviderInfoHandle = *testStateProviderInfoHandle;

        status = co_await fileStreamSPtr->WriteAsync(*itemWriter.GetBuffer(0), 0, recordBlockSizeWithChecksum);
        TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: File stream write to the file failed.");

        // Write the properties.
        BlockHandle::SPtr propertiesHandle;
        FileBlock<BlockHandle::SPtr>::SerializerFunc propertiesSerializerFunction(testStateProviderProperties_.RawPtr(), &TestStateProviderProperties::Write);
        status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(*fileStreamSPtr, propertiesSerializerFunction, GetThisAllocator(), CancellationToken::None, propertiesHandle);
        TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Write properties failed.");

        // Write the footer, set the footer version = 0
        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*propertiesHandle, 0, GetThisAllocator(), fileFooter);
        TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Create FileFooter failed.");

        BlockHandle::SPtr returnBlockHandle;
        FileBlock<FileFooter::SPtr>::SerializerFunc footerSerializerFunction(fileFooter.RawPtr(), &FileFooter::Write);
        status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(*fileStreamSPtr, footerSerializerFunction, GetThisAllocator(), CancellationToken::None, returnBlockHandle);
        TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Write footer failed.");

        status = co_await fileStreamSPtr->FlushAsync();
        TraceAndThrowOnFailure(status, L"CreateCheckpointFileAsync: Flush file stream failed.");
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(
        NT_SUCCESS(status), 
        "{0}:{1}:{2} CreateCheckpointFileAsync: FileStream CloseAsync failed. Status: {3}", 
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    if (exceptionSPtr != nullptr)
    {
        ktl::Exception exp = exceptionSPtr->Info;

        Common::Assert::CodingError(
            "{0}:{1}:{2} Api: CreateCheckpointFileAsync failed. Error Code: {3}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            exp.GetStatus());

        throw exp;
    }

    co_return;
}

Awaitable<bool> TestStateProvider::OpenCheckpointFileAsync(__in KWString & filePath)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KBlockFile::SPtr FileSPtr = nullptr;
    io::KFileStream::SPtr fileStreamSPtr = nullptr;

    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        FALSE,
        KBlockFile::eOpenExisting,
        FileSPtr,
        nullptr,
        GetThisAllocator(),
        TEST_STATE_PROVIDER_TAG);

    // If trying to open the file and the file doesn't exist, return 
    // Other errors should throw.
    if (status == STATUS_OBJECT_NAME_NOT_FOUND) 
    {
        co_return false;
    }

    TraceAndThrowOnFailure(status, L"OpenCheckpointFileAsync: Open file failed.");

    KFinally([&] {FileSPtr->Close(); });

    status = io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), TEST_STATE_PROVIDER_TAG);
    TraceAndThrowOnFailure(status, L"OpenCheckpointFileAsync: Create file stream failed.");

    status = co_await fileStreamSPtr->OpenAsync(*FileSPtr);
    TraceAndThrowOnFailure(status, L"OpenCheckpointFileAsync: Open file stream failed.");

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
        BlockHandle::SPtr footerHandle = nullptr;
        LONG64 tmpSize = fileStreamSPtr->GetLength();

        ASSERT_IFNOT(
            tmpSize >= 0, 
            "{0}:{1}:{2} OpenCheckpointFileAsync: Get file size from file stream, it should not be negative number. FileSize: {3}.", 
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            tmpSize);

        ULONG64 fileSize = static_cast<ULONG64>(tmpSize);
        ULONG32 footerSize = FileFooter::SerializedSize();
        if (fileSize < (footerSize + sizeof(ULONG64)))
        {
            // File is too small to contain even a footer.
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }

        // Create a block handle for the footer.
        // Note that size required is footer size + the checksum (sizeof(ULONG64))
        ULONG64 footerStartingOffset = fileSize - footerSize - sizeof(ULONG64);
        status = BlockHandle::Create(
            footerStartingOffset,
            footerSize,
            GetThisAllocator(),
            footerHandle);
        TraceAndThrowOnFailure(status, L"OpenCheckpointFileAsync: Create footer BlockHandle failed.");

        FileBlock<FileFooter::SPtr>::DeserializerFunc deserializerFunction(&FileFooter::Read);
        FileFooter::SPtr footerSPtr = (co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(*fileStreamSPtr, *footerHandle, deserializerFunction, GetThisAllocator(), CancellationToken::None));

        ASSERT_IFNOT(
            footerSPtr->Version == 0,
            "{0}:{1}:{2} OpenCheckpointFileAsync: FooterVersion should be 0. FooterVersion: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            footerSPtr->Version);

        // Read and validate the properties section.
        BlockHandle::SPtr propertiesHandle = footerSPtr->PropertiesHandle;

        TestStateProviderProperties::SPtr testStateProviderProperties_;
        TestStateProviderProperties::Create(GetThisAllocator(), testStateProviderProperties_);
        FileBlock<TestStateProviderProperties::SPtr>::DeserializerFunc propertiesDeserializerFunction(&FileProperties::Read);
        testStateProviderProperties_ = (co_await FileBlock<TestStateProviderProperties::SPtr>::ReadBlockAsync(*fileStreamSPtr, *propertiesHandle, propertiesDeserializerFunction, GetThisAllocator(), CancellationToken::None));

        ASSERT_IFNOT(
            testStateProviderProperties_->TestStateProviderInfoHandle->Offset == 0,
            "{0}:{1}:{2} OpenCheckpointFileAsync: Properties offset should be 0. Offset: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            testStateProviderProperties_->TestStateProviderInfoHandle->Offset);

        ULONG bytesToRead = static_cast<ULONG>(testStateProviderProperties_->TestStateProviderInfoHandle->Size + sizeof(ULONG64));
        KBuffer::SPtr itemStream = nullptr;
        status = KBuffer::Create(bytesToRead, itemStream, GetThisAllocator(), TEST_STATE_PROVIDER_TAG);
        TraceAndThrowOnFailure(status, L"OpenCheckpointFileAsync: Create buffer failed.");
        fileStreamSPtr->SetPosition(testStateProviderProperties_->TestStateProviderInfoHandle->Offset);

        ULONG bytesRead = 0;
        status = co_await fileStreamSPtr->ReadAsync(*itemStream, bytesRead, 0, bytesToRead);
        ASSERT_IFNOT(
            bytesRead == bytesToRead,
            "{0}:{1}:{2} OpenCheckpointFileAsync: Bytes read should be as expected. bytesRead: {3}, bytesToRead: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            bytesRead,
            bytesToRead);

        TraceAndThrowOnFailure(status, L"OpenCheckpointFileAsync: FileStream read failed.");
        BinaryReader itemReader(*itemStream, GetThisAllocator());

        LONG64 readVersion;
        itemReader.Read(readVersion);
        currentVersion_ = readVersion;

        KString::SPtr readValue;
        itemReader.Read(readValue, UTF16);
        currentValue_ = readValue;

        ULONG64 checksum;
        itemReader.Read(checksum);

        ULONG64 recordChecksum = CRC64::ToCRC64(*itemStream, 0, static_cast<ULONG32>(testStateProviderProperties_->TestStateProviderInfoHandle->Size));
        ASSERT_IFNOT(
            checksum == recordChecksum,
            "{0}:{1}:{2} OpenCheckpointFileAsync: Checksum is incorrect. checksum: {3}, recordChecksum: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            checksum,
            recordChecksum);
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(
        NT_SUCCESS(status), 
        "{0}:{1}:{2} OpenCheckpointFileAsync: FileStream CloseAsync failed. Status: {3}", 
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        status);

    if (exceptionSPtr != nullptr)
    {
        ktl::Exception exp = exceptionSPtr->Info;

        Common::Assert::CodingError(
            "{0}:{1}:{2} Api: OpenCheckpointFileAsync failed. Error Code: {3}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            exp.GetStatus());

        throw exp;
    }

    co_return true;
}

KString::CSPtr TestStateProvider::GetCheckpointFilePath(__in KStringView const & fileName)
{
    BOOLEAN boolStatus;

    KString::SPtr filePath = nullptr;
    NTSTATUS status = KString::Create(filePath, GetThisAllocator(), *workFolder_);
    TraceAndThrowOnFailure(status, L"GetCheckpointFilePath: Create workFolder string failed.");

    boolStatus = filePath->Concat(Common::Path::GetPathSeparatorWstr().c_str());
    ASSERT_IFNOT(
        boolStatus == TRUE,
        "{0}:{1}:{2} GetCheckpointFilePath: Concat file path failed.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    boolStatus = filePath->Concat(fileName);
    ASSERT_IFNOT(
        boolStatus == TRUE,
        "{0}:{1}:{2} GetCheckpointFilePath: Concat file name failed.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);
    return filePath.RawPtr();
}

void TestStateProvider::ThrowIfNotReadable() const
{
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus;

    ASSERT_IFNOT(
        replicatorWRef_ != nullptr,
        "{0}:{1}:{2} ThrowIfNotReadable: replicatorWRef should not be nullptr, current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));
    ASSERT_IFNOT(
        replicatorWRef_->IsAlive(),
        "{0}:{1}:{2} ThrowIfNotReadable: replicatorWRef is not alive, current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    auto replicatorSPtr = replicatorWRef_->TryGetTarget();
    auto partitionSPtr = replicatorSPtr->get_StatefulPartition();
    NTSTATUS status = partitionSPtr->GetReadStatus(&readStatus);
    TraceAndThrowOnFailure(status, L"ThrowIfNotReadable: GetReadStatus.");

    if (readStatus == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED)
    {
        return;
    }

    throw Exception(SF_STATUS_NOT_READY);
}

void TestStateProvider::TraceAndThrowOnFailure(
    __in NTSTATUS status,
    __in wstring const & traceInfo) const
{
    if (!NT_SUCCESS(status)) 
    { 
        Trace.WriteError(
            TraceComponent,
            "{0}:{1}:{2} {3} status: {4}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            traceInfo,
            status);
        throw ktl::Exception(status); 
    } 
}

Awaitable<void> TestStateProvider::SafeFileReplaceAsync(
    __in KString const & checkpointFileName,
    __in KString const & temporaryCheckpointFileName,
    __in KString const & backupCheckpointFileName,
    __in CancellationToken const & cancellationToken,
    __in KAllocator & allocator)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    // Delete previous backup, if it exists.
    Common::ErrorCode errorCode;
    if (Common::File::Exists(static_cast<LPCWSTR>(backupCheckpointFileName)))
    {
        errorCode = Common::File::Delete2(static_cast<LPCWSTR>(backupCheckpointFileName));
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }

    // Previous replace could have failed in the middle before the next checkpoint file got renamed to current.
    if (Common::File::Exists(static_cast<LPCWSTR>(checkpointFileName)) == false)
    {
        // Next checkpoint file is valid.  Move it to be current.
        errorCode = Common::File::Move(static_cast<LPCWSTR>(temporaryCheckpointFileName), static_cast<LPCWSTR>(checkpointFileName), false);
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }

        co_return;
    }

    // Current exists, so we must have gotten here only after writing a valid next checkpoint file.
    errorCode = Common::File::Move(
        static_cast<LPCWSTR>(checkpointFileName),
        static_cast<LPCWSTR>(backupCheckpointFileName),
        false);
    if (errorCode.IsSuccess() == false)
    {
        throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
    }

    errorCode = Common::File::Move(
        static_cast<LPCWSTR>(temporaryCheckpointFileName),
        static_cast<LPCWSTR>(checkpointFileName),
        false);
    if (errorCode.IsSuccess() == false)
    {
        throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
    }

    // Delete the backup, now that we've safely replaced the file.
    if (Common::File::Exists(static_cast<LPCWSTR>(backupCheckpointFileName)))
    {
        errorCode = Common::File::Delete2(static_cast<LPCWSTR>(backupCheckpointFileName));
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
        }
    }

    co_return;
}

Awaitable<void> TestStateProvider::RecoverCheckpointAsync(__in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    ASSERT_IFNOT(
        lifeState_ == Open,
        "{0}:{1}:{2} RecoverCheckpointAsync: LifeState should be Open, current LifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    KWString filePath = KWString(GetThisAllocator(), currentCheckpointFilePath_->ToUNICODE_STRING());

    if (hasPersistedState_)
    {
        // CurrentValue is already read from init params.
        // If there is a checkpoint value and version will be reset.
        // Note that below function simply returns if there is no checkpoint to recover.
        bool isCheckpointRecovered = co_await OpenCheckpointFileAsync(filePath);
        if (isCheckpointRecovered == false)
        {
            co_await CreateCheckpointFileAsync(*currentValue_, currentVersion_, true);
        }
    }

    // Update the last CheckpointData to reflect the recovered state.
    // If there is no checkpoint these values will be default.
    {
        VersionedState::CSPtr versionedState = VersionedState::Create(currentVersion_, *currentValue_, GetThisAllocator());
        lastCheckpointedData_.Put(Ktl::Move(versionedState));
    }

    // TODO: This is a temporary fix for the copy before checkpoint case.
    // True fix should serialize the prepare checkpoint lsn as part of the checkpoint.
    performCheckpointLSN_ = 0;
    
    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: RecoverCheckpointAsync. Version: {3} Value: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        currentVersion_,
        ToStringLiteral(*currentValue_));

    co_return;
}

Awaitable<void> TestStateProvider::BackupCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: BackupCheckpointAsync. Directory: {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(backupDirectory));

    KString::SPtr backupFilePath = KPath::Combine(backupDirectory, CurrentCheckpointPrefix, GetThisAllocator());

    Common::ErrorCode code = Common::File::Copy(static_cast<LPCWSTR>(*currentCheckpointFilePath_), static_cast<LPCWSTR>(*backupFilePath));
    if (code.IsSuccess() == false)
    {
        throw Exception(StatusConverter::Convert(code.ToHResult()));
    }

    co_return;
}

Awaitable<void> TestStateProvider::RestoreCheckpointAsync(
    __in KString const & backupDirectory,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    KString::SPtr backupFilePath = KPath::Combine(backupDirectory, CurrentCheckpointPrefix, GetThisAllocator());

    Common::ErrorCode code =  Common::File::Move(static_cast<LPCWSTR>(*backupFilePath), static_cast<LPCWSTR>(*currentCheckpointFilePath_), false);
    if (!isBackCompatibleTest_ && code.IsSuccess() == false)
    {
        throw Exception(StatusConverter::Convert(code.ToHResult()));
    }

    co_return;
}

Awaitable<void> TestStateProvider::RemoveStateAsync(
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    ASSERT_IFNOT(
        lifeState_ == None,
        "{0}:{1}:{2} RemoveStateAsync: lifeState should be None. Current lifeState: {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    KWString folderPath = KWString(GetThisAllocator(), workFolder_->ToUNICODE_STRING());

    Common::ErrorCode errorCode = Common::Directory::Delete(static_cast<LPCWSTR>(folderPath), true);
    if (!errorCode.IsSuccess())
    {
        throw Exception(StatusConverter::Convert(errorCode.ToHResult()));
    }

    checkpointState_ = Removed;
    co_await suspend_never{};
    co_return;
}

OperationDataStream::SPtr TestStateProvider::GetCurrentState()
{
    VersionedState::CSPtr snappedChecpointData = lastCheckpointedData_.Get();

    ASSERT_IF(
        snappedChecpointData->Value == nullptr,
        "{0}:{1}:{2} GetCurrentState: Last checkpointed data cannot be nullptr",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    return TestOperationDataStream::Create(
        *PartitionedReplicaIdentifier,
        stateProviderId_,
        performCheckpointLSN_,
        *snappedChecpointData,
        GetThisAllocator()).RawPtr();
}

Awaitable<void> TestStateProvider::BeginSettingCurrentStateAsync()
{
    // Only one copy per lifetime
    ASSERT_IFNOT(
        consumedCopyCheckpointLSN_ == FABRIC_INVALID_SEQUENCE_NUMBER,
        "{0}:{1}:{2} BeginSettingCurrentState: Current consumedCopyCheckpointLSN: {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        consumedCopyCheckpointLSN_);

    // Api life time reset
    isEndSettingCurrentStateCalled = false;
    isBeginSettingCurrentStateCalled_ = true;

    currentVersion_ = StartingVersionNumber;
    currentValue_ = nullptr;
    co_return;
}

Awaitable<void> TestStateProvider::SetCurrentStateAsync(
    __in LONG64 stateRecordNumber,
    __in OperationData const & copyOperationData,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    OperationData::CSPtr operationData(&copyOperationData);
            
    ASSERT_IFNOT(
        operationData->BufferCount == 1 || operationData->BufferCount == 2,
        "{0}:{1}:{2} Api: SetCurrentStateAsync: Unexpected buffer count. State Record Number: {3} BufferCount: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        stateRecordNumber,
        operationData->BufferCount);

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: SetCurrentStateAsync. State Record Number: {3} BufferCount: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        stateRecordNumber,
        operationData->BufferCount);

    try 
    {
        // first operation has 2 buffers
        if (operationData->BufferCount == 2)
        {
            ASSERT_IFNOT(
                currentVersion_ == StartingVersionNumber,
                "{0}:{1}:{2} Api: Current must be starting: Current must be nullptr. State Record Number: {3}. Current: {4}",
                TracePartitionId,
                ReplicaId,
                StateProviderId,
                stateRecordNumber,
                currentVersion_);

            {
                BinaryReader reader(*(*operationData)[0], GetThisAllocator());
                reader.Read(consumedCopyCheckpointLSN_);

                // Uncheckpointed state provider will copy FABRIC_INVALID_SEQUENCE_NUMBER.
                ASSERT_IFNOT(
                    consumedCopyCheckpointLSN_ >= FABRIC_INVALID_SEQUENCE_NUMBER,
                    "{0}:{1}:{2} SetCurrentStateAsync: Current consumedCopyCheckpointLSN: {3}",
                    TracePartitionId,
                    ReplicaId,
                    StateProviderId,
                    consumedCopyCheckpointLSN_);
            }

            {
                BinaryReader reader(*(*operationData)[1], GetThisAllocator());
                reader.Read(currentVersion_);

                ASSERT_IFNOT(
                    currentVersion_ >= StartingVersionNumber,
                    "{0}:{1}:{2} SetCurrentStateAsync: currentVersion: {3}, StartingVersionNumber: {4}.",
                    TracePartitionId,
                    ReplicaId,
                    StateProviderId,
                    currentVersion_,
                    StartingVersionNumber);
            }
        }
        else if (operationData->BufferCount == 1)
        {
            ASSERT_IFNOT(
                currentValue_ == nullptr,
                "{0}:{1}:{2} Api: SetCurrentStateAsync: Current must be nullptr. State Record Number: {3}",
                TracePartitionId,
                ReplicaId,
                StateProviderId,
                stateRecordNumber);

            BinaryReader reader(*(*operationData)[0], GetThisAllocator());
            reader.Read(currentValue_, UTF16);
        }
    }
    catch (Exception & e)
    {
        Common::Assert::CodingError(
            "{0}:{1}:{2} Api: SetCurrentStateAsync failed. State Record Number: {3} BufferCount: {4} Error Code: {5}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            stateRecordNumber,
            operationData->BufferCount,
            e.GetStatus());

        throw;
    }
    catch (...)
    {
        Common::Assert::CodingError(
            "{0}:{1}:{2} Api: SetCurrentStateAsync failed. State Record Number: {3} BufferCount: {4}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            stateRecordNumber,
            operationData->BufferCount);

        throw;
    }

    co_await suspend_never{};
    co_return;
}

Awaitable<void> TestStateProvider::EndSettingCurrentStateAsync(
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    // Resurrection case
    if (currentValue_ == nullptr)
    {
        currentValue_ = ReadInitializationParamaters();
    }

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2} Api: EndSettingCurrentStateAsync. Version: {3} Value: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        currentVersion_,
        ToStringLiteral(*currentValue_));

    ASSERT_IFNOT(
        consumedCopyCheckpointLSN_ >= FABRIC_INVALID_SEQUENCE_NUMBER,
        "{0}:{1}:{2} EndSettingCurrentStateAsync: consumedCopyCheckpointLSN: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        consumedCopyCheckpointLSN_);
    ASSERT_IFNOT(
        currentVersion_ >= StartingVersionNumber,
        "{0}:{1}:{2} EndSettingCurrentStateAsync: currentVersion: {3}, StartingVersionNumber: {4}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        currentVersion_,
        StartingVersionNumber);
    ASSERT_IFNOT(
        isBeginSettingCurrentStateCalled_,
        "{0}:{1}:{2} EndSettingCurrentStateAsync: IsBeginSettingCurrentStateCalled if false.",
        TracePartitionId,
        ReplicaId,
        StateProviderId);

    isEndSettingCurrentStateCalled = true;
    isBeginSettingCurrentStateCalled_ = false;

    co_await suspend_never{};
    co_return;
}

Awaitable<void> TestStateProvider::PrepareForRemoveAsync(
    __in TransactionBase const& transactionBase,
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(transactionBase);
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    co_await suspend_never{};
    co_return;
}

Awaitable<OperationContext::CSPtr> TestStateProvider::ApplyAsync(
    __in LONG64 logicalSequenceNumber,
    __in TransactionBase const & transactionBase,
    __in ApplyContext::Enum applyContext,
    __in_opt OperationData const * const metadataPtr,
    __in_opt OperationData const * const dataPtr)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ApplyContext::Enum roleType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::ROLE_MASK);
    ApplyContext::Enum operationType = static_cast<ApplyContext::Enum>(applyContext & ApplyContext::OPERATION_MASK);

    ASSERT_IF(
        (operationType == ApplyContext::UNDO),
        "{0}:{1}:{2} ApplyAsync: UNDO operation is not supported. OperationType: {3} LSN: {4}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(operationType),
        logicalSequenceNumber);

    ASSERT_IFNOT(
        dataPtr != nullptr,
        "{0}:{1}:{2} Redo or undo data can never be null. LSN {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        logicalSequenceNumber);

    if (operationType == ApplyContext::FALSE_PROGRESS)
    {
        ASSERT_IFNOT(
            (roleType == ApplyContext::SECONDARY),
            "{0}:{1}:{2} ApplyAsync: FALSE_PROGRESS operation must be with roleType SECONDARY. {3}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<int>(roleType));

        ASSERT_IFNOT(
            lifeState_ == Idle,
            "{0}:{1}:{2} ApplyAsync: FALSE_PROGRESS operation must be with lifeState_ Idle. LifeState: {3}.",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            static_cast<int>(lifeState_));

        UndoOperationData::CSPtr undoOperationData;
        status = UndoOperationData::Create(*PartitionedReplicaIdentifier, dataPtr, GetThisAllocator(), undoOperationData);
        THROW_ON_FAILURE(status);

        currentValue_ = const_cast<KString *>(undoOperationData->Value.RawPtr());
        currentVersion_ = undoOperationData->Version;

        Trace.WriteInfo(
            TraceComponent,
            "{0}:{1}:{2}:{3} ApplyAsync: ContextRole: {4} ContextType: {5} Value: {6} Version: {7} LSN: {8}",
            TracePartitionId,
            ReplicaId,
            StateProviderId,
            ToStringLiteral(*name_),
            static_cast<int>(roleType),
            static_cast<int>(operationType),
            ToStringLiteral(*currentValue_),
            currentVersion_,
            logicalSequenceNumber);

        co_return nullptr;
    }

    // Use apply context to make more specific.
    ASSERT_IFNOT(
        (lifeState_ == Open) || (lifeState_ == Idle) || (lifeState_ == Secondary) || (lifeState_ == Primary),
        "{0}:{1}:{2} ApplyAsync: Current lifeState: {3}.",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        static_cast<int>(lifeState_));

    ASSERT_IFNOT(
        dataPtr->BufferCount == 1,
        "{0}:{1}:{2} Expected only 1 apply data buffer. Found {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        dataPtr->BufferCount);

    try
    {
        KBuffer::CSPtr & buffer = (*dataPtr)[0];
        BinaryReader reader(*buffer, GetThisAllocator());
        reader.Read(currentValue_, UTF16);
    }
    catch (Exception & e)
    {
        Common::Assert::CodingError(
            "{0}: SPId: {1}. Api: ApplyAsync failed. Txn: {2} LSN: {3} BufferCount: {4} ApplyContext: {5} Error Code: {6}",
            TraceComponent,
            stateProviderId_,
            transactionBase.TransactionId,
            logicalSequenceNumber,
            dataPtr->BufferCount,
            static_cast<int>(applyContext),
            e.GetStatus());

        throw;
    }
    
    // NOTE: Do not change this
    // This is a hard dependency on the fabric test where if a commit is successful, we expect the new version to be the transaction's commit sequence number
    currentVersion_ = transactionBase.CommitSequenceNumber;

    Trace.WriteInfo(
        TraceComponent,
        "{0}:{1}:{2}:{3} ApplyAsync: ContextRole: {4} ContextType: {5} Value: {6} Version: {7} LSN: {8}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        ToStringLiteral(*name_),
        static_cast<int>(roleType),
        static_cast<int>(operationType),
        ToStringLiteral(*currentValue_),
        currentVersion_,
        logicalSequenceNumber);

    co_return nullptr;
}

void TestStateProvider::Unlock(__in OperationContext const & operationContext)
{
    UNREFERENCED_PARAMETER(operationContext);
}

KString::SPtr TestStateProvider::ReadInitializationParamaters()
{
    ASSERT_IF(
        initializationParameters_ == nullptr,
        "{0}:{1}:{2} TestStateprovider is expected to have initialization context when created from fabric test",
        TracePartitionId,
        ReplicaId,
        StateProviderId);
    ASSERT_IFNOT(
        initializationParameters_->BufferCount == 1,
        "{0}:{1}:{2} TestStateprovider is expected to have 1 buffer if non-null and not {3}",
        TracePartitionId,
        ReplicaId,
        StateProviderId,
        initializationParameters_->BufferCount);
    BinaryReader reader(*(*initializationParameters_)[0], GetThisAllocator());

    KString::SPtr output;
    reader.Read(output, UTF16);
    return output;
}

TestStateProvider::TestStateProvider(
    __in FactoryArguments const & factoryArguments,
    __in bool isBackCompatibleTest)
    : KObject()
    , KShared()
    , isBackCompatibleTest_(isBackCompatibleTest)
    , name_()
    , stateProviderId_(factoryArguments.StateProviderId)
    , initializationParameters_(factoryArguments.InitializationParameters)
    , partitionId_(factoryArguments.PartitionId)
    , replicaId_(factoryArguments.ReplicaId)
    , PartitionedReplicaTraceComponent(*PartitionedReplicaId::Create(factoryArguments.PartitionId, factoryArguments.ReplicaId, GetThisAllocator()))
    , lock_()
    , serializer_(GetThisAllocator())
    , replicatorWRef_(nullptr)
    , lastPrepareCheckpointData_(nullptr)
    , lastCheckpointedData_(nullptr)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = KString::Create(currentValue_, GetThisAllocator(), L"");
    TraceAndThrowOnFailure(status, L"Constructor: Create currentValue string failed.");

    status = AsyncLock::Create(GetThisAllocator(), TEST_STATE_PROVIDER_TAG, lock_);
    TraceAndThrowOnFailure(status, L"Constructor: Create ReaderWriterAsyncLock failed.");

    status = KUri::Create(factoryArguments.Name->Get(KUriView::eRaw), GetThisAllocator(), name_);
    TraceAndThrowOnFailure(status, L"Constructor: Create name failed.");

    if (initializationParameters_ != nullptr && initializationParameters_->BufferCount == 1)
    {
        currentValue_ = ReadInitializationParamaters();
    }
}

TestStateProvider::~TestStateProvider()
{
    // We cannot assert the lifeState here due to injected crashes.
}
