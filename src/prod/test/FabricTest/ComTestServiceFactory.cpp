// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace std;
using namespace Common;

HRESULT ComTestServiceFactory::CreateInstance( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_INSTANCE_ID instanceId,
    /* [retval][out] */ IFabricStatelessServiceInstance **service)
{
    UNREFERENCED_PARAMETER(instanceId);
    wstring serviceTypeString(serviceType);

    // Deserialize initData to wstring
    wstring initDataStr = GetInitString(initializationData, initializationDataLength);

    if(serviceTypeString == CalculatorService::DefaultServiceType)
    {
        CalculatorServiceSPtr calculatorServiceSPtr = factory_.CreateCalculatorService(partitionId, serviceName, initDataStr);
        ComPointer<ComCalculatorService> comPointer = make_com<ComCalculatorService>(*calculatorServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatelessServiceInstance, reinterpret_cast<void**>(service));
    }
    else
    {
        TestSession::FailTest("Unknown service type {0}", serviceTypeString);
    }
}

HRESULT ComTestServiceFactory::CreateReplica( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [retval][out] */ IFabricStatefulServiceReplica **service)
{
    wstring serviceTypeString(serviceType);

    // Deserialize initData to wstring
    wstring initDataStr = GetInitString(initializationData, initializationDataLength);

    if(serviceTypeString == TestStoreService::DefaultServiceType)
    {
        TestStoreServiceSPtr testStoreServiceSPtr = factory_.CreateTestStoreService(partitionId, serviceName, initDataStr);
        ComPointer<ComTestStoreService> comPointer = make_com<ComTestStoreService>(*testStoreServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
    }
    else if(serviceTypeString == TestPersistedStoreService::DefaultServiceType)
    {
        TestPersistedStoreServiceSPtr testPersistedStoreServiceSPtr = factory_.CreateTestPersistedStoreService(
            partitionId,
            replicaId,
            serviceName,
            initDataStr);
        ComPointer<ComTestPersistedStoreService> comPointer = make_com<ComTestPersistedStoreService>(*testPersistedStoreServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
    }
    else if (serviceTypeString == TXRService::DefaultServiceType)
    {
        TXRServiceSPtr txrServiceSPtr = factory_.CreateTXRService(
            partitionId,
            replicaId,
            serviceName,
            initDataStr);
        ComPointer<ComTXRService> comPointer = make_com<ComTXRService>(*txrServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
    }
    else if (serviceTypeString == TStoreService::DefaultServiceType)
    {
        TStoreServiceSPtr tstoreServiceSPtr = factory_.CreateTStoreService(
            partitionId,
            replicaId,
            serviceName,
            initDataStr);
        ComPointer<ComTStoreService> comPointer = make_com<ComTStoreService>(*tstoreServiceSPtr);
        return comPointer->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(service));
    }
    else
    {
        TestSession::FailTest("Unknown service type {0}", serviceTypeString);
    }
}

wstring ComTestServiceFactory::GetInitString(const byte * initData, ULONG nBytes)
{
    StringBody strBody(L"");
    if(nBytes > 0 && initData != NULL)
    {
        vector<byte> initDataV(initData, initData + nBytes);
        FabricSerializer::Deserialize(strBody, initDataV);
    }
    return strBody.String;
}
