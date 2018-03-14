// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Store;

KeyValueStoreItemEnumerator::KeyValueStoreItemEnumerator(
    std::wstring const & keyPrefix,
    bool strictPrefix,
    IStoreBase::EnumerationSPtr const & replicatedStoreEnumeration)
    : IKeyValueStoreItemEnumerator(),
    KeyValueStoreEnumeratorBase(keyPrefix, strictPrefix, replicatedStoreEnumeration),
    current_(),
    lock_()
{
}

KeyValueStoreItemEnumerator::~KeyValueStoreItemEnumerator()
{
}

ErrorCode KeyValueStoreItemEnumerator::MoveNext()
{
    return KeyValueStoreEnumeratorBase::MoveNext();
}

ErrorCode KeyValueStoreItemEnumerator::TryMoveNext(bool & success)
{
    return KeyValueStoreEnumeratorBase::TryMoveNext(success);
}

IKeyValueStoreItemResultPtr KeyValueStoreItemEnumerator::get_Current()
{
    {
        AcquireReadLock lock(lock_);
        return current_;
    }
}

void KeyValueStoreItemEnumerator::ResetCurrent()
{
    {
        AcquireWriteLock lock(lock_);
        current_ = Api::IKeyValueStoreItemResultPtr();
    }
}

ErrorCode KeyValueStoreItemEnumerator::InitializeCurrent(
    IStoreBase::EnumerationSPtr const & enumeration,
    wstring const & key,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    FILETIME lastModifiedUtc,
    FILETIME lastModifiedOnPrimaryUtc)
{
    vector<BYTE> value;
    auto error = enumeration->CurrentValue(value);
    if (!error.IsSuccess()) { return error; }

    auto itemImpl = make_shared<KeyValueStoreItemResult>(key, move(value), sequenceNumber, lastModifiedUtc, lastModifiedOnPrimaryUtc);

    {
        AcquireWriteLock lock(lock_);
        current_ = IKeyValueStoreItemResultPtr((IKeyValueStoreItemResult*)itemImpl.get(), itemImpl->CreateComponentRoot());
    }
    
    return ErrorCode(ErrorCodeValue::Success);
}
