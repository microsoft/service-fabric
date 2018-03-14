// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;

NTSTATUS OpenParameters::Create(
    __in bool completeCheckpoint,
    __in bool cleanupRestore,
    __in KAllocator& allocator,
    __out CSPtr & result) noexcept
{
    result = _new(STATE_MANAGER_TAG, allocator) OpenParameters(completeCheckpoint, cleanupRestore);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(result->Status()) == false)
    {
        return (CSPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

OpenParameters::OpenParameters(
    __in bool completeCheckpoint,
    __in bool cleanupRestore) noexcept
    : completeCheckpoint_(completeCheckpoint)
    , cleanupRestore_(cleanupRestore)
{
    ASSERT_IFNOT(
        cleanupRestore == false || completeCheckpoint == false,
        "OpenParameters: CompleteCheckpoint: {0}, CleanupRestore: {1}",
        completeCheckpoint,
        cleanupRestore);
}

OpenParameters::~OpenParameters()
{
}
