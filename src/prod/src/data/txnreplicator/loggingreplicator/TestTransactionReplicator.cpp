// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

#define TESTTRANSACTIONREPLICATOR_TAG 'TRT'

namespace LoggingReplicatorTests
{
    TestTransactionReplicator::SPtr TestTransactionReplicator::Create(
        __in KAllocator & allocator)
    {
        SPtr result = _new(TESTTRANSACTIONREPLICATOR_TAG, allocator) TestTransactionReplicator();
        CODING_ERROR_ASSERT(result != nullptr);
        CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

        return result;
    }

    NTSTATUS TestTransactionReplicator::RegisterTransactionChangeHandler(
        __in ITransactionChangeHandler & transactionChangeHandler) noexcept
    {
        UNREFERENCED_PARAMETER(transactionChangeHandler);

        CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
    }

    NTSTATUS TestTransactionReplicator::UnRegisterTransactionChangeHandler() noexcept
    {
        CODING_ASSERT("STATUS_NOT_IMPLEMENTED");
    }


    TestTransactionReplicator::TestTransactionReplicator()
        : KObject()
        , KShared()
    {
    }

    TestTransactionReplicator::~TestTransactionReplicator()
    {
    }
}
