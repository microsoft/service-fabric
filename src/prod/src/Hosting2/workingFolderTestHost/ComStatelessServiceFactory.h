// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

class ComStatelessServiceFactory : 
    public IFabricStatelessServiceFactory,
    protected Common::ComUnknownBase,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
    DENY_COPY(ComStatelessServiceFactory)

    COM_INTERFACE_LIST1(
        ComStatelessServiceFactory,
        IID_IFabricStatelessServiceFactory,
        IFabricStatelessServiceFactory)

public:
    ComStatelessServiceFactory();
    virtual ~ComStatelessServiceFactory();

    HRESULT STDMETHODCALLTYPE CreateInstance( 
        /* [in] */ LPCWSTR serviceType,
        /* [in] */ FABRIC_URI serviceName,
        /* [in] */ ULONG initializationDataLength,
        /* [size_is][in] */ const byte *initializationData,
        /* [in] */ FABRIC_PARTITION_ID partitionId,
        /* [in] */ FABRIC_INSTANCE_ID instanceId,
        /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance);
private:
    std::wstring traceId_;
};
