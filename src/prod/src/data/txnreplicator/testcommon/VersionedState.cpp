// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator::TestCommon;
using namespace ktl;

using namespace Data::Utilities;

VersionedState::CSPtr VersionedState::Create(
    __in FABRIC_SEQUENCE_NUMBER version,
    __in KString const & value,
    __in KAllocator & allocator) noexcept
{
    CSPtr pointer = _new(KTL_TAG_TEST, allocator) VersionedState(version, value);
    ASSERT_IF(pointer == nullptr, "OOM while allocating VersionedState");
    ASSERT_IFNOT(NT_SUCCESS(pointer->Status()), "Unsuccessful initialization while allocating VersionedState {0}", pointer->Status());

    return pointer;
}

FABRIC_SEQUENCE_NUMBER VersionedState::get_Version() const
{
    return version_;
}

KString::CSPtr VersionedState::get_Value() const
{
    return value_;
}

VersionedState::VersionedState(
    __in FABRIC_SEQUENCE_NUMBER version,
    __in KString const & value)
    : KObject()
    , KShared()
    , version_(version)
    , value_(&value)
{
}

VersionedState::~VersionedState()
{
}
