// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Store;

KeyValueStoreItemMetadataEnumerator::KeyValueStoreItemMetadataEnumerator(
    std::wstring const & keyPrefix,
    bool strictPrefix,
    IStoreBase::EnumerationSPtr const & replicatedStoreEnumeration)
    : IKeyValueStoreItemMetadataEnumerator(),
    KeyValueStoreEnumeratorBase(keyPrefix, strictPrefix, replicatedStoreEnumeration),
    current_(),
    lock_()
{
}

KeyValueStoreItemMetadataEnumerator::~KeyValueStoreItemMetadataEnumerator()
{
}

ErrorCode KeyValueStoreItemMetadataEnumerator::MoveNext()
{
    return KeyValueStoreEnumeratorBase::MoveNext();
}

ErrorCode KeyValueStoreItemMetadataEnumerator::TryMoveNext(bool & success)
{
    return KeyValueStoreEnumeratorBase::TryMoveNext(success);
}

IKeyValueStoreItemMetadataResultPtr KeyValueStoreItemMetadataEnumerator::get_Current()
{
    {
        AcquireReadLock lock(lock_);
        return current_;
    }
}

void KeyValueStoreItemMetadataEnumerator::ResetCurrent()
{
    {
        AcquireWriteLock lock(lock_);
        current_ = Api::IKeyValueStoreItemMetadataResultPtr();
    }
}

ErrorCode KeyValueStoreItemMetadataEnumerator::InitializeCurrent(
    IStoreBase::EnumerationSPtr const & enumeration,
    wstring const & key,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    FILETIME lastModifiedUtc,
    FILETIME lastModifiedOnPrimaryUtc)
{
    size_t valueSize;
    auto error = enumeration->CurrentValueSize(valueSize);
    if (!error.IsSuccess()) { return error; }

    auto itemImpl = make_shared<KeyValueStoreItemMetadataResult>(key, static_cast<LONG>(valueSize), sequenceNumber, lastModifiedUtc, lastModifiedOnPrimaryUtc);

    {
        AcquireWriteLock lock(lock_);
        current_ = IKeyValueStoreItemMetadataResultPtr((IKeyValueStoreItemMetadataResult*)itemImpl.get(), itemImpl->CreateComponentRoot());
    }

    return ErrorCode(ErrorCodeValue::Success);
}
