// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    //
    // Forward decl.
    //
    class FabricAsyncOperationContext;

    class FabricDnsService :
        public KShared<FabricDnsService>,
        public IFabricStatelessServiceInstance,
        public IFabricServiceNotificationEventHandler
    {
        K_FORCE_SHARED(FabricDnsService);

        K_BEGIN_COM_INTERFACE_LIST(FabricDnsService)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricStatelessServiceInstance)
            K_COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
            K_COM_INTERFACE_ITEM(IID_IFabricServiceNotificationEventHandler, IFabricServiceNotificationEventHandler)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out FabricDnsService::SPtr& spService,
            __in KAllocator& allocator,
            __in IDnsTracer& tracer,
            __in LPCWSTR serviceTypeName,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in_ecount(initializationDataLength) const byte *initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId
            );

    private:
        FabricDnsService(
            __in IDnsTracer& tracer,
            __in LPCWSTR serviceTypeName,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in_ecount(initializationDataLength) const byte* initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId
            );

    public:
        // IFabricStatelessServiceInstance Impl.
        virtual HRESULT BeginOpen(
            __in IFabricStatelessServicePartition* partition,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            ) override;

        virtual HRESULT EndOpen(
            __in IFabricAsyncOperationContext* context,
            __out IFabricStringResult** serviceAddress
            ) override;

        virtual HRESULT BeginClose(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            ) override;

        virtual HRESULT EndClose(
            __in IFabricAsyncOperationContext* context
            ) override;

        virtual void Abort(void) override;

    public:
        // IFabricServiceNotificationEventHandler Impl.
        virtual HRESULT OnNotification(
            __in IFabricServiceNotification*
        ) override;

    private:
        void OnCloseCompleted(
            __in NTSTATUS status
        );

        void LoadConfig(
            __out DnsServiceParams& params
        );

        bool RegisterServiceNotifications();
        void UnegisterServiceNotifications();

    private:
        IDnsTracer& _tracer;
        KString::SPtr _spServiceName;
        DnsServiceParams _params;
        IDnsService::SPtr _spService;
        IFabricResolve::SPtr _spFabricResolve;
        ComPointer<IFabricStatelessServicePartition2> _spFabricHealth;
        ComPointer<IFabricServiceManagementClient4> _spServiceManagementClient;

        FabricAsyncOperationContext::SPtr _spOpenOperationContext;
        FabricAsyncOperationContext::SPtr _spCloseOperationContext;

        LONGLONG _filterId;
        KEvent _closeCompletedEvent;
    };
}
