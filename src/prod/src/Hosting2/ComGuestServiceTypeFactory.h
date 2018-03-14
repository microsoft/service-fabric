// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComGuestServiceTypeFactory : 
        public IFabricStatelessServiceFactory,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComGuestServiceTypeFactory)

        COM_INTERFACE_LIST1(
            ComGuestServiceTypeFactory,
            IID_IFabricStatelessServiceFactory,
            IFabricStatelessServiceFactory)

    public:
        ComGuestServiceTypeFactory(std::vector<ServiceModel::EndpointDescription> const & endpointDescriptions);
        
        virtual ~ComGuestServiceTypeFactory();

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
        std::vector<ServiceModel::EndpointDescription> endpointDescriptions_;
    };
}
