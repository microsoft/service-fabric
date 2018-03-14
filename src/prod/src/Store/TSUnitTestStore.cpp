// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("TSUnitTestStore");

class TSUnitTestStore::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in TSUnitTestStore & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CloseAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        auto operation = owner_.ktlLogger_->BeginClose(
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnKtlCloseComplete(operation, false); },
            thisSPtr);
        this->OnKtlCloseComplete(operation, true);
    }

private:

    void OnKtlCloseComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_.ktlLogger_->EndClose(operation);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "KtlLogger close failed: {0}", error);
        }

        error = ErrorCode::FirstError(error, owner_.localStore_->Terminate());

        owner_.localStore_->Drain();

        this->TryComplete(thisSPtr, error);
    }

private:
    TSUnitTestStore & owner_;
};

TSUnitTestStore::TSUnitTestStore(wstring const & workingDirectory) 
    : workingDirectory_(workingDirectory)
    , mockComponentRoot_(make_shared<ComponentRoot>())
    , ktlLogger_()
    , localStore_()
{
}

TSUnitTestStore::~TSUnitTestStore()
{
}
        
IReplicatedStoreUPtr TSUnitTestStore::Create(wstring const & workingDirectory)
{
    return unique_ptr<TSUnitTestStore>(new TSUnitTestStore(workingDirectory));
}

ErrorCode TSUnitTestStore::InitializeLocalStoreForUnittests(bool databaseShouldExist)
{
    UNREFERENCED_PARAMETER(databaseShouldExist);

    auto workingDirectory = Path::Combine(workingDirectory_, L"TSUnitTestStore");
    auto nodeConfig = make_shared<FabricNodeConfig>();
    ktlLogger_ = make_shared<KtlLogger::KtlLoggerNode>(*mockComponentRoot_, workingDirectory, nodeConfig);

    ManualResetEvent openEvent(false);
    auto operation = ktlLogger_->BeginOpen(TimeSpan::MaxValue, [&openEvent](AsyncOperationSPtr const &) { openEvent.Set(); }, AsyncOperationSPtr());

    openEvent.WaitOne();

    auto error = ktlLogger_->EndOpen(operation);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "KtlLogger open failed: {0}", error);

        return error;
    }

    auto replicatedStoreSettings = make_unique<TSReplicatedStoreSettings>(workingDirectory);
    localStore_ = make_shared<TSLocalStore>(move(replicatedStoreSettings), ktlLogger_);

    error = localStore_->Initialize(L"TSUnitTestStore", Federation::NodeId(LargeInteger(0, 42)));

    if (error.IsSuccess())
    {
        error = FabricComponent::Open();
    }

    return error;
}

::FABRIC_TRANSACTION_ISOLATION_LEVEL TSUnitTestStore::GetDefaultIsolationLevel()
{
    return localStore_->Test_GetReplicatedStore()->GetDefaultIsolationLevel();
}

ErrorCode TSUnitTestStore::CreateTransaction(__out TransactionSPtr & txSPtr)
{
    return localStore_->Test_GetReplicatedStore()->CreateTransaction(txSPtr);
}

FILETIME TSUnitTestStore::GetStoreUtcFILETIME()
{
    return localStore_->Test_GetReplicatedStore()->GetStoreUtcFILETIME();
}

ErrorCode TSUnitTestStore::CreateEnumerationByTypeAndKey(
    __in TransactionSPtr const & txSPtr,
    __in std::wstring const & type,
    __in std::wstring const & keyStart,
    __out EnumerationSPtr & enumerationSPtr)
{
    return localStore_->Test_GetReplicatedStore()->CreateEnumerationByTypeAndKey(
        txSPtr,
        type,
        keyStart,
        enumerationSPtr);
}

ErrorCode TSUnitTestStore::ReadExact(
    __in TransactionSPtr const & txSPtr,
    __in std::wstring const & type,
    __in std::wstring const & keyStart,
    __out std::vector<byte> & value,
    __out __int64 & operationLsn)
{
    return localStore_->Test_GetReplicatedStore()->ReadExact(
        txSPtr,
        type,
        keyStart,
        value,
        operationLsn);
}

ErrorCode TSUnitTestStore::ReadExact(
    __in TransactionSPtr const & txSPtr,
    __in std::wstring const & type,
    __in std::wstring const & keyStart,
    __out std::vector<byte> & value,
    __out __int64 & operationLsn,
    __out FILETIME & lastModified)
{
    return localStore_->Test_GetReplicatedStore()->ReadExact(
        txSPtr,
        type,
        keyStart,
        value,
        operationLsn,
        lastModified);
}

bool TSUnitTestStore::GetIsActivePrimary() const
{
    return localStore_->Test_GetReplicatedStore()->GetIsActivePrimary();
}

ErrorCode TSUnitTestStore::GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER & lsn)
{
    return localStore_->Test_GetReplicatedStore()->GetLastCommittedSequenceNumber(lsn);
}

ErrorCode TSUnitTestStore::CreateSimpleTransaction(
    __out TransactionSPtr & txSPtr)
{
    return localStore_->Test_GetReplicatedStore()->CreateSimpleTransaction(txSPtr);
}

ErrorCode TSUnitTestStore::Insert(
    __in TransactionSPtr const & txSPtr,
    __in std::wstring const & type,
    __in std::wstring const & key,
    __in void const * value,
    __in size_t valueSizeInBytes)
{
    return localStore_->Test_GetReplicatedStore()->Insert(
        txSPtr,
        type,
        key,
        value,
        valueSizeInBytes);
}

ErrorCode TSUnitTestStore::Update(
    __in TransactionSPtr const & txSPtr,
    __in std::wstring const & type,
    __in std::wstring const & key,
    __in _int64 checkOperationNumber,
    __in std::wstring const & newKey,
    __in_opt void const * newValue,
    __in size_t valueSizeInBytes)
{
    return localStore_->Test_GetReplicatedStore()->Update(
        txSPtr,
        type,
        key,
        checkOperationNumber,
        newKey,
        newValue,
        valueSizeInBytes);
}

ErrorCode TSUnitTestStore::Delete(
    __in TransactionSPtr const & txSPtr,
    __in std::wstring const & type,
    __in std::wstring const & key,
    __in _int64 checkOperationNumber)
{
    return localStore_->Test_GetReplicatedStore()->Delete(
        txSPtr,
        type,
        key,
        checkOperationNumber);
}

ErrorCode TSUnitTestStore::SetThrottleCallback(ThrottleCallback const & callback)
{
    return localStore_->Test_GetReplicatedStore()->SetThrottleCallback(callback);
}

ErrorCode TSUnitTestStore::GetCurrentEpoch(__out FABRIC_EPOCH & epoch) const
{
    return localStore_->Test_GetReplicatedStore()->GetCurrentEpoch(epoch);
}
    
ErrorCode TSUnitTestStore::UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const & settings)
{
    return localStore_->Test_GetReplicatedStore()->UpdateReplicatorSettings(settings);
}

ErrorCode TSUnitTestStore::UpdateReplicatorSettings(Reliability::ReplicationComponent::ReplicatorSettingsUPtr const & settings)
{
    return localStore_->Test_GetReplicatedStore()->UpdateReplicatorSettings(settings);
}

AsyncOperationSPtr TSUnitTestStore::BeginOpen(
    ::FABRIC_REPLICA_OPEN_MODE, 
    ComPointer<::IFabricStatefulServicePartition> const &,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCodeValue::NotImplemented,
        callback,
        parent);
}

ErrorCode TSUnitTestStore::EndOpen(
    AsyncOperationSPtr const & asyncOperation, 
    __out ComPointer<::IFabricReplicator> &)
{
    return CompletedAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr TSUnitTestStore::BeginChangeRole(
    ::FABRIC_REPLICA_ROLE,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCodeValue::NotImplemented,
        callback,
        parent);
}

ErrorCode TSUnitTestStore::EndChangeRole(
    AsyncOperationSPtr const & asyncOperation, 
    __out std::wstring &)
{
    return CompletedAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr TSUnitTestStore::BeginClose(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode TSUnitTestStore::EndClose(
    AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void TSUnitTestStore::Abort()
{
    this->Close();
}

ErrorCode TSUnitTestStore::OnClose()
{
    ManualResetEvent closeEvent(false);
    auto operation = this->BeginClose([&closeEvent](AsyncOperationSPtr const &) { closeEvent.Set(); }, AsyncOperationSPtr());

    closeEvent.WaitOne();

    return this->EndClose(operation);
}
