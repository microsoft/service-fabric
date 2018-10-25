// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComGuestServiceInstance :
        public IGuestServiceInstance,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComGuestServiceInstance)

        COM_INTERFACE_LIST1(
            ComGuestServiceInstance,
            IID_IFabricStatelessServiceInstance,
            IFabricStatelessServiceInstance)

    public:
        ComGuestServiceInstance(
            IGuestServiceTypeHost & typeHost,
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            Common::Guid const & partitionId,
            FABRIC_INSTANCE_ID instanceId);

        virtual ~ComGuestServiceInstance();

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
        
        void STDMETHODCALLTYPE Abort(void);

    public:
        void OnActivatedCodePackageTerminated(
            CodePackageEventDescription && eventDesc) override;

        bool Test_IsInstanceFaulted();
        void Test_SetReplicaFaulted(bool value);

    private:
        static std::wstring BuildTraceInfo(
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            Common::Guid const & partitionId,
            ::FABRIC_INSTANCE_ID instanceId);

        void RegisterForEvents();
        void UnregisterForEvents();

    private:
        class EndpointsJsonWrapper;

    private:
        IGuestServiceTypeHost & typeHost_;
        std::wstring const serviceName_;
        std::wstring const serviceTypeName_;
        std::wstring const serviceTraceInfo_;
        std::wstring endpointsAsJsonStr_;
        Common::Guid const partitionId_;
        ::FABRIC_INSTANCE_ID const instanceId_;
        Common::atomic_bool hasReportedFault_;
        ULONGLONG eventRegistrationHandle_;
        Common::ComPointer<IFabricStatelessServicePartition2> partition_;
    };
}
