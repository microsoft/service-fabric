// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TStoreNightWatchTXRService;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

StringLiteral const TraceComponent("TStoreNightWatchTXRService");

TStoreTestRunner::SPtr TStoreTestRunner::Create(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    TStoreTestRunner * pointer = _new('xxxx', allocator) TStoreTestRunner(
        testParametersJson,
        txnReplicator);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return TStoreTestRunner::SPtr(pointer);
}

TStoreTestRunner::TStoreTestRunner(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator)
    : NightWatchTXRService::TestRunner(testParametersJson, txnReplicator)
    , storeSPtr_(nullptr)
    , keyMap_()
    , testParameters_()
{
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat;
    JsonHelper::Deserialize<NightWatchTXRService::NightWatchTestParameters>(testParameters_, testParametersJson, flags);
}

TStoreTestRunner::~TStoreTestRunner()
{
}

Awaitable<LONG64> TStoreTestRunner::AddTx(__in std::wstring const & stateProviderName)
{
    TxnReplicator::IStateProvider2::SPtr stateProvider;
    KUriView nameUri(L"fabric:/nightwatchStore");
    KStringView typeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());

    keyMapLock_.AcquireExclusive();
    LONG64 key = InterlockedIncrement64(&index_);
    keyMap_[stateProviderName] = key;
    keyMapLock_.ReleaseExclusive();

    KBuffer::SPtr value = nullptr;
    KBuffer::Create(128, value, GetThisAllocator());

    {
        TxnReplicator::Transaction::SPtr transaction;
        THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));
        bool alreadyExist = false;
        NTSTATUS status = co_await txnReplicator_->GetOrAddAsync(
            *transaction,
            nameUri,
            typeName,
            stateProvider,
            alreadyExist);
        THROW_ON_FAILURE(status);
        ASSERT_IFNOT(stateProvider != nullptr, "State provider should not be null");
        THROW_ON_FAILURE(co_await transaction->CommitAsync());
        transaction->Dispose();
    }

    {
        TxnReplicator::Transaction::SPtr transaction;
        THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));

        storeSPtr_ = dynamic_cast<Data::TStore::IStore<LONG64, KBuffer::SPtr>*>(stateProvider.RawPtr());

        Data::TStore::IStoreTransaction<LONG64, KBuffer::SPtr>::SPtr storeTransaction = nullptr;
        storeSPtr_->CreateOrFindTransaction(*transaction, storeTransaction);
        co_await storeSPtr_->AddAsync(
            *storeTransaction,
            key,
            value,
            Common::TimeSpan::MaxValue,
            CancellationToken::None
        );

        THROW_ON_FAILURE(co_await transaction->CommitAsync());
        transaction->Dispose();
    }

    co_return 1;
}

Awaitable<LONG64> TStoreTestRunner::UpdateTx(__in std::wstring const & stateProviderName, __in LONG64 version)
{

    TxnReplicator::Transaction::SPtr transaction;
    THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));

    std::wstring valueString = testParameters_.UpdateOperationValue;

    keyMapLock_.AcquireShared();
    LONG64 key = keyMap_[stateProviderName];
    keyMapLock_.ReleaseShared();

    KBuffer::SPtr newValue = nullptr;
    KBuffer::Create(128, newValue, GetThisAllocator());

    ASSERT_IFNOT(storeSPtr_ != nullptr, "StateProvider does not exist");
    Data::TStore::IStoreTransaction<LONG64, KBuffer::SPtr>::SPtr storeTransaction = nullptr;
    storeSPtr_->CreateOrFindTransaction(*transaction, storeTransaction);
    bool success = co_await storeSPtr_->ConditionalUpdateAsync(
        *storeTransaction,
        key,
        newValue,
        Common::TimeSpan::MaxValue,
        CancellationToken::None
    );

    if (!success)
    {
        exceptionThrown_.store(true);
        exceptionSptr_ = Data::Utilities::SharedException::Create(ktl::Exception(SF_STATUS_TRANSACTION_ABORTED), GetThisAllocator());

        transaction->Dispose();
        co_return -1;
    }

    THROW_ON_FAILURE(co_await transaction->CommitAsync());
    transaction->Dispose();
    co_return transaction->CommitSequenceNumber;
}
