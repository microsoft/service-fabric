// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStatefulServiceFactory Implementation
//

ComStatefulServiceFactory::ComStatefulServiceFactory(IStatefulServiceFactoryPtr const & impl)
    : IFabricStatefulServiceFactory(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStatefulServiceFactory::~ComStatefulServiceFactory()
{
}

HRESULT ComStatefulServiceFactory::CreateReplica( 
    /* [in] */ LPCWSTR serviceType,
    /* [in] */ FABRIC_URI serviceName,
    /* [in] */ ULONG initializationDataLength,
    /* [size_is][in] */ const byte *initializationData,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [retval][out] */ IFabricStatefulServiceReplica **serviceReplica)
{
    if (serviceType == NULL || serviceName == NULL || serviceReplica == NULL) 
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

    IStatefulServiceReplicaPtr serviceReplicaImpl;

    auto error = impl_->CreateReplica(
        parsedServiceType,
        parsedServiceName,
        initializationDataBytes,
        Guid(partitionId),
        replicaId,
        serviceReplicaImpl);
    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(error.ToHResult()); }

    ComPointer<ComStatefulServiceReplica> result = make_com<ComStatefulServiceReplica>(serviceReplicaImpl);
    return result->QueryInterface(IID_IFabricStatefulServiceReplica,  reinterpret_cast<void**>(serviceReplica));
}
