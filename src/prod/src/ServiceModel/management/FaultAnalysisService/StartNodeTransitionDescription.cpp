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

StringLiteral const TraceComponent("StartNodeTransitionDescription");

StartNodeTransitionDescription::StartNodeTransitionDescription()
: nodeTransitionType_()
, operationId_() 
, nodeName_()
, instanceId_()
, stopDurationInSeconds_()
{
}

StartNodeTransitionDescription::StartNodeTransitionDescription(StartNodeTransitionDescription const & other)
: nodeTransitionType_(other.nodeTransitionType_)
, operationId_(other.operationId_) 
, nodeName_(other.nodeName_)
, instanceId_(other.instanceId_)
, stopDurationInSeconds_(other.stopDurationInSeconds_)
{
}

StartNodeTransitionDescription::StartNodeTransitionDescription(StartNodeTransitionDescription && other)
: nodeTransitionType_(other.nodeTransitionType_)
, operationId_(other.operationId_) 
, nodeName_(move(other.nodeName_))
, instanceId_(move(other.instanceId_))
, stopDurationInSeconds_(move(other.stopDurationInSeconds_))
{
}

StartNodeTransitionDescription & StartNodeTransitionDescription::operator = (StartNodeTransitionDescription && other)
{
    if (this != &other)
    {
        this->operationId_ = move(other.operationId_);
        this->nodeTransitionType_ = move(other.nodeTransitionType_);
        this->nodeName_ = move(other.nodeName_);
        this->instanceId_ = move(other.instanceId_);
        this->stopDurationInSeconds_ = move(other.stopDurationInSeconds_);
    }

    return *this;
}

StartNodeTransitionDescription::StartNodeTransitionDescription(
    FABRIC_NODE_TRANSITION_TYPE nodeTransitionType,
    Common::Guid const & operationId,
    std::wstring const & nodeName,
    uint64 nodeInstanceId,
    DWORD stopDurationInSeconds)
: nodeTransitionType_(nodeTransitionType)
, operationId_(operationId)
, nodeName_(nodeName)
, instanceId_(nodeInstanceId)
, stopDurationInSeconds_(stopDurationInSeconds)
{
}
            

Common::ErrorCode StartNodeTransitionDescription::FromPublicApi(
    FABRIC_NODE_TRANSITION_DESCRIPTION const & publicDescription)
{
    if (publicDescription.NodeTransitionType == FABRIC_NODE_TRANSITION_TYPE_START)
    {
        auto startDescription = reinterpret_cast<FABRIC_NODE_START_DESCRIPTION*>(publicDescription.Value);        
        if (startDescription == nullptr)
        {
            Trace.WriteError(TraceComponent, "startDescription is NULL");
            return ErrorCodeValue::InvalidArgument;
        }
        
        nodeTransitionType_ = FABRIC_NODE_TRANSITION_TYPE_START;
        operationId_ = Guid(startDescription->OperationId);
        HRESULT hr = StringUtility::LpcwstrToWstring(startDescription->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
            return ErrorCodeValue::InvalidNameUri;
        }

        if (startDescription->NodeInstanceId == 0)
        {
            return ErrorCodeValue::InvalidInstanceId;
        }
        
        instanceId_ = startDescription->NodeInstanceId;

        stopDurationInSeconds_ = 0;    
    }
    else if (publicDescription.NodeTransitionType == FABRIC_NODE_TRANSITION_TYPE_STOP)
    {
        auto stopDescription = reinterpret_cast<FABRIC_NODE_STOP_DESCRIPTION*>(publicDescription.Value);
        if (stopDescription == nullptr)
        {
            Trace.WriteError(TraceComponent, "stopDescription is NULL");
            return ErrorCodeValue::InvalidArgument;
        }
        
        nodeTransitionType_ = FABRIC_NODE_TRANSITION_TYPE_STOP;
        operationId_ = Guid(stopDescription->OperationId);     
        HRESULT hr = StringUtility::LpcwstrToWstring(stopDescription->NodeName, false, ParameterValidator::MinStringSize, ParameterValidator::MaxStringSize, nodeName_);
        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "Failed parsing nodename");
            return ErrorCodeValue::InvalidNameUri;
        }

        if (stopDescription->NodeInstanceId == 0)
        {
            return ErrorCodeValue::InvalidInstanceId;
        }
        
        instanceId_ = stopDescription->NodeInstanceId;
        
        stopDurationInSeconds_ = stopDescription->StopDurationInSeconds; 
    }
    else
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

void StartNodeTransitionDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_NODE_TRANSITION_DESCRIPTION & result) const
{
    if (nodeTransitionType_ == FABRIC_NODE_TRANSITION_TYPE_START)
    {
        auto startDescription = heap.AddItem<FABRIC_NODE_START_DESCRIPTION>();
        result.NodeTransitionType = FABRIC_NODE_TRANSITION_TYPE_START;
        result.Value = startDescription.GetRawPointer();

        startDescription->OperationId = operationId_.AsGUID();
        startDescription->NodeName = heap.AddString(nodeName_);
        startDescription->NodeInstanceId = instanceId_;
        startDescription->Reserved = NULL;
    }
    else if (nodeTransitionType_ == FABRIC_NODE_TRANSITION_TYPE_STOP)
    {
        auto stopDescription = heap.AddItem<FABRIC_NODE_STOP_DESCRIPTION>();
        result.NodeTransitionType = FABRIC_NODE_TRANSITION_TYPE_STOP;
        result.Value = stopDescription.GetRawPointer();

        stopDescription->OperationId = operationId_.AsGUID();
        stopDescription->NodeName = heap.AddString(nodeName_);
        stopDescription->NodeInstanceId = instanceId_;
        stopDescription->StopDurationInSeconds = stopDurationInSeconds_;
        stopDescription->Reserved = NULL;
    }
    else
    {
    }
}
