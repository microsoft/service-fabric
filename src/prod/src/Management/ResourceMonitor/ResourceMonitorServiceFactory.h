// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ResourceMonitor
{
    class ResourceMonitorServiceFactory :
        public IFabricStatelessServiceFactory,
        protected Common::ComUnknownBase,
        public Common::ComponentRoot,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ResourceMonitor>
    {
        DENY_COPY(ResourceMonitorServiceFactory)

            COM_INTERFACE_LIST1(
                ResourceMonitorServiceFactory,
                IID_IFabricStatelessServiceFactory,
                IFabricStatelessServiceFactory
            )
    public:
        ResourceMonitorServiceFactory(Common::ComPointer<IFabricRuntime> const & fabricRuntimeCPtr);
        virtual ~ResourceMonitorServiceFactory();

        void Initialize();

        HRESULT STDMETHODCALLTYPE CreateInstance(
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_INSTANCE_ID instanceId,
            /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance);
    private:
        Common::ComPointer<IFabricRuntime> fabricRuntimeCPtr_;
    };
}
