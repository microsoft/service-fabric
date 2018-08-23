//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComStatefulGuestServiceTypeFactory : 
        public IFabricStatefulServiceFactory,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComStatefulGuestServiceTypeFactory)

        COM_INTERFACE_LIST1(
            ComStatefulGuestServiceTypeFactory,
            IID_IFabricStatefulServiceFactory,
            IFabricStatefulServiceFactory)

    public:
        ComStatefulGuestServiceTypeFactory(IGuestServiceTypeHost & typeHost);
        
        virtual ~ComStatefulGuestServiceTypeFactory();

        HRESULT STDMETHODCALLTYPE CreateReplica(
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [retval][out] */ IFabricStatefulServiceReplica **serviceReplica) override;

    private:
        std::wstring traceId_;
        IGuestServiceTypeHost & typeHost_;
    };
}
