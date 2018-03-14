// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data;
using namespace ktl;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("TSEnumeration");

TSEnumeration::TSEnumeration(
    ::Store::PartitionedReplicaId const & partitionedReplicaId,
    wstring const & type, 
    wstring const & keyPrefix,
    IKvs::SPtr const & innerStore,
    IKvsTransaction::SPtr && storeTx,
    IKvsEnumerator::SPtr && innerEnum)
    : TSEnumerationBase(partitionedReplicaId, type, keyPrefix, false) // strictPrefix check performed in KeyValueStoreEnumeratorBase
    , innerStore_(innerStore)
    , storeTx_(move(storeTx))
    , innerEnum_(move(innerEnum)) 
    , traceId_()
{
    traceId_ = wformatString("[{0}+{1}]", 
        PartitionedReplicaTraceComponent::TraceId,
        TextTraceThis);

    WriteNoise(
        TraceComponent,
        "{0} ctor({1}, {2})",
        this->TraceId,
        this->GetTargetType(),
        this->GetTargetKeyPrefix());
}

shared_ptr<TSEnumeration> TSEnumeration::Create(
    ::Store::PartitionedReplicaId const & partitionedReplicaId,
    wstring const & type, 
    wstring const & keyPrefix,
    IKvs::SPtr const & innerStore,
    IKvsTransaction::SPtr && storeTx,
    IKvsEnumerator::SPtr && enumerator)
{
    return shared_ptr<TSEnumeration>(new TSEnumeration(partitionedReplicaId, type, keyPrefix, innerStore, move(storeTx), move(enumerator)));
}

TSEnumeration::~TSEnumeration()
{
    WriteNoise(
        TraceComponent,
        "{0} ~dtor",
        this->TraceId);
}

ErrorCode TSEnumeration::MoveNext()
{
    return this->OnMoveNextBase();
}

bool TSEnumeration::OnInnerMoveNext()
{
    return innerEnum_->MoveNext();
}

KString::SPtr TSEnumeration::OnGetCurrentKey()
{
    return innerEnum_->Current();
}

ErrorCode TSEnumeration::CurrentOperationLSN(__inout _int64 & lsn)
{
    IKvsGetEntry currentEntry;
    auto error = this->InnerGetCurrentEntry(currentEntry);
    if (!error.IsSuccess()) { return error; }

    lsn = currentEntry.Key;

    WriteNoise(
        TraceComponent, 
        "{0} CurrentLsn({1}, {2}) lsn={3}",
        this->TraceId,
        this->GetCurrentType(),
        this->GetCurrentKey(),
        lsn);

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::CurrentLastModifiedFILETIME(__inout FILETIME & filetime)
{
    // TODO: Used by Naming service
    //
    FILETIME result = {0};
    filetime = result;

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::CurrentLastModifiedOnPrimaryFILETIME(__inout FILETIME & filetime)
{
    // TODO: Used by managed KVS
    //
    FILETIME result = {0};
    filetime = result;

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::CurrentType(__inout std::wstring & type)
{
    type = this->GetCurrentType();

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::CurrentKey(__inout std::wstring & key)
{
    key = this->GetCurrentKey();

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::CurrentValue(__inout std::vector<byte> & value)
{
    value.clear();

    IKvsGetEntry currentEntry;
    auto error = this->InnerGetCurrentEntry(currentEntry);
    if (!error.IsSuccess()) { return error; }

    value = this->ToByteVector(currentEntry.Value);

    WriteNoise(
        TraceComponent,
        "{0} CurrentValue({1}, {2}) {3} bytes",
        this->TraceId,
        this->GetCurrentType(),
        this->GetCurrentKey(),
        value.size());

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::CurrentValueSize(__inout size_t & size)
{
    IKvsGetEntry currentEntry;
    auto error = this->InnerGetCurrentEntry(currentEntry);
    if (!error.IsSuccess()) { return error; }

    size = currentEntry.Value->QuerySize();

    return ErrorCodeValue::Success;
}

ErrorCode TSEnumeration::InnerGetCurrentEntry(__out IKvsGetEntry & currentEntry)
{
    KString::SPtr currentKey;
    TRY_CATCH_VOID( currentKey = innerEnum_->Current() );

    bool found = false;
    TRY_CATCH_VOID( found = SyncAwait(innerStore_->ConditionalGetAsync(
        *storeTx_,
        currentKey,
        TimeSpan::MaxValue,
        currentEntry,
        CancellationToken::None)) );

    return found ? ErrorCodeValue::Success : ErrorCodeValue::NotFound;
}

StringLiteral const & TSEnumeration::GetTraceComponent() const
{
    return TraceComponent;
}

wstring const & TSEnumeration::GetTraceId() const
{
    return this->TraceId;
}
