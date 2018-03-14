// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TestCommon;
using namespace FabricTest;

SGServiceFactory::SGServiceFactory(
    __in Federation::NodeId nodeId)
    : nodeId_(nodeId)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGServiceFactory::SGServiceFactory - ctor",
        nodeId_,
        this);
}

SGServiceFactory::~SGServiceFactory()
{
    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGServiceFactory::~SGServiceFactory - dtor",
        nodeId_,
        this);
}

SGStatefulServiceSPtr SGServiceFactory::CreateStatefulService(
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId)
{
    UNREFERENCED_PARAMETER(partitionId);

    std::wstring serviceTypeString(serviceType);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGServiceFactory::CreateStatefulService - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType,
        serviceName);

    SGStatefulServiceSPtr serviceSPtr = std::make_shared<SGStatefulService>(nodeId_, serviceType, serviceName, initializationDataLength, initializationData, replicaId);

    wstring init;

    if (initializationDataLength > 0)
    {
        init = wstring(reinterpret_cast<const wchar_t*>(initializationData));
    }

    if (SGStatefulService::StatefulServiceType == serviceTypeString)
    {
                                //  ECC    ECS    NCC    NCS   //
        serviceSPtr->FinalConstruct(false, false, false, false, init);
    }
    else if (SGStatefulService::StatefulServiceECCType == serviceTypeString)
    {
        serviceSPtr->FinalConstruct(true, false, false, false, init);
    }
    else if (SGStatefulService::StatefulServiceECSType == serviceTypeString)
    {
        serviceSPtr->FinalConstruct(false, true, false, false, init);
    }
    else if (SGStatefulService::StatefulServiceNCCType == serviceTypeString)
    {
        serviceSPtr->FinalConstruct(false, false, true, false, init);
    }
    else if (SGStatefulService::StatefulServiceNCSType == serviceTypeString)
    {
        serviceSPtr->FinalConstruct(false, false, false, true, init);
    }
    else
    {
        TestSession::FailTest("Unknown serviceType ({0})", serviceTypeString);
    }

    return serviceSPtr;
}

SGStatelessServiceSPtr SGServiceFactory::CreateStatelessService(
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_INSTANCE_ID instanceId)
{
    UNREFERENCED_PARAMETER(partitionId);

    std::wstring serviceTypeString(serviceType);

    TestSession::WriteNoise(
        TraceSource, 
        "Node({0}) - ({1}) - SGServiceFactory::CreateStatelessService - serviceType({2}) serviceName({3})",
        nodeId_,
        this,
        serviceType,
        serviceName);

    if (SGStatelessService::StatelessServiceType != serviceTypeString)
    {
        TestSession::FailTest("Unknown serviceType ({0})", serviceTypeString);
    }

    return std::make_shared<SGStatelessService>(nodeId_, serviceType, serviceName, initializationDataLength, initializationData, instanceId);
}


wstring const SGServiceFactory::SGStatefulServiceGroupType = L"SGStatefulServiceFactory";
wstring const SGServiceFactory::SGStatelessServiceGroupType = L"SGStatelessServiceFactory";
StringLiteral const SGServiceFactory::TraceSource("FabricTest.ServiceGroup.SGServiceFactory");
