// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("NodeTransitionResult");

NodeTransitionResult::NodeTransitionResult()
    : errorCode_(0)
    , nodeResult_()
{
}

NodeTransitionResult::NodeTransitionResult(NodeTransitionResult && other)
    : errorCode_(move(other.errorCode_))
    , nodeResult_(move(other.nodeResult_))
{
}

ErrorCode NodeTransitionResult::FromPublicApi(FABRIC_NODE_TRANSITION_RESULT const & publicResult)
{
    Trace.WriteInfo(TraceComponent, "Inside NodeTransitionResult::FromPublicApi");

    errorCode_ = publicResult.ErrorCode;

    Management::FaultAnalysisService::NodeResult nodeResult;

    FABRIC_NODE_RESULT * publicNodeResult = publicResult.NodeResult;
    auto error = nodeResult.FromPublicApi(*publicNodeResult);
    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "NodeTransitionResult::FromPublicApi/Failed at selectedPartition.FromPublicApi");
        return error;
    }

    Trace.WriteWarning(TraceComponent, "nodeResult.ServiceName: {0}", nodeResult.NodeName);
    Trace.WriteWarning(TraceComponent, "selectedPartition.PartitionId: {0}", nodeResult.NodeInstanceId);

    nodeResult_ = make_shared<Management::FaultAnalysisService::NodeResult>(nodeResult);
    
    return ErrorCodeValue::Success;
}

ErrorCode NodeTransitionResult::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_NODE_TRANSITION_RESULT & publicResult)
{
    UNREFERENCED_PARAMETER(heap);

    Trace.WriteInfo(TraceComponent, "Inside NodeTransitionResult::ToPublicApi");

    
    auto publicNodeResultPtr = heap.AddItem<FABRIC_NODE_RESULT>();
    nodeResult_->ToPublicApi(heap, *publicNodeResultPtr);
    publicResult.NodeResult = publicNodeResultPtr.GetRawPointer();

    publicResult.ErrorCode = errorCode_;


    Trace.WriteInfo(TraceComponent, "Exit NodeTransitionResult::ToPublicApi");
    

    return ErrorCodeValue::Success;
}
 
