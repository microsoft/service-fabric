// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::Utilities;

#define TESTTRANSACTIONOPERATION_TAG 'rpTC'

TestTransactionChangeHandler::SPtr TestTransactionChangeHandler::Create(
    __in KAllocator& allocator)
{
    SPtr result = _new(TESTTRANSACTIONOPERATION_TAG, allocator) TestTransactionChangeHandler();
    CODING_ERROR_ASSERT(result != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(result->Status()));

    return result;
}

void TestTransactionChangeHandler::OnTransactionCommitted(
    __in ITransactionalReplicator& source, 
    __in ITransaction const & transaction)
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(transaction);
    InterlockedIncrement(&commitNotificationCount_);
}

TestTransactionChangeHandler::TestTransactionChangeHandler()
    : KObject()
    , KShared()
{
    SetConstructorStatus(S_OK);
}

TestTransactionChangeHandler::~TestTransactionChangeHandler()
{
}
