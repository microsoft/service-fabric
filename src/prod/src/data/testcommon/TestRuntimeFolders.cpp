// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TestCommon;

#define TESTRUNTIMEFOLDER 'FtRT'

TestRuntimeFolders::TestRuntimeFolders(__in LPCWSTR workingDirectory)
    : KObject()
    , KShared()
    , workDirectory_()
{
    NTSTATUS status = KString::Create(workDirectory_, GetThisAllocator(), workingDirectory);
    THROW_ON_FAILURE(status);
}

TestRuntimeFolders::~TestRuntimeFolders()
{
}

TxnReplicator::IRuntimeFolders::SPtr TestRuntimeFolders::Create(
    __in LPCWSTR workingDirectory,
    __in KAllocator & allocator)
{
    TestRuntimeFolders * pointer = _new(TESTRUNTIMEFOLDER, allocator) TestRuntimeFolders(workingDirectory);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return TxnReplicator::IRuntimeFolders::SPtr(pointer);
}

LPCWSTR TestRuntimeFolders::get_WorkDirectory() const
{
    return *workDirectory_;
}
