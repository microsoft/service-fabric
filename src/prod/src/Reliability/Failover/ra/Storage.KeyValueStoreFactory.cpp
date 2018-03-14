// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Storage;

ErrorCode KeyValueStoreFactory::TryCreate(
    Parameters const & parameters,
    ReconfigurationAgent * ra,
    __out Storage::Api::IKeyValueStoreSPtr & kvs)
{
    Storage::Api::IKeyValueStoreSPtr inner;
    auto error = TryCreateInternal(parameters, ra, inner);
    if (!error.IsSuccess())
    {
        return error;
    }

    if (parameters.IsFaultInjectionEnabled)
    {
        kvs = make_shared<FaultInjectionAdapter>(inner);
    }
    else
    {
        kvs = inner;
    }

    return error;
}

ErrorCode KeyValueStoreFactory::TryCreateInternal(
    Parameters const & parameters,
    ReconfigurationAgent * ra,
    __out Storage::Api::IKeyValueStoreSPtr & kvs)
{
    if (parameters.IsInMemory)
    {
        ASSERT_IF(parameters.InMemoryKeyValueStoreState == nullptr, "in memory state cant be null");
        kvs = make_shared<InMemoryKeyValueStore>(parameters.InMemoryKeyValueStoreState);
        return ErrorCode::Success();
    }

    ASSERT_IF(ra == nullptr, "RA cannot be null for ese");
    ASSERT_IF(parameters.StoreFactory == nullptr, "StoreFactory cannot be null for ese");
    auto inner = make_shared<LocalStoreAdapter>(
        parameters.StoreFactory,
        *ra);

    auto error = inner->Open(
        parameters.InstanceSuffix, 
        parameters.WorkingDirectory,
        parameters.KtlLoggerInstance);

    if (!error.IsSuccess())
    {
        return error;
    }

    kvs = inner;
    return error;
}
