// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;
using namespace Data::LoggingReplicator;

NTSTATUS VersionManagerFactory::Create(
    __in PartitionedReplicaId const & traceId,
    __in TxnReplicator::IVersionProvider & versionProvider,
    __in KAllocator & allocator,
    __out TxnReplicator::IVersionManager::SPtr & versionManager)
{
    UNREFERENCED_PARAMETER(traceId);

    VersionManager::SPtr tmpVersionManager = VersionManager::Create(allocator);

    if (NT_SUCCESS(tmpVersionManager->Status()) == false)
    {
        return tmpVersionManager->Status();
    }

    tmpVersionManager->Initialize(versionProvider);
    versionManager = tmpVersionManager.RawPtr();

    return tmpVersionManager->Status();
}
