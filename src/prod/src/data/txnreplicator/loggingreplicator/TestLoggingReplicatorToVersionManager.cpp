// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTLOGGINGREPLICATORTOVERSIONMANAGER_TAG 'VtLT'

TestLoggingReplicatorToVersionManager::TestLoggingReplicatorToVersionManager()
    : ILoggingReplicatorToVersionManager()
    , KObject()
    , KShared()
{
}

TestLoggingReplicatorToVersionManager::~TestLoggingReplicatorToVersionManager()
{
}

TestLoggingReplicatorToVersionManager::SPtr TestLoggingReplicatorToVersionManager::Create(__in KAllocator & allocator)
{
    TestLoggingReplicatorToVersionManager * pointer = _new(TESTLOGGINGREPLICATORTOVERSIONMANAGER_TAG, allocator)TestLoggingReplicatorToVersionManager();
    CODING_ERROR_ASSERT(pointer != nullptr);

    return TestLoggingReplicatorToVersionManager::SPtr(pointer);
}

void TestLoggingReplicatorToVersionManager::UpdateDispatchingBarrierTask(__in CompletionTask & barrierTask)
{
    UNREFERENCED_PARAMETER(barrierTask);
}
