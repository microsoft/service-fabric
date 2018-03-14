// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace TxnReplicator;

RuntimeFolders::RuntimeFolders(__in LPCWSTR workDirectory)
    : KObject()
    , KShared()
    , workDirectory_()
{
    NTSTATUS status = KString::Create(
        workDirectory_,
        GetThisAllocator(),
        workDirectory);

    THROW_ON_FAILURE(status);
}

RuntimeFolders::~RuntimeFolders()
{
}

IRuntimeFolders::SPtr RuntimeFolders::Create(
    __in IFabricCodePackageActivationContext & codePackage,
    __in KAllocator& allocator)
{
    RuntimeFolders * pointer = _new(RUNTIMEFOLDERS_TAG, allocator)RuntimeFolders(codePackage.get_WorkDirectory());
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return IRuntimeFolders::SPtr(pointer);
}

IRuntimeFolders::SPtr RuntimeFolders::Create(
    __in IFabricTransactionalReplicatorRuntimeConfigurations & runtimeConfigurations,
    __in KAllocator& allocator)
{
    RuntimeFolders * pointer = _new(RUNTIMEFOLDERS_TAG, allocator)RuntimeFolders(runtimeConfigurations.get_WorkDirectory());
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return IRuntimeFolders::SPtr(pointer);
}

LPCWSTR RuntimeFolders::get_WorkDirectory() const
{
    return *workDirectory_;
}
