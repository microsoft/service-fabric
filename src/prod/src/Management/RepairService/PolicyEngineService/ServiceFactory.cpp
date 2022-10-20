// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

ServiceFactory::ServiceFactory()
{
    Trace.WriteInfo(ComponentName, "ServiceFactory::ServiceFactory()");
}

ServiceFactory::~ServiceFactory()
{
    Trace.WriteInfo(ComponentName, "ServiceFactory::~ServiceFactory()");
}

void ServiceFactory::Initialize(IFabricCodePackageActivationContext* activationContext, Common::ManualResetEvent* exitServiceHostEventPtr)
{
    Trace.WriteNoise(ComponentName, "ServiceFactory::Initialize()");
    activationContextCPtr_.SetAndAddRef(activationContext);
    exitServiceHostEventPtr_ = exitServiceHostEventPtr;
}

STDMETHODIMP ServiceFactory::CreateReplica( 
    __in LPCWSTR serviceType,
    __in FABRIC_URI serviceName,
    __in ULONG initializationDataLength,
    __in const byte *initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __out IFabricStatefulServiceReplica **serviceReplica)
{
    Trace.WriteInfo(ComponentName, "ServiceFactory::CreateReplica()");

    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);
    UNREFERENCED_PARAMETER(replicaId);

    if (serviceType == nullptr || serviceName == nullptr)
    {
        Trace.WriteError(ComponentName, "ServiceFactory::CreateReplica() failed. Argument serviceType or serviceName is null.");
        return E_POINTER;
    }

    ComPointer<RepairPolicyEngineServiceReplica> RepairPolicyEngineServiceReplicaCPtr = make_com<RepairPolicyEngineServiceReplica>();
    HRESULT resultReplicaInitialize = RepairPolicyEngineServiceReplicaCPtr->Initialize(activationContextCPtr_.GetRawPointer(), partitionId, replicaId, exitServiceHostEventPtr_);

    if (FAILED(resultReplicaInitialize))
    {
        Trace.WriteError(ComponentName, "ServiceFactory::CreateReplica() failed because RepairPolicyEngineServiceReplica.Initialize() failed with error:{0}", resultReplicaInitialize);
        return resultReplicaInitialize;
    }
    
    ComPointer<IFabricStatefulServiceReplica> statefulServiceReplicaCPtr(RepairPolicyEngineServiceReplicaCPtr, IID_IFabricStatefulServiceReplica);

    *serviceReplica = statefulServiceReplicaCPtr.DetachNoRelease();

    return S_OK;
}
