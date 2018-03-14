// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

TestVersionProvider::SPtr TestVersionProvider::Create(
    __in std::vector<LONG64>& resultList,
    __in KAllocator& allocator)
{
    TestVersionProvider * pointer = _new(KTL_TAG_TEST, allocator) TestVersionProvider(resultList);
    THROW_ON_ALLOCATION_FAILURE(pointer)
    THROW_ON_FAILURE(pointer->Status())

    return SPtr(pointer);
}

NTSTATUS TestVersionProvider::GetVersion(__out LONG64 & version) const noexcept
{
    ASSERT_IFNOT(barrierIterator_ != barrierIteratorEnd_, "Barrier Response List cannot be empty")
    version = *(barrierIterator_++);
    return STATUS_SUCCESS;
}

TestVersionProvider::TestVersionProvider(std::vector<LONG64>& resultList) noexcept
    : barrierIterator_(resultList.begin())
    , barrierIteratorEnd_(resultList.end())
{
}

TestVersionProvider::~TestVersionProvider() 
{
}
