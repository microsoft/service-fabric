// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace Transport;
using namespace TestCommon;

StringLiteral const TraceSource = "StatelessService";
std::wstring const CalculatorService::DefaultServiceType = L"CalculatorServiceType";

CalculatorService::CalculatorService(Federation::NodeId nodeId, FABRIC_PARTITION_ID partitionId, wstring const& serviceName, wstring const& initDataStr, wstring const& serviceType, wstring const& appId)
    :serviceName_(serviceName),
    partitionId_(Common::Guid(partitionId).ToString()),
    serviceLocation_(),
    isOpen_(false),
    partition_(),
    serviceType_(serviceType),
    appId_(appId),
    nodeId_(nodeId),
    initDataStr_(initDataStr)
{
    TestSession::WriteNoise(TraceSource, partitionId_, "CalculatorService created with name {0} and initialization-data:'{1}'", serviceName, initDataStr_);
}

CalculatorService::~CalculatorService()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "Dtor called for Calculator Service");
}

ErrorCode CalculatorService::OnOpen(Common::ComPointer<IFabricStatelessServicePartition3> const& partition)
{
    TestSession::FailTestIf(isOpen_, "Service open called twice");

    FABRIC_SERVICE_PARTITION_INFORMATION const *partitionInfo;
    HRESULT result = partition->GetPartitionInfo(&partitionInfo);

    if (!SUCCEEDED(result))
    {
        return ErrorCode::FromHResult(result);
    }

    TestSession::FailTestIfNot(result == S_OK, "GetPartitionInfo did not return success");
    partition_ = partition;
    partitionWrapper_.SetPartition(partition);
    serviceLocation_ = Common::Guid::NewGuid().ToString();
    isOpen_ = true;

    TestSession::WriteNoise(TraceSource, partitionId_, "Open called for CalculatorService");

    auto root = shared_from_this();
    TestServiceInfo testServiceInfo(
        appId_,
        serviceType_, 
        serviceName_, 
        partitionId_, 
        serviceLocation_, 
        false, 
        FABRIC_REPLICA_ROLE_NONE);

    OnOpenCallback(testServiceInfo, root, false);

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode CalculatorService::OnClose()
{
    TestSession::WriteNoise(TraceSource, partitionId_, "Close called for CalculatorService");
    TestSession::FailTestIfNot(isOpen_, "Close called on unopened calculator service");
    isOpen_ = false;
    OnCloseCallback(serviceLocation_);
    OnCloseCallback = nullptr;
    OnOpenCallback = nullptr;
    partition_.Release();
    return ErrorCode(ErrorCodeValue::Success);
}

void CalculatorService::OnAbort()
{
    if (isOpen_)
    {
        TestSession::WriteNoise(TraceSource, partitionId_, "Abort called for CalculatorService");
        isOpen_ = false;

        if (OnCloseCallback)
        {
            OnCloseCallback(serviceLocation_);
        }

        OnCloseCallback = nullptr;
        OnOpenCallback = nullptr;
        partition_.Release();
    }
}
