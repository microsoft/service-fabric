// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {70073d24-acee-47c5-b23b-bef845abd9bf}
    static const GUID CLSID_ComInfrastructureServiceAgent = 
        {0x70073d24,0xacee,0x47c5,{0xb2,0x3b,0xbe,0xf8,0x45,0xab,0xd9,0xbf}};

    class ComInfrastructureServiceAgent :
        public IFabricInfrastructureServiceAgent,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComInfrastructureServiceAgent)

        BEGIN_COM_INTERFACE_LIST(ComInfrastructureServiceAgent)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricInfrastructureServiceAgent)
            COM_INTERFACE_ITEM(IID_IFabricInfrastructureServiceAgent, IFabricInfrastructureServiceAgent)
            COM_INTERFACE_ITEM(CLSID_ComInfrastructureServiceAgent, ComInfrastructureServiceAgent)
        END_COM_INTERFACE_LIST()

    public:
        ComInfrastructureServiceAgent(IInfrastructureServiceAgentPtr const & impl);
        virtual ~ComInfrastructureServiceAgent();

        IInfrastructureServiceAgentPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricInfrastructureServiceAgent methods
        // 
        HRESULT STDMETHODCALLTYPE RegisterInfrastructureServiceFactory(
            /* [in] */ IFabricStatefulServiceFactory *);

        HRESULT STDMETHODCALLTYPE RegisterInfrastructureService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID,
            /* [in] */ IFabricInfrastructureService *,
            /* [out, retval] */ IFabricStringResult ** serviceAddress);

        HRESULT STDMETHODCALLTYPE UnregisterInfrastructureService(
            /* [in] */ FABRIC_PARTITION_ID,
            /* [in] */ FABRIC_REPLICA_ID);

        HRESULT STDMETHODCALLTYPE BeginStartInfrastructureTask(
            /* [in] */ FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION * taskDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStartInfrastructureTask(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginFinishInfrastructureTask(
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndFinishInfrastructureTask(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginQueryInfrastructureTask(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndQueryInfrastructureTask(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricInfrastructureTaskQueryResult ** queryResult);

    private:
        class ComStartInfrastructureTaskAsyncOperation;
        class ComFinishInfrastructureTaskAsyncOperation;
        class ComQueryInfrastructureTaskAsyncOperation;
        class ComQueryResult;

        IInfrastructureServiceAgentPtr impl_;
    };
}

