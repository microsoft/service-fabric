// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace std;
using namespace Common;
using namespace ServiceModel;

HRESULT ComTestHostServiceFactory::CreateInstance( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_INSTANCE_ID instanceId,
    /* [retval][out] */ IFabricStatelessServiceInstance **service)
{
    UNREFERENCED_PARAMETER(instanceId);

    // Deserialize initData to wstring
    wstring initDataStr = GetInitString(initializationData, initializationDataLength);
    
    wstring serviceTypeString(serviceType);

    ServiceTypeDescription serviceTypeDescription;
    TestSession::FailTestIfNot(TryGetServiceTypeDescription(serviceTypeString, serviceTypeDescription), "serviceType {0} not supported", serviceTypeString);
    TestSession::FailTestIf(serviceTypeDescription.IsStateful, "CreateStatefulService for invalid service type {0}", serviceTypeString);

    CalculatorServiceSPtr testGenericStatelessServiceSPtr = factory_.CreateStatelessService(partitionId, serviceName, serviceTypeString, initDataStr);
    ComPointer<ComCalculatorService> comPointer = make_com<ComCalculatorService>(*testGenericStatelessServiceSPtr);
    return comPointer->QueryInterface(IID_IFabricStatelessServiceInstance, reinterpret_cast<void**>(service));
}

HRESULT ComTestHostServiceFactory::CreateReplica(
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [retval][out] */ IFabricStatefulServiceReplica **service)
{
    // Deserialize initData to wstring
    wstring initDataStr = GetInitString(initializationData, initializationDataLength);

    wstring serviceTypeString(serviceType);

    ServiceTypeDescription serviceTypeDescription;
    TestSession::FailTestIfNot(TryGetServiceTypeDescription(serviceTypeString, serviceTypeDescription), "serviceType {0} not supported", serviceTypeString);
    TestSession::FailTestIfNot(serviceTypeDescription.IsStateful, "CreateStatefulService for invalid service type {0}", serviceTypeString);

    if(serviceTypeDescription.HasPersistedState)
    {
        TestPersistedStoreServiceSPtr testPersistedStoreServiceSPtr = factory_.CreatePersistedService(
            partitionId,
            replicaId,
            serviceName,
            serviceTypeString,
            initDataStr);
        ComPointer<ComTestPersistedStoreService> comPointer = make_com<ComTestPersistedStoreService>(*testPersistedStoreServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
    }
    else
    {
        TestStoreServiceSPtr testStoreServiceSPtr = factory_.CreateStatefulService(partitionId, serviceName, serviceTypeString, initDataStr);
        ComPointer<ComTestStoreService> comPointer = make_com<ComTestStoreService>(*testStoreServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
    }
}

bool ComTestHostServiceFactory::TryGetServiceTypeDescription(std::wstring const& serviceType, ServiceTypeDescription & serviceTypeDescription)
{
    for (auto iter = supportedServiceTypes_.begin(); iter != supportedServiceTypes_.end(); ++iter)
    {
        if(iter->ServiceTypeName == serviceType)
        {
            serviceTypeDescription = *iter;
            return true;
        }
    }

    return false;
}

wstring ComTestHostServiceFactory::GetInitString(const byte *initData, ULONG nBytes)
{
    StringBody strBody(L"");
    if(nBytes > 0 && initData != NULL)
    {
        vector<byte> initDataV(initData, initData + nBytes);
        FabricSerializer::Deserialize(strBody, initDataV);
    }
    return strBody.String;
}
