// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;

KeyValueStoreEnumeratorBase::KeyValueStoreEnumeratorBase(
    std::wstring const & keyPrefix,
    bool strictPrefix,
    IStoreBase::EnumerationSPtr const & replicatedStoreEnumeration)
    : ReplicatedStoreEnumeratorBase()
    , keyPrefix_(keyPrefix)
    , strictPrefix_(strictPrefix)
    , replicatedStoreEnumeration_(replicatedStoreEnumeration)
{
}

KeyValueStoreEnumeratorBase::~KeyValueStoreEnumeratorBase()
{
}

ErrorCode KeyValueStoreEnumeratorBase::MoveNext()
{
    return this->OnMoveNextBase();
}

ErrorCode KeyValueStoreEnumeratorBase::OnMoveNextBase()
{
    auto error = replicatedStoreEnumeration_->MoveNext();
    if (!error.IsSuccess())
    {
        ResetCurrent();
        return error;
    }

    wstring currentKey;
    error = replicatedStoreEnumeration_->CurrentKey(currentKey);
    if (!error.IsSuccess()) { return error; }

    if (strictPrefix_ && (!keyPrefix_.empty()) && (!StringUtility::StartsWith(currentKey, keyPrefix_)))
    {
        return ErrorCode(ErrorCodeValue::EnumerationCompleted);
    }

    FILETIME currentLastModifiedUtc;
    error = replicatedStoreEnumeration_->CurrentLastModifiedFILETIME(currentLastModifiedUtc);
    if (!error.IsSuccess()) { return error; }

    FILETIME currentLastModifiedOnPrimaryUtc;
    error = replicatedStoreEnumeration_->CurrentLastModifiedOnPrimaryFILETIME(currentLastModifiedOnPrimaryUtc);
    if (!error.IsSuccess()) { return error; }

    FABRIC_SEQUENCE_NUMBER currentSequenceNumber;
    error = replicatedStoreEnumeration_->CurrentOperationLSN(currentSequenceNumber);
    if (!error.IsSuccess()) { return error; }

    error = InitializeCurrent(replicatedStoreEnumeration_, currentKey, currentSequenceNumber, currentLastModifiedUtc, currentLastModifiedOnPrimaryUtc);
    if (!error.IsSuccess())
    {
        ResetCurrent();
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode KeyValueStoreEnumeratorBase::TryMoveNext(bool & success)
{
    return this->TryMoveNextBase(success);
}
