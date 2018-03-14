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

#define TESTOPERATION_TAG 'rpOT'

TestOperation::TestOperation(__in Data::Utilities::OperationData const & operationData)
    : operationData_(&operationData)
{
}

TestOperation::~TestOperation()
{
}

TestOperation::SPtr TestOperation::Create(
    __in OperationData const & operationData,
    __in KAllocator & allocator)
{
    TestOperation * pointer = _new(TESTOPERATION_TAG, allocator) TestOperation(operationData);
    CODING_ERROR_ASSERT(pointer != nullptr);

    return TestOperation::SPtr(pointer);
}

void TestOperation::Acknowledge()
{
}

FABRIC_OPERATION_TYPE TestOperation::get_OperationType() const
{
    return FABRIC_OPERATION_TYPE_NORMAL;
}

LONG64 TestOperation::get_SequenceNumber() const
{
    return 0;
}

LONG64 TestOperation::get_AtomicGroupId() const
{
    return 0;
}

OperationData::CSPtr TestOperation::get_Data() const
{
    return operationData_;
}
