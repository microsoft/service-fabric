// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Common;
using namespace std;
using namespace Store;

//
// IStatefulServiceReplica
//
AsyncOperationSPtr FauxLocalStore::BeginOpen(
    ::FABRIC_REPLICA_OPEN_MODE,
    ComPointer<::IFabricStatefulServicePartition> const &,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode FauxLocalStore::EndOpen(
    AsyncOperationSPtr const & asyncOperation, 
    __out ComPointer<::IFabricReplicator> &)
{
    return CompletedAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FauxLocalStore::BeginChangeRole(
    ::FABRIC_REPLICA_ROLE,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode FauxLocalStore::EndChangeRole(
    AsyncOperationSPtr const & asyncOperation, 
    __out std::wstring &)
{
    return CompletedAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr FauxLocalStore::BeginClose(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode FauxLocalStore::EndClose(
    AsyncOperationSPtr const & asyncOperation)
{
    return CompletedAsyncOperation::End(asyncOperation);
}

void FauxLocalStore::Abort()
{
}

//
// IReplicatedStore
//
ErrorCode FauxLocalStore::CreateTransaction(TransactionSPtr & transactionSPtr)
{
    transactionSPtr = make_shared<Transaction>();

    return ErrorCode::Success();
}

ErrorCode FauxLocalStore::CreateSimpleTransaction(TransactionSPtr & transactionSPtr)
{
    return CreateTransaction(transactionSPtr);
}

ErrorCode FauxLocalStore::CreateEnumerationByTypeAndKey(
    TransactionSPtr const &,
    std::wstring const &,
    std::wstring const &,
    EnumerationSPtr & enumerationSPtr)
{
    enumerationSPtr = make_shared<Enumeration>();

    return ErrorCode::Success();
}

ErrorCode FauxLocalStore::Insert(
    TransactionSPtr const &,
    std::wstring const &,
    std::wstring const &,
    __in void const *,
    size_t)
{
    return ErrorCode::Success();
}

ErrorCode FauxLocalStore::Update(
    TransactionSPtr const &,
    std::wstring const &,
    std::wstring const &,
    _int64,
    std::wstring const &,
    __in_opt void const *,
    size_t)
{
    return ErrorCode::Success();
}

ErrorCode FauxLocalStore::Delete(
    TransactionSPtr const &,
    std::wstring const &,
    std::wstring const &,
    _int64)
{
    return ErrorCode::Success();
}

FILETIME FauxLocalStore::GetStoreUtcFILETIME()
{
    return DateTime::Now().AsFileTime;
}

// ------------------------------------------------------------------------
// FauxLocalStore::Transaction
//

FauxLocalStore::Transaction::Transaction()
{
}

FauxLocalStore::Transaction::~Transaction()
{
}

ErrorCode FauxLocalStore::Transaction::Commit(
    ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
    TimeSpan const timeout)
{
    UNREFERENCED_PARAMETER(timeout);

    // No support for commit sequence number, hence set to default value of 0
    commitSequenceNumber = Store::ILocalStore::SequenceNumberIgnore;
    return ErrorCode::Success();
}

AsyncOperationSPtr FauxLocalStore::Transaction::BeginCommit(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    ::FABRIC_SEQUENCE_NUMBER dontCareSequenceNumber;
    auto error = Commit(dontCareSequenceNumber, timeout);
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, state);
}

ErrorCode FauxLocalStore::Transaction::EndCommit(
    AsyncOperationSPtr const & operation,
    __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
{
    // No support for commit sequence number, hence set to default value of 0
    commitSequenceNumber = Store::ILocalStore::SequenceNumberIgnore;
    return CompletedAsyncOperation::End(operation);
}

void FauxLocalStore::Transaction::Rollback()
{
}

// ------------------------------------------------------------------------
// EseLocalStore::Enumeration
//

FauxLocalStore::Enumeration::Enumeration()
{
}

FauxLocalStore::Enumeration::~Enumeration()
{
}

ErrorCode FauxLocalStore::Enumeration::MoveNext()
{
    return ErrorCode(ErrorCodeValue::EnumerationCompleted);
}

ErrorCode FauxLocalStore::Enumeration::CurrentOperationLSN(_int64 &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}

ErrorCode FauxLocalStore::Enumeration::CurrentLastModifiedFILETIME(FILETIME &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}

ErrorCode FauxLocalStore::Enumeration::CurrentLastModifiedOnPrimaryFILETIME(FILETIME &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}

ErrorCode FauxLocalStore::Enumeration::CurrentType(
    __inout std::wstring &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}

ErrorCode FauxLocalStore::Enumeration::CurrentKey(
    __inout std::wstring &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}

ErrorCode FauxLocalStore::Enumeration::CurrentValue(
    __inout std::vector<byte> &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}

ErrorCode FauxLocalStore::Enumeration::CurrentValueSize(
    __inout size_t &)
{
    Assert::CodingError("Not Supported since MoveNext always fails.");
}
