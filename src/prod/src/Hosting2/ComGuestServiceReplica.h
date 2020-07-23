//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComGuestServiceReplica : 
        public IGuestServiceReplica,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComGuestServiceReplica)

        COM_INTERFACE_LIST1(
            ComGuestServiceReplica,
            IID_IFabricStatefulServiceReplica,
            IFabricStatefulServiceReplica)

    public:
        ComGuestServiceReplica(
            IGuestServiceTypeHost & typeHost,
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId);

        virtual ~ComGuestServiceReplica();

        HRESULT STDMETHODCALLTYPE BeginOpen(
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndOpen(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator) override;

        HRESULT STDMETHODCALLTYPE BeginChangeRole(
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndChangeRole(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress) override;

        HRESULT STDMETHODCALLTYPE BeginClose(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndClose(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        void STDMETHODCALLTYPE Abort(void) override;

    public:
        void OnActivatedCodePackageTerminated(
            CodePackageEventDescription && eventDesc) override;

        bool Test_IsReplicaFaulted();
        void Test_SetReplicaFaulted(bool value);

    private:
        static std::wstring BuildTraceInfo(
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            Common::Guid const & partitionId,
            ::FABRIC_REPLICA_ID instanceId);

        void RegisterForEvents();
        void UnregisterForEvents();

    private:
        IGuestServiceTypeHost & typeHost_;
        std::wstring const serviceName_;
        std::wstring const serviceTypeName_;
        std::wstring const serviceTraceInfo_;
        std::wstring endpointsAsJsonStr_;
        Common::Guid const partitionId_;
        ::FABRIC_REPLICA_ID const replicaId_;
        ::FABRIC_REPLICA_ROLE currenRole_;
        Common::atomic_bool hasReportedFault_;
        bool isActivationInProgress_;
        bool isDeactivationInProgress_;
        ULONGLONG eventRegistrationHandle_;
        Common::ComPointer<IFabricStatefulServicePartition2> partition_;
        ComPointer<ComDummyReplicator> dummyReplicator_;
        std::wstring partitionName_;
    };
}
