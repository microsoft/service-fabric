// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace StateManagerTests;

TestOperationContext::SPtr TestOperationContext::Create(__in LONG64 stateProviderId, __in KAllocator & allocator) noexcept
{
    SPtr result = _new(TEST_OPERATIONCONTEXT, allocator) TestOperationContext(stateProviderId);
    VERIFY_IS_NOT_NULL(result, L"Must not be nullptr");
    VERIFY_IS_TRUE(NT_SUCCESS(result->Status()));

    return result;
}

bool TestOperationContext::get_IsUnlocked() const
{
    return isUnlocked_;
}

void TestOperationContext::Unlock(LONG64 stateProviderId) noexcept
{
    VERIFY_ARE_EQUAL(stateProviderId, stateProviderId);
    VERIFY_IS_FALSE(isUnlocked_);
    isUnlocked_ = true;
}

TestOperationContext::TestOperationContext(__in LONG64 stateProviderId) noexcept
    : stateProviderId_(stateProviderId)
{
}

TestOperationContext::~TestOperationContext()
{
}


