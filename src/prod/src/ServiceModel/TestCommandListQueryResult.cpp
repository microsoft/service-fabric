// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

TestCommandListQueryResult::TestCommandListQueryResult()
    : operationId_()
    , testCommandState_()
    , testCommandType_()
{
}

TestCommandListQueryResult::TestCommandListQueryResult(
    Common::Guid const & operationId,     
    FABRIC_TEST_COMMAND_PROGRESS_STATE state,
    FABRIC_TEST_COMMAND_TYPE type)
    : operationId_(operationId)
    , testCommandState_(state)
    , testCommandType_(type)
{
}

TestCommandListQueryResult::TestCommandListQueryResult(TestCommandListQueryResult &&other)
    : operationId_(move(other.operationId_))
    , testCommandState_(move(other.testCommandState_))
    , testCommandType_(move(other.testCommandType_))
{
}

TestCommandListQueryResult & TestCommandListQueryResult::operator = (TestCommandListQueryResult && other)
{
    if (this != &other)
    {
        operationId_ = move(other.operationId_);
        testCommandState_ = move(other.testCommandState_);
        testCommandType_ = move(other.testCommandType_);
    }

    return *this;
}

void TestCommandListQueryResult::ToPublicApi(
    __in Common::ScopedHeap &, 
    __out TEST_COMMAND_QUERY_RESULT_ITEM & publicResult) const 
{
    publicResult.OperationId = operationId_.AsGUID();
    publicResult.TestCommandState = testCommandState_;
    publicResult.TestCommandType = testCommandType_;
}

void TestCommandListQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring TestCommandListQueryResult::ToString() const
{   
    return wformatString("OperationId = '{0}', State = '{1}', ActionType = '{2}'",
        operationId_,
        (DWORD)testCommandState_,
        (DWORD)testCommandType_);
}


