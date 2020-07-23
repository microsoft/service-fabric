// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceFactory.h"
#include "ServiceImplementation.h"
#include "StatefulServiceImplementation.h"

using namespace NativeServiceGroupMember;

ServiceFactory::ServiceFactory()
{
}

ServiceFactory::~ServiceFactory()
{
}

STDMETHODIMP ServiceFactory::CreateInstance( 
    __in LPCWSTR serviceType,
    __in FABRIC_URI serviceName,
    __in ULONG initializationDataLength,
    __in const byte *initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_INSTANCE_ID instanceId,
    __out IFabricStatelessServiceInstance **serviceInstance)
{
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);
    UNREFERENCED_PARAMETER(instanceId);

    if (serviceInstance == nullptr || serviceType == nullptr || serviceName == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        wstring type(serviceType);
        wstring uri(serviceName);
        wstring adderType(L"StatelessAdder");

        StringUtility::Replace(uri, type, adderType);

        ComPointer<ServiceImplementation> instance = make_com<ServiceImplementation>(uri);

        return instance->QueryInterface(IID_IFabricStatelessServiceInstance, reinterpret_cast<void**>(serviceInstance));
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
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
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(initializationDataLength);
    UNREFERENCED_PARAMETER(initializationData);
    UNREFERENCED_PARAMETER(replicaId);

    if (serviceType == nullptr || serviceName == nullptr)
    {
        return E_POINTER;
    }

    try
    {
        wstring type(serviceType);
        wstring uri(serviceName);
        wstring adderType(L"StatefulAdder");

        StringUtility::Replace(uri, type, adderType);

        ComPointer<StatefulServiceImplementation> instance = make_com<StatefulServiceImplementation>(uri);

        return instance->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(serviceReplica));
    }
    catch (bad_alloc const &) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}
