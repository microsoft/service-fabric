// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace StateManagerTests;

TestTransaction::SPtr TestTransaction::Create(LONG64 transactionId, KAllocator& allocator)
{
    TestTransaction::SPtr result = _new(TEST_TAG, allocator) TestTransaction(transactionId);
    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(result->Status() == STATUS_SUCCESS);

    return result;
}

TestTransaction::TestTransaction(LONG64 transactionId) 
    : TransactionBase(transactionId, false)
{
}

TestTransaction::~TestTransaction()
{
}
