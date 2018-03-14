// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace FabricTest;

//
// Constructor/Destructor.
//
SGComServiceFactory::SGComServiceFactory(
    SGServiceFactory & factory)
    : root_(factory.shared_from_this())
    , factory_(factory)
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComServiceFactory::SGComServiceFactory ({0}) - ctor - factory_({1})",
        this,
        &factory_);
}

SGComServiceFactory::~SGComServiceFactory()
{
    TestSession::WriteNoise(
        TraceSource, 
        "SGComServiceFactory::~SGComServiceFactory ({0}) - dtor",
        this);
}

//
// IFabricStatefulServiceFactory methods. 
//
HRESULT STDMETHODCALLTYPE SGComServiceFactory::CreateReplica(
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte* initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __out IFabricStatefulServiceReplica** serviceReplica)
{
    if (NULL == serviceReplica) { return E_POINTER; }

    SGStatefulServiceSPtr serviceSPtr = factory_.CreateStatefulService(serviceType, serviceName, initializationDataLength, initializationData, partitionId, replicaId);
    ComPointer<SGComStatefulService> serviceCPtr = make_com<SGComStatefulService>(*serviceSPtr);
    HRESULT hr =  serviceCPtr->QueryInterface(IID_IFabricStatefulServiceReplica, reinterpret_cast<void**>(serviceReplica));

    TestSession::WriteNoise(
        TraceSource, 
        "SGComServiceFactory::CreateReplica ({0}) - serviceType({1}) serviceName({2}) replicaId({3}) serviceReplica({4})",
        this,
        serviceType,
        serviceName,
        replicaId,
        *serviceReplica);

    return hr;
}

HRESULT STDMETHODCALLTYPE SGComServiceFactory::CreateInstance(
    __in LPCWSTR serviceType,
    __in LPCWSTR serviceName,
    __in ULONG initializationDataLength,
    __in const byte * initializationData,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_INSTANCE_ID instanceId,
    __out IFabricStatelessServiceInstance ** serviceInstance)
{
    if (NULL == serviceInstance) { return E_POINTER; }

    SGStatelessServiceSPtr serviceSPtr = factory_.CreateStatelessService(serviceType, serviceName, initializationDataLength, initializationData, partitionId, instanceId);
    ComPointer<SGComStatelessService> serviceCPtr = make_com<SGComStatelessService>(*serviceSPtr);
    HRESULT hr =  serviceCPtr->QueryInterface(IID_IFabricStatelessServiceInstance, reinterpret_cast<void**>(serviceInstance));

    TestSession::WriteNoise(
        TraceSource, 
        "SGComServiceFactory::CreateInstance ({0}) - serviceType({1}) serviceName({2}) instanceId({3}) serviceInstance({4})",
        this,
        serviceType,
        serviceName,
        instanceId,
        *serviceInstance);

    return hr;
}

StringLiteral const SGComServiceFactory::TraceSource("FabricTest.ServiceGroup.SGComServiceFactory");
