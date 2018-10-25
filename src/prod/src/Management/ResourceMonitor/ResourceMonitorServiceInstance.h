// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ResourceMonitor
{
    class ResourceMonitorServiceInstance :
        public IFabricStatelessServiceInstance,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ResourceMonitorServiceInstance)

            BEGIN_COM_INTERFACE_LIST(ResourceMonitorServiceInstance)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceInstance)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
            END_COM_INTERFACE_LIST()
    public:
        ResourceMonitorServiceInstance(
            wstring const & serviceName,
            wstring const & serviceTypeName,
            Common::Guid const & partitionId,
            FABRIC_INSTANCE_ID instanceId,
            Common::ComPointer<IFabricRuntime> const & fabricRuntimeCPtr);
        virtual ~ResourceMonitorServiceInstance();

        /*************************************************************
        *   IFabricStatelessServiceInstance methods.
        *************************************************************/
        HRESULT STDMETHODCALLTYPE BeginOpen(
            /* [in] */ IFabricStatelessServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOpen(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);

        HRESULT STDMETHODCALLTYPE BeginClose(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndClose(
            /* [in] */ IFabricAsyncOperationContext *context);

        void Abort();

        __declspec(property(get = get_Agent)) std::shared_ptr<ResourceMonitoringAgent> AgentSPtr;
        std::shared_ptr<ResourceMonitoringAgent> & get_Agent() { return resourceMonitorAgent_; }

    private:
        // Name of this service
        wstring const serviceName_;
        // Service type of this service
        wstring const serviceTypeName_;
        // Partition ID for this stateless service.
        Common::Guid const partitionId_;
        // Instance ID of this particular replica.
        FABRIC_INSTANCE_ID const instanceId_;

        class OpenComAsyncOperationContext;
        class CloseComAsyncOperationContext;

        Common::ComPointer<IFabricRuntime> fabricRuntimeCPtr_;

        // The component that is doing the actual work.
        std::shared_ptr<ResourceMonitoringAgent> resourceMonitorAgent_;
    };
}
