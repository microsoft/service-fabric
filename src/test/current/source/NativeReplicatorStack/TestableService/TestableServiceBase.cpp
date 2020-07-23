// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TestableService;
using namespace Data;

static const StringLiteral TraceComponent("TestableService");

TestableServiceBase::TestableServiceBase(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : StatefulServiceBase(partitionId, replicaId, root)
    , workStatus_(WorkStatusEnum::NotStarted)
    , statusCode_(STATUS_UNSUCCESSFUL)
{
}

TestableServiceBase::~TestableServiceBase()
{
}

Common::ErrorCode TestableServiceBase::OnHttpPostRequest(
    __in Common::ByteBufferUPtr && body,
    __in Common::ByteBufferUPtr & responseBody)
{
    Trace.WriteInfo(TraceComponent, "Handle HTTP Post request");

    ClientRequest request;
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat; // This will have the enum in string format
    ErrorCode error = JsonHelper::Deserialize<ClientRequest>(request, body, flags);

    if (!error.IsSuccess()) {
        Trace.WriteError(TraceComponent, "Failed to deserialize client request body.");
        return ErrorCodeValue::StaleRequest;
    }

    Trace.WriteInfo(
        TraceComponent,
        "Deserialized http POST request with object {0} with Error Code {1}",
        request,
        error);

    switch (request.Operation)
    {
        case ClientOperationEnum::DoWork:
        {
            DoWorkAsyncWrapper();
            error = ErrorCodeValue::Success;
            break;
        }
        case ClientOperationEnum::CheckWorkStatus:
        {
            CheckWorkStatusReponse response;
            response.WorkStatus = workStatus_;
            response.StatusCode = statusCode_;
            error = JsonHelper::Serialize<CheckWorkStatusReponse>(
                response,
                responseBody,
                flags);
            break;
        }
        case ClientOperationEnum::GetProgress:
        {
            TestableServiceProgress progress = SyncAwait(GetProgressAsync());
            error = JsonHelper::Serialize<TestableServiceProgress>(
                progress,
                responseBody,
                flags);
            break;
        }
        default:
        {
            Trace.WriteError(TraceComponent, "Invalid Operation from client.");
            error = ErrorCodeValue::InvalidOperation;
        }
    }

    return error;
}

ktl::Task TestableServiceBase::DoWorkAsyncWrapper()
{
    WorkStatusEnum::Enum prevStatus = workStatus_.exchange(WorkStatusEnum::InProgress);
    if (prevStatus == WorkStatusEnum::InProgress)
    {
        // If the work is still InProgress, we don't need to set StatusCode. It's still STATUS_UNSUCCESSFUL.
        Trace.WriteWarning(TraceComponent, "Previous work is still in progress. Ignore new work request.");
        co_return;
    }

    try
    {
        co_await DoWorkAsync();
    }
    catch (ktl::Exception & exception)
    {
        // If the expection throw, set the WorkStatus to faulted and StatusCode to the exception status.
        Trace.WriteError(TraceComponent, "Exception happened when executing DoWorkAsync(). Exception status: {0}", exception.GetStatus());
        workStatus_ = WorkStatusEnum::Faulted;
        statusCode_ = exception.GetStatus();
        co_return;
    }

    // If the work completed sucessfully, set both WorkStatus and StatusCode.
    workStatus_ = WorkStatusEnum::Done;
    statusCode_ = STATUS_SUCCESS;
}

