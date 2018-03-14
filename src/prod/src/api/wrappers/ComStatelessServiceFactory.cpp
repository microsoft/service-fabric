// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStatelessServiceFactory Implementation
//

ComStatelessServiceFactory::ComStatelessServiceFactory(IStatelessServiceFactoryPtr const & impl)
    : IFabricStatelessServiceFactory(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStatelessServiceFactory::~ComStatelessServiceFactory()
{
}

HRESULT ComStatelessServiceFactory::CreateInstance( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_INSTANCE_ID instanceId,
    /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance)
{
    if (serviceType == NULL || serviceName == NULL || serviceInstance == NULL) 
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    wstring parsedServiceType;
    auto hr = StringUtility::LpcwstrToWstring(serviceType, false, parsedServiceType);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    NamingUri parsedServiceName;
    hr = NamingUri::TryParse(serviceName, "serviceName", parsedServiceName).ToHResult();
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    std::vector<byte> initializationDataBytes;
    if (initializationDataLength > 0)
    {
        initializationDataBytes.insert(
            initializationDataBytes.end(), 
            initializationData,
            initializationData + initializationDataLength);
    }

    IStatelessServiceInstancePtr serviceInstanceImpl;

    auto error = impl_->CreateInstance(
        parsedServiceType,
        parsedServiceName,
        initializationDataBytes,
        Guid(partitionId),
        instanceId,
        serviceInstanceImpl);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    ComPointer<ComStatelessServiceInstance> result = make_com<ComStatelessServiceInstance>(serviceInstanceImpl);
    return result->QueryInterface(IID_IFabricStatelessServiceInstance,  reinterpret_cast<void**>(serviceInstance));
}
