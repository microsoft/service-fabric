// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Transport;
using namespace TestCommon;
using namespace ktl;

StringLiteral const TraceSource("TStoreService");
wstring const TStoreService::DefaultServiceType = L"TStoreServiceType";
KStringView const StateProviderTypeName = L"TStoreStateProvider";

TStoreService::TStoreService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const& serviceName,
    Federation::NodeId nodeId,
    wstring const& workingDir,
    TestSession & testSession,
    wstring const& initDataStr,
    wstring const& serviceType,
    wstring const& appId)
    : TXRService(partitionId, replicaId, serviceName, nodeId, workingDir, testSession, initDataStr, serviceType, appId)
{
    TestSession::WriteNoise(TraceSource, GetPartitionId(), "TStoreService created with name: {0} initialization-data:'{1}'", GetServiceName(), initDataStr);
}

TStoreService::~TStoreService()
{
    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Dtor called for Service at {0}", GetNodeId());
}

wstring TStoreService::GetStateProviderName()
{
    return L"fabrictest:/tstoresp/";
}

ErrorCode TStoreService::Put(
    __int64 key,
    wstring const& value,
    FABRIC_SEQUENCE_NUMBER currentVersion,
    wstring serviceName,
    FABRIC_SEQUENCE_NUMBER & newVersion,
    ULONGLONG & dataLossVersion)
{
    newVersion = 0;
    wstring keyStateProvider = GetStateProviderName();

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Put called for Service. key {0}, value {1}, current version {2}", keyStateProvider, value, currentVersion);
    TestSession::FailTestIf(GetScheme() != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Put: Only INT64 scheme is supported");

    KUriView stateProviderName(&keyStateProvider[0]);
    ErrorCode error;

    if (!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCodeValue::FabricTestIncorrectServiceLocation;
    }

    TxnReplicator::ITransactionalReplicator::SPtr txnReplicator = GetTxnReplicator();
    error = InvokeKtlApiNoExcept([&]
    {
        FABRIC_EPOCH e;
        NTSTATUS status = txnReplicator->GetCurrentEpoch(e);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        dataLossVersion = e.DataLossNumber;
        return status;
    });

    if (!error.IsSuccess())
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Get failed to GetCurrentEpoch. StateProvider {0} value {1} with error {2}",
            keyStateProvider,
            value,
            error);

        return error;
    }

    KAllocator & allocator = GetAllocator();
    KString::SPtr kValue = KString::Create(&value[0], allocator);

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    // Get IStateProvider
    NTSTATUS status = txnReplicator->Get(stateProviderName, stateProvider);

    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Put failed to Get StateProvider {0} value {1} with error {2}",
            keyStateProvider,
            value,
            error);
        return error;
    }

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        TxnReplicator::Transaction::SPtr transaction;
        KFinally([&] {transaction->Dispose(); });

        error = InvokeKtlApiNoExcept([&]
        {
            return txnReplicator->CreateTransaction(transaction);
        });
        TestSession::FailTestIf(!error.IsSuccess(), "Failed to create transaction");

        // Create the state provider
        status = SyncAwait(txnReplicator->AddAsync(
            *transaction,
            stateProviderName,
            StateProviderTypeName));
        if (!NT_SUCCESS(status))
        {
            error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Put failed to Add StateProvider {0} value {1} with error {2}",
                keyStateProvider,
                value,
                error);
            return error;
        }
        
        status = SyncAwait(transaction->CommitAsync(FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout, ktl::CancellationToken::None));
        THROW_ON_FAILURE(status);

        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Put attempted to create new State Provider with name {0} and value {1} with error {2}",
            keyStateProvider,
            value,
            error);

        if (!error.IsSuccess())
        {
            return error;
        }

        // Get IStateProvider again
        status = txnReplicator->Get(stateProviderName, stateProvider);

        if (!NT_SUCCESS(status))
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Put failed to Get StateProvider {0} value {1} with error {2}",
                keyStateProvider,
                value,
                error);

            return error;
        }
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        KFinally([&] {transaction->Dispose(); });

        error = InvokeKtlApiNoExcept([&]
        {
            return txnReplicator->CreateTransaction(transaction);
        });
        TestSession::FailTestIf(!error.IsSuccess(), "Failed to create transaction");

        Data::TStore::IStore<__int64, KString::SPtr>* storeStateProvider = dynamic_cast<Data::TStore::IStore<__int64, KString::SPtr>*>(stateProvider.RawPtr());
        Data::TStore::IStoreTransaction<__int64, KString::SPtr>::SPtr storeTransaction = nullptr;
        storeStateProvider->CreateOrFindTransaction(*transaction, storeTransaction);

        error = InvokeKtlApi([&]
        {
            // Use a timeout here to ensure that if the operation cannot complete, we dont block this thread forever
            // If a thread is blocked for too long, fabric test fails with an error
            SyncAwait(storeStateProvider->AddAsync(*storeTransaction, key, kValue, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout, CancellationToken::None));
            NTSTATUS status = SyncAwait(transaction->CommitAsync(FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout, ktl::CancellationToken::None));
            THROW_ON_FAILURE(status);
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "AddAsync failed for state provider {0} with value {1} with error {2}",
                keyStateProvider,
                value,
                error);

            return error;
        }
    }

    return error;
}

ErrorCode TStoreService::Get(
    __int64 key,
    wstring serviceName,
    wstring & value,
    FABRIC_SEQUENCE_NUMBER & version,
    ULONGLONG & dataLossVersion)
{
    version = 0;
    wstring keyStateProvider = GetStateProviderName();

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Get called for Service. key {0}", keyStateProvider);
    TestSession::FailTestIf(GetScheme() != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Put: Only INT64 scheme is supported");

    KUriView stateProviderName(&keyStateProvider[0]);
    ErrorCode error;

    if (!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCodeValue::FabricTestIncorrectServiceLocation;
    }

    TxnReplicator::ITransactionalReplicator::SPtr txnReplicator = GetTxnReplicator();
    error = InvokeKtlApiNoExcept([&]
    {
        FABRIC_EPOCH e;
        NTSTATUS status = txnReplicator->GetCurrentEpoch(e);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        dataLossVersion = e.DataLossNumber;
        return status;
    });

    if (!error.IsSuccess())
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Get failed to GetCurrentDataLossNumber. StateProvider {0} value {1} with error {2}",
            keyStateProvider,
            value,
            error);

        return error;
    }

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    NTSTATUS status = txnReplicator->Get(stateProviderName, stateProvider);

    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Get: Get failed to Get StateProvider {0} value {1} with error {2}",
            keyStateProvider,
            value,
            error);

        return error;
    }

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Get: Get for StateProvider {0} failed as the state provider does not exist",
            keyStateProvider);

        return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        KFinally([&] {transaction->Dispose(); });
        error = InvokeKtlApiNoExcept([&]
        {
            return txnReplicator->CreateTransaction(transaction);
        });
        TestSession::FailTestIf(!error.IsSuccess(), "Failed to create transaction");

        Data::TStore::IStore<__int64, KString::SPtr>* storeStateProvider = dynamic_cast<Data::TStore::IStore<__int64, KString::SPtr>*>(stateProvider.RawPtr());
        Data::TStore::IStoreTransaction<__int64, KString::SPtr>::SPtr storeTransaction = nullptr;
        storeStateProvider->CreateOrFindTransaction(*transaction, storeTransaction);
        storeTransaction->ReadIsolationLevel = Data::TStore::StoreTransactionReadIsolationLevel::Enum::Snapshot;
        
        Data::KeyValuePair<LONG64, KString::SPtr> kvpair;
        KString::SPtr kValue;

        bool findValue = false;
        error = InvokeKtlApi([&]
        {
            findValue = SyncAwait(storeStateProvider->ConditionalGetAsync(*storeTransaction, key, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout, kvpair, CancellationToken::None));
            // TODO: This should be replaced with CommitAsync on the replicator's transaction
            storeTransaction->Close();
        });

        kValue = kvpair.Value;

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Get failed to ConditionalGetAsync StateProvider {0} value {1} with error {2}",
                keyStateProvider,
                value,
                error);

            return error;
        }

        if (!findValue)
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Get: Failed to find value for key {0}",
                key);

            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }

        WStringLiteral valueStringLiteral(
            (wchar_t *)*kValue,
            (wchar_t *)*kValue + kValue->Length());

        value.clear();
        value.append(valueStringLiteral.begin(), valueStringLiteral.end()); // Copy the value as the KString::SPtr could go out of scope
    }

    return error;
}

ErrorCode TStoreService::GetAll(
    wstring serviceName,
    vector<pair<__int64,wstring>> & kvpairs)
{
    UNREFERENCED_PARAMETER(serviceName);
    wstring keyStateProvider = GetStateProviderName();

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "GetAll called for Service.");

    ErrorCode error;
    KUriView stateProviderName(&keyStateProvider[0]);

    TxnReplicator::ITransactionalReplicator::SPtr txnReplicator = GetTxnReplicator();

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    NTSTATUS status = txnReplicator->Get(stateProviderName, stateProvider);

    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "GetAll: GetAll failed to Get StateProvider {0} with error {1}",
            keyStateProvider,
            error);

        return error;
    }

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "GetAll: GetAll for StateProvider {0} failed as the state provider does not exist",
            keyStateProvider);

        return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        KFinally([&] {transaction->Dispose(); });
        error = InvokeKtlApiNoExcept([&]
        {
            return txnReplicator->CreateTransaction(transaction);
        });
        TestSession::FailTestIf(!error.IsSuccess(), "Failed to create transaction");

        Data::TStore::IStore<__int64, KString::SPtr>* storeStateProvider = dynamic_cast<Data::TStore::IStore<__int64, KString::SPtr>*>(stateProvider.RawPtr());
        Data::TStore::IStoreTransaction<__int64, KString::SPtr>::SPtr storeTransaction = nullptr;
        storeStateProvider->CreateOrFindTransaction(*transaction, storeTransaction);
        storeTransaction->ReadIsolationLevel = Data::TStore::StoreTransactionReadIsolationLevel::Snapshot;
        KSharedPtr<Data::Utilities::IAsyncEnumerator<Data::KeyValuePair<__int64, Data::KeyValuePair<LONG64, KString::SPtr>>>> enumerator = nullptr;

        error = InvokeKtlApi([&]
        {
            enumerator = SyncAwait(storeStateProvider->CreateEnumeratorAsync(*storeTransaction));
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "GetAll failed to CreateEnumeratorAsync StateProvider {0} with error {1}",
                keyStateProvider,
                error);

            return error;
        }

        if (!enumerator)
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "GetAll: Failed to get enumerator");

            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }

        while (SyncAwait(enumerator->MoveNextAsync(CancellationToken::None)) != false)
        {
            Data::KeyValuePair<__int64, Data::KeyValuePair<LONG64, KString::SPtr>> current = enumerator->GetCurrent();
            KString::SPtr kValue = current.Value.Value;
            WStringLiteral valueStringLiteral(
                (wchar_t *)*kValue,
                (wchar_t *)*kValue + kValue->Length());

            wstring value;
            value.append(valueStringLiteral.begin(), valueStringLiteral.end()); // Copy the value as the KString::SPtr could go out of scope
            kvpairs.push_back(make_pair(current.Key, value));
        }

        error = InvokeKtlApi([&]
        {
            // TODO: This should be replaced with CommitAsync on the replicator's transaction
            storeTransaction->Close();
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "GetAll failed to close transaction with error {0} ",
                error);

            return error;
        }
    }

    return error;
}

ErrorCode TStoreService::GetKeys(
    wstring serviceName,
    __int64 first,
    __int64 last,
    vector<__int64> & keys)
{
    UNREFERENCED_PARAMETER(serviceName);
    wstring keyStateProvider = GetStateProviderName();

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "GetKeys called for Service.");

    ErrorCode error;
    KUriView stateProviderName(&keyStateProvider[0]);

    TxnReplicator::ITransactionalReplicator::SPtr txnReplicator = GetTxnReplicator();

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    NTSTATUS status = txnReplicator->Get(stateProviderName, stateProvider);

    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "GetKeys: GetKeys failed to Get StateProvider {0} with error {1}",
            keyStateProvider,
            error);

        return error;
    }

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "GetKeys: GetKeys for StateProvider {0} failed as the state provider does not exist",
            keyStateProvider);

        return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        KFinally([&] {transaction->Dispose(); });
        error = InvokeKtlApiNoExcept([&]
        {
            return txnReplicator->CreateTransaction(transaction);
        });
        TestSession::FailTestIf(!error.IsSuccess(), "Failed to create transaction");

        Data::TStore::IStore<__int64, KString::SPtr>* storeStateProvider = dynamic_cast<Data::TStore::IStore<__int64, KString::SPtr>*>(stateProvider.RawPtr());
        Data::TStore::IStoreTransaction<__int64, KString::SPtr>::SPtr storeTransaction = nullptr;
        storeStateProvider->CreateOrFindTransaction(*transaction, storeTransaction);
        storeTransaction->ReadIsolationLevel = Data::TStore::StoreTransactionReadIsolationLevel::Snapshot;
        KSharedPtr<Data::IEnumerator<__int64>> enumerator = nullptr;

        error = InvokeKtlApi([&]
        {
            enumerator = SyncAwait(storeStateProvider->CreateKeyEnumeratorAsync(*storeTransaction, first, last));
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "GetRange failed to CreateEnumeratorAsync StateProvider {0} with error {1}",
                keyStateProvider,
                error);

            return error;
        }

        if (!enumerator)
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "GetRange: Failed to get enumerator");

            return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
        }

        while (enumerator->MoveNext())
        {
            __int64 key = enumerator->Current();
            keys.push_back(key);
        }

        error = InvokeKtlApi([&]
        {
            // TODO: This should be replaced with CommitAsync on the replicator's transaction
            storeTransaction->Close();
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "GetKeys failed to close transaction with error {0} ",
                error);

            return error;
        }
    }

    return error;
}

ErrorCode TStoreService::Delete(
    __int64 key,
    FABRIC_SEQUENCE_NUMBER currentVersion,
    std::wstring serviceName,
    FABRIC_SEQUENCE_NUMBER & newVersion,
    ULONGLONG & dataLossVersion)
{
    UNREFERENCED_PARAMETER(currentVersion);
    newVersion = 0;
    wstring keyStateProvider = GetStateProviderName();

    TestSession::WriteNoise(TraceSource, GetPartitionId(), "Delete called for Service. key {0}, current version {1}", key, currentVersion);
    TestSession::FailTestIf(GetScheme() != FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE, "Delete: Only INT64 scheme is supported");

    KUriView stateProviderName(&keyStateProvider[0]);
    ErrorCode error;

    if (!VerifyServiceLocation(key, serviceName))
    {
        return ErrorCodeValue::FabricTestIncorrectServiceLocation;
    }

    TxnReplicator::ITransactionalReplicator::SPtr txnReplicator = GetTxnReplicator();
    error = InvokeKtlApiNoExcept([&]
    {
        FABRIC_EPOCH e;
        NTSTATUS status = txnReplicator->GetCurrentEpoch(e);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        dataLossVersion = e.DataLossNumber;
        return status;
    });

    if (!error.IsSuccess())
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Delete failed to GetCurrentDataLossNumber. StateProvider {0} key {1} with error {2}",
            keyStateProvider,
            key,
            error);

        return error;
    }

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    NTSTATUS status = txnReplicator->Get(stateProviderName, stateProvider);

    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        error = ErrorCode::FromHResult(Data::Utilities::StatusConverter::ToHResult(status));
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Delete: Delete failed to Get StateProvider {0} key {1} with error {2}",
            keyStateProvider,
            key,
            error);

        return error;
    }

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        TestSession::WriteNoise(
            TraceSource,
            GetPartitionId(),
            "Delete: Get for StateProvider {0} failed as the state provider does not exist",
            keyStateProvider);

        return ErrorCode(ErrorCodeValue::FabricTestKeyDoesNotExist);
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        KFinally([&] {transaction->Dispose(); });
        error = InvokeKtlApiNoExcept([&]
        {
            return txnReplicator->CreateTransaction(transaction);
        });
        TestSession::FailTestIf(!error.IsSuccess(), "Failed to create transaction");

        Data::TStore::IStore<__int64, KString::SPtr>* storeStateProvider = dynamic_cast<Data::TStore::IStore<__int64, KString::SPtr>*>(stateProvider.RawPtr());
        Data::TStore::IStoreTransaction<__int64, KString::SPtr>::SPtr storeTransaction = nullptr;
        storeStateProvider->CreateOrFindTransaction(*transaction, storeTransaction);

        bool removed = false;
        error = InvokeKtlApi([&]
        {
            removed = SyncAwait(storeStateProvider->ConditionalRemoveAsync(*storeTransaction, key, FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout, CancellationToken::None));
            NTSTATUS status = SyncAwait(transaction->CommitAsync(FabricTestSessionConfig::GetConfig().TXRServiceOperationTimeout, ktl::CancellationToken::None));
            THROW_ON_FAILURE(status);
        });

        if (!error.IsSuccess())
        {
            TestSession::WriteNoise(
                TraceSource,
                GetPartitionId(),
                "Get failed to ConditionalRemoveAsync StateProvider {0} key {1} with error {2}",
                keyStateProvider,
                key,
                error);

            return error;
        }

        TestSession::FailTestIf(!removed, "Failed to remove value");
    }

    return error;
}
