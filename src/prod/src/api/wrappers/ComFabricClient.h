// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //
    // Used for location change callbacks so that if a callback is fired
    // it can keep the client alive
    struct ITentativeFabricServiceClient : public IFabricServiceManagementClient6
    {
        virtual ULONG STDMETHODCALLTYPE TryAddRef() = 0;
    };

    class LocationChangeCallbackAdapter;
    typedef std::shared_ptr<LocationChangeCallbackAdapter> LocationChangeCallbackAdapterSPtr;

    //
    // This class implements the COM wrapper for the fabric client.
    //
    class ComFabricClient :
        public IFabricClientSettings2,
        public IFabricPropertyManagementClient2,
        public ITentativeFabricServiceClient,
        public IFabricServiceGroupManagementClient4,
        public IFabricApplicationManagementClient10,
        public IFabricClusterManagementClient10,
        public IFabricQueryClient10,
        public IFabricRepairManagementClient2,
        public IFabricHealthClient4,
        public IInternalFabricInfrastructureServiceClient,
        public IInternalFabricQueryClient2,
        public IInternalFabricClusterManagementClient,
        public IInternalFabricApplicationManagementClient2,
        public IInternalFabricServiceManagementClient2,
        public IFabricAccessControlClient,
        public IFabricImageStoreClient,
        public IFabricInfrastructureServiceClient,
        public IFabricTestManagementClient4,
        public IFabricTestManagementClientInternal2,
        public IFabricFaultManagementClient,
        public IFabricFaultManagementClientInternal,
        public IFabricNetworkManagementClient,
        public Naming::IFabricTestClient,
        public IFabricSecretStoreClient,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricClient)

        BEGIN_COM_INTERFACE_LIST(ComFabricClient)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricClientSettings2)
            COM_INTERFACE_ITEM(IID_IFabricClientSettings,IFabricClientSettings)
            COM_INTERFACE_ITEM(IID_IFabricClientSettings2,IFabricClientSettings2)
            COM_INTERFACE_ITEM(IID_IFabricPropertyManagementClient,IFabricPropertyManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricPropertyManagementClient2,IFabricPropertyManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient,IFabricServiceManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient2,IFabricServiceManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient3,IFabricServiceManagementClient3)
            COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient4,IFabricServiceManagementClient4)
            COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient5, IFabricServiceManagementClient5)
            COM_INTERFACE_ITEM(IID_IFabricServiceManagementClient6, IFabricServiceManagementClient6)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupManagementClient,IFabricServiceGroupManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupManagementClient2,IFabricServiceGroupManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupManagementClient3,IFabricServiceGroupManagementClient3)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupManagementClient4, IFabricServiceGroupManagementClient4)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient,IFabricApplicationManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient2,IFabricApplicationManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient3,IFabricApplicationManagementClient3)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient4,IFabricApplicationManagementClient4)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient5,IFabricApplicationManagementClient5)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient6, IFabricApplicationManagementClient6)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient7, IFabricApplicationManagementClient7)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient8, IFabricApplicationManagementClient8)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient9, IFabricApplicationManagementClient9)
            COM_INTERFACE_ITEM(IID_IFabricApplicationManagementClient10, IFabricApplicationManagementClient10)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient,IFabricClusterManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient2,IFabricClusterManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient3,IFabricClusterManagementClient3)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient4,IFabricClusterManagementClient4)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient5,IFabricClusterManagementClient5)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient6,IFabricClusterManagementClient6)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient7, IFabricClusterManagementClient7)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient8, IFabricClusterManagementClient8)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient9, IFabricClusterManagementClient9)
            COM_INTERFACE_ITEM(IID_IFabricClusterManagementClient10, IFabricClusterManagementClient10)
            COM_INTERFACE_ITEM(IID_IInternalFabricClusterManagementClient, IInternalFabricClusterManagementClient)
            COM_INTERFACE_ITEM(IID_IInternalFabricApplicationManagementClient, IInternalFabricApplicationManagementClient)
            COM_INTERFACE_ITEM(IID_IInternalFabricApplicationManagementClient2, IInternalFabricApplicationManagementClient2)
            COM_INTERFACE_ITEM(IID_IInternalFabricServiceManagementClient, IInternalFabricServiceManagementClient)
            COM_INTERFACE_ITEM(IID_IInternalFabricServiceManagementClient2, IInternalFabricServiceManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricRepairManagementClient,IFabricRepairManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricRepairManagementClient2,IFabricRepairManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricHealthClient,IFabricHealthClient)
            COM_INTERFACE_ITEM(IID_IFabricHealthClient2,IFabricHealthClient2)
            COM_INTERFACE_ITEM(IID_IFabricHealthClient3, IFabricHealthClient3)
            COM_INTERFACE_ITEM(IID_IFabricHealthClient4, IFabricHealthClient4)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient,IFabricQueryClient)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient2,IFabricQueryClient2)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient3,IFabricQueryClient3)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient4,IFabricQueryClient4)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient5, IFabricQueryClient5)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient6, IFabricQueryClient6)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient7, IFabricQueryClient7)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient8, IFabricQueryClient8)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient9, IFabricQueryClient9)
            COM_INTERFACE_ITEM(IID_IFabricQueryClient10, IFabricQueryClient10)
            COM_INTERFACE_ITEM(IID_IInternalFabricInfrastructureServiceClient, IInternalFabricInfrastructureServiceClient)
            COM_INTERFACE_ITEM(IID_IInternalFabricQueryClient, IInternalFabricQueryClient)
            COM_INTERFACE_ITEM(IID_IInternalFabricQueryClient2, IInternalFabricQueryClient2)
            COM_INTERFACE_ITEM(IID_IFabricAccessControlClient, IFabricAccessControlClient)
            COM_INTERFACE_ITEM(IID_IFabricImageStoreClient,IFabricImageStoreClient)
            COM_INTERFACE_ITEM(IID_IFabricInfrastructureServiceClient, IFabricInfrastructureServiceClient)
            COM_INTERFACE_ITEM(IID_IFabricTestManagementClient, IFabricTestManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricTestManagementClient2, IFabricTestManagementClient2)
            COM_INTERFACE_ITEM(IID_IFabricTestManagementClient3, IFabricTestManagementClient3)
            COM_INTERFACE_ITEM(IID_IFabricTestManagementClient4, IFabricTestManagementClient4)
            COM_INTERFACE_ITEM(IID_IFabricTestManagementClientInternal, IFabricTestManagementClientInternal)
            COM_INTERFACE_ITEM(IID_IFabricTestManagementClientInternal2, IFabricTestManagementClientInternal2)
            COM_INTERFACE_ITEM(IID_IFabricFaultManagementClient, IFabricFaultManagementClient)
            COM_INTERFACE_ITEM(IID_IFabricFaultManagementClientInternal, IFabricFaultManagementClientInternal)
            COM_INTERFACE_ITEM(IID_IFabricNetworkManagementClient, IFabricNetworkManagementClient)
            COM_INTERFACE_ITEM(Naming::IID_IFabricTestClient, Naming::IFabricTestClient)
            COM_INTERFACE_ITEM(IID_IFabricSecretStoreClient, IFabricSecretStoreClient)
        END_COM_INTERFACE_LIST()

    public:
        __declspec(property(get=get_ServiceMgmtClient)) Api::IServiceManagementClientPtr const & ServiceMgmtClient;
        Api::IServiceManagementClientPtr const & get_ServiceMgmtClient() const { return serviceMgmtClient_; }

        ComFabricClient(IClientFactoryPtr const& factoryPtr);
        virtual ~ComFabricClient();
        ULONG STDMETHODCALLTYPE TryAddRef();

        static HRESULT CreateComFabricClient(IClientFactoryPtr const&, __out Common::ComPointer<ComFabricClient>&);

        //
        // IFabricClientSettings methods.
        //

        HRESULT STDMETHODCALLTYPE SetSecurityCredentials(
            /* [in] */ FABRIC_SECURITY_CREDENTIALS const* securityCredentials);

        HRESULT STDMETHODCALLTYPE SetKeepAlive(
            /* [in] */ ULONG keepAliveIntervalInSeconds);

        //
        // IFabricClientSettings2 methods.
        //

        HRESULT STDMETHODCALLTYPE GetSettings(
            /* [out, retval] */ IFabricClientSettingsResult **result);

        HRESULT STDMETHODCALLTYPE SetSettings(
            /* [in] */ FABRIC_CLIENT_SETTINGS const* fabricClientSettings);

        //
        // IFabricClusterManagementClient methods.
        //

        HRESULT STDMETHODCALLTYPE BeginNodeStateRemoved(
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndNodeStateRemoved(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginRecoverPartitions(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRecoverPartitions(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricClusterManagementClient2 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginDeactivateNode(
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ FABRIC_NODE_DEACTIVATION_INTENT intent,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeactivateNode(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginActivateNode(
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndActivateNode(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginProvisionFabric(
            /* [in] */ LPCWSTR codeFilepath,
            /* [in] */ LPCWSTR clusterManifestFilepath,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndProvisionFabric(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginUpgradeFabric(
            /* [in] */ const FABRIC_UPGRADE_DESCRIPTION *upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpgradeFabric(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetFabricUpgradeProgress(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetFabricUpgradeProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricUpgradeProgressResult2 **result);

        HRESULT STDMETHODCALLTYPE BeginMoveNextFabricUpgradeDomain(
            /* [in] */ IFabricUpgradeProgressResult2 *progress,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndMoveNextFabricUpgradeDomain(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginMoveNextFabricUpgradeDomain2(
            /* [in] */ LPCWSTR nextDomain,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndMoveNextFabricUpgradeDomain2(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginStartInfrastructureTask(
            /* [in] */ const FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION *task,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndStartInfrastructureTask(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isComplete);

        HRESULT STDMETHODCALLTYPE BeginFinishInfrastructureTask(
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndFinishInfrastructureTask(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isComplete);

        HRESULT STDMETHODCALLTYPE BeginUnprovisionFabric(
            /* [in] */ LPCWSTR codeVersion,
            /* [in] */ LPCWSTR configVersion,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUnprovisionFabric(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetClusterManifest(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetClusterManifest(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        HRESULT STDMETHODCALLTYPE BeginRecoverPartition(
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRecoverPartition(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginRecoverServicePartitions(
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRecoverServicePartitions(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginRecoverSystemPartitions(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRecoverSystemPartitions(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricClusterManagementClient3
        //

        HRESULT STDMETHODCALLTYPE BeginUpdateFabricUpgrade(
            /* [in] */ const FABRIC_UPGRADE_UPDATE_DESCRIPTION *updateDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpdateFabricUpgrade(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginAddUnreliableTransportBehavior(
            /*[in]*/ LPCWSTR nodeName,
            /*[in]*/ LPCWSTR name,
            /*[in]*/ const FABRIC_UNRELIABLETRANSPORT_BEHAVIOR * unreliableTransportBehavior,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndAddUnreliableTransportBehavior(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRemoveUnreliableTransportBehavior(
            /*[in]*/ LPCWSTR nodeName,
            /*[in]*/ LPCWSTR name,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRemoveUnreliableTransportBehavior(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetTransportBehaviorList(
            /*[in]*/ LPCWSTR nodeName,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetTransportBehaviorList(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /*[out, retval]*/  IFabricStringListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginStopNode(
            /*[in]*/ const FABRIC_STOP_NODE_DESCRIPTION * stopNodeDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStopNode(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRestartNode(
            /*[in]*/ const FABRIC_RESTART_NODE_DESCRIPTION * restartNodeDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartNode(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginStartNode(
            /*[in]*/ const FABRIC_START_NODE_DESCRIPTION * startNodeDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStartNode(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE CopyClusterPackage(
            /* [in] */ LPCWSTR imageStoreConnectionString,
            /* [in] */ LPCWSTR clusterManifestPath,
            /* [in] */ LPCWSTR clusterManifestPathInImageStore,
            /* [in] */ LPCWSTR codePackagePath,
            /* [in] */ LPCWSTR codePackagePathInImageStore);

        HRESULT STDMETHODCALLTYPE RemoveClusterPackage(
            /* [in] */ LPCWSTR imageStoreConnectionString,
            /* [in] */ LPCWSTR clusterManifestPathInImageStore,
            /* [in] */ LPCWSTR codePackagePathInImageStore);

        HRESULT STDMETHODCALLTYPE BeginRestartNodeInternal(
            /*[in]*/ const FABRIC_RESTART_NODE_DESCRIPTION * restartNodeDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartNodeInternal(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginStartNodeInternal(
            /*[in]*/ const FABRIC_START_NODE_DESCRIPTION * startNodeDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStartNodeInternal(
            /*[in]*/ IFabricAsyncOperationContext * context);

        //
        // IFabricClusterManagementClient4 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginRollbackFabricUpgrade(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndRollbackFabricUpgrade(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricClusterManagementClient5 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginResetPartitionLoad(
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndResetPartitionLoad(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricClusterManagementClient6 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginToggleVerboseServicePlacementHealthReporting(
            /* [in] */ BOOLEAN enabled,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndToggleVerboseServicePlacementHealthReporting(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricClusterManagementClient7 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginUpgradeConfiguration(
            /* [in] */ const FABRIC_START_UPGRADE_DESCRIPTION * startUpgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndUpgradeConfiguration(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetClusterConfigurationUpgradeStatus(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetClusterConfigurationUpgradeStatus(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricOrchestrationUpgradeStatusResult ** result);

        HRESULT STDMETHODCALLTYPE BeginGetClusterConfiguration(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetClusterConfiguration(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetUpgradesPendingApproval(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetUpgradesPendingApproval(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginStartApprovedUpgrades(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStartApprovedUpgrades(
            /* [in] */ IFabricAsyncOperationContext * context);

        //
        // IFabricClusterManagementClient8 methods.
        //

        HRESULT BeginGetClusterManifest2(
            /* [in] */ const FABRIC_CLUSTER_MANIFEST_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT EndGetClusterManifest2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        //
        // IFabricClusterManagementClient9 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginGetUpgradeOrchestrationServiceState(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetUpgradeOrchestrationServiceState(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        HRESULT STDMETHODCALLTYPE BeginSetUpgradeOrchestrationServiceState(
            /* [in] */ LPCWSTR state,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndSetUpgradeOrchestrationServiceState(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricUpgradeOrchestrationServiceStateResult **result);

        //
        // IFabricClusterManagementClient10 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginGetClusterConfiguration2(
            /* [in] */ LPCWSTR apiVersion,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetClusterConfiguration2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        //
        // IFabricHealthClient methods.
        //
        HRESULT STDMETHODCALLTYPE ReportHealth(
            /* [in] */ const FABRIC_HEALTH_REPORT * publicHealthReport);

        HRESULT STDMETHODCALLTYPE BeginGetClusterHealth(
            /* [in] */ const FABRIC_CLUSTER_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

         HRESULT STDMETHODCALLTYPE EndGetClusterHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricClusterHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetNodeHealth(
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ const FABRIC_CLUSTER_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetNodeHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricNodeHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetApplicationHealth(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetApplicationHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricApplicationHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetServiceHealth(
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetServiceHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetPartitionHealth(
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetPartitionHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetReplicaHealth(
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetReplicaHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicaHealthResult **result);

          HRESULT STDMETHODCALLTYPE BeginGetDeployedApplicationHealth(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetDeployedApplicationHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricDeployedApplicationHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetDeployedServicePackageHealth(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ const FABRIC_APPLICATION_HEALTH_POLICY *healthPolicy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetDeployedServicePackageHealth(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricDeployedServicePackageHealthResult **result);

         //
         // IFabricHealthClient2 methods.
         //
         HRESULT STDMETHODCALLTYPE BeginGetClusterHealth2(
             /* [in] */ const FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetClusterHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricClusterHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetNodeHealth2(
             /* [in] */ const FABRIC_NODE_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetNodeHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricNodeHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetApplicationHealth2(
             /* [in] */ const FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetApplicationHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricApplicationHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetServiceHealth2(
             /* [in] */ const FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetServiceHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricServiceHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetPartitionHealth2(
             /* [in] */ const FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetPartitionHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricPartitionHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetReplicaHealth2(
             /* [in] */ const FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetReplicaHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricReplicaHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetDeployedApplicationHealth2(
             /* [in] */ const FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetDeployedApplicationHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricDeployedApplicationHealthResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetDeployedServicePackageHealth2(
             /* [in] */ const FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetDeployedServicePackageHealth2(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricDeployedServicePackageHealthResult **result);

         //
         // IFabricHealthClient3 methods.
         //
         HRESULT STDMETHODCALLTYPE BeginGetClusterHealthChunk(
             /* [in] */ const FABRIC_CLUSTER_HEALTH_CHUNK_QUERY_DESCRIPTION *queryDescription,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback *callback,
             /* [retval][out] */ IFabricAsyncOperationContext **context);
         HRESULT STDMETHODCALLTYPE EndGetClusterHealthChunk(
             /* [in] */ IFabricAsyncOperationContext *context,
             /* [retval][out] */ IFabricGetClusterHealthChunkResult **result);

         //
         // IFabricHealthClient4 methods.
         //
         HRESULT STDMETHODCALLTYPE ReportHealth2(
             /* [in] */ const FABRIC_HEALTH_REPORT * publicHealthReport,
             /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS * sendOptions);

        //
        // IFabricPropertyManagementClient2 methods.
        //
        HRESULT STDMETHODCALLTYPE BeginCreateName(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateName(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeleteName(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteName(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginNameExists(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndNameExists(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *value);

        HRESULT STDMETHODCALLTYPE BeginEnumerateSubNames(
            /* [in] */ FABRIC_URI name,
            /* [in] */ IFabricNameEnumerationResult *previousResult,
            /* [in] */ BOOLEAN recursive,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndEnumerateSubNames(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricNameEnumerationResult **result);

        HRESULT STDMETHODCALLTYPE BeginPutPropertyBinary(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ ULONG dataLength,
            /* [size_is][in] */ const BYTE *data,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPutPropertyBinary(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginPutPropertyInt64(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ LONGLONG data,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPutPropertyInt64(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginPutPropertyDouble(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ double data,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPutPropertyDouble(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginPutPropertyWString(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ LPCWSTR data,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPutPropertyWString(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginPutPropertyGuid(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ const GUID *data,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPutPropertyGuid(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeleteProperty(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteProperty(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetPropertyMetadata(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetPropertyMetadata(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPropertyMetadataResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetProperty(
            /* [in] */ FABRIC_URI name,
            /* [in] */ LPCWSTR propertyName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetProperty(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPropertyValueResult **result);

        HRESULT STDMETHODCALLTYPE BeginSubmitPropertyBatch(
            /* [in] */ FABRIC_URI name,
            /* [in] */ ULONG operationCount,
            /* [size_is][in] */ const FABRIC_PROPERTY_BATCH_OPERATION *operations,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndSubmitPropertyBatch(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONG *failedOperationIndexInRequest,
            /* [retval][out] */ IFabricPropertyBatchResult **result);

        HRESULT STDMETHODCALLTYPE BeginEnumerateProperties(
            /* [in] */ FABRIC_URI name,
            /* [in] */ BOOLEAN includeValues,
            /* [in] */ IFabricPropertyEnumerationResult *previousResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndEnumerateProperties(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPropertyEnumerationResult **result);

        virtual HRESULT STDMETHODCALLTYPE BeginPutCustomPropertyOperation(
            /* [in] */ FABRIC_URI name,
            /* [in] */ const FABRIC_PUT_CUSTOM_PROPERTY_OPERATION *propertyOperation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndPutCustomPropertyOperation(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricQueryClient methods.
        //

        HRESULT STDMETHODCALLTYPE BeginGetNodeList(
            /* [in] */ const FABRIC_NODE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetNodeList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetNodeListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetApplicationTypeList(
            /* [in] */ const FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationTypeList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetApplicationTypeListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetServiceTypeList(
            /* [in] */ const FABRIC_SERVICE_TYPE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceTypeList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetServiceTypeListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetApplicationList(
            /* [in] */ const FABRIC_APPLICATION_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetApplicationListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetServiceList(
            /* [in] */ const FABRIC_SERVICE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetServiceListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetPartitionList(
            /* [in] */ const FABRIC_SERVICE_PARTITION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetPartitionList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetPartitionListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetReplicaList(
            /* [in] */ const FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetReplicaList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetReplicaListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedApplicationList(
            /* [in] */ const FABRIC_DEPLOYED_APPLICATION_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedApplicationList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedApplicationListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedServicePackageList(
            /* [in] */ const FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedServicePackageList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedServicePackageListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedServiceTypeList(
            /* [in] */ const FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedServiceTypeList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedServiceTypeListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedCodePackageList(
            /* [in] */ const FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedCodePackageList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedCodePackageListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedReplicaList(
            /* [in] */ const FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedReplicaList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedReplicaListResult **result);

        //
        // IFabricQueryClient2 methods.
        //
        HRESULT STDMETHODCALLTYPE BeginGetDeployedReplicaDetail(
            /* [in] */ const FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedReplicaDetail(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedServiceReplicaDetailResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetClusterLoadInformation(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetClusterLoadInformation(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetClusterLoadInformationResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetPartitionLoadInformation(
            /* [in] */ const FABRIC_PARTITION_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetPartitionLoadInformation(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetPartitionLoadInformationResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetProvisionedFabricCodeVersionList(
            /* [in] */ const FABRIC_PROVISIONED_CODE_VERSION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetProvisionedFabricCodeVersionList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetProvisionedCodeVersionListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetProvisionedFabricConfigVersionList(
            /* [in] */ const FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetProvisionedFabricConfigVersionList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetProvisionedConfigVersionListResult **result);


        //
        // IFabricQueryClient3
        //

        HRESULT STDMETHODCALLTYPE BeginGetNodeLoadInformation(
            /* [in] */ const FABRIC_NODE_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetNodeLoadInformation(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetNodeLoadInformationResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetReplicaLoadInformation(
            /* [in] */ const FABRIC_REPLICA_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetReplicaLoadInformation(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetReplicaLoadInformationResult **result);

        //
        // IFabricQueryClient4 methods
        //
        HRESULT STDMETHODCALLTYPE BeginGetServiceGroupMemberList(
            /* [in] */ const FABRIC_SERVICE_GROUP_MEMBER_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceGroupMemberList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetServiceGroupMemberListResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetServiceGroupMemberTypeList(
            /* [in] */ const FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceGroupMemberTypeList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetServiceGroupMemberTypeListResult **result);

        //
        // IFabricQueryClient5 methods
        //
        HRESULT STDMETHODCALLTYPE BeginGetUnplacedReplicaInformation(
            /* [in] */ const FABRIC_UNPLACED_REPLICA_INFORMATION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetUnplacedReplicaInformation(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetUnplacedReplicaInformationResult **result);

        //
        // IFabricQueryClient6 methods
        //
        HRESULT STDMETHODCALLTYPE EndGetNodeList2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetNodeListResult2 **result);

        HRESULT STDMETHODCALLTYPE BeginGetNodeListInternal(
            /* [in] */ const FABRIC_NODE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ BOOLEAN excludeStoppedNodeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetNodeList2Internal(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetNodeListResult2 **result);

        HRESULT STDMETHODCALLTYPE EndGetApplicationList2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetApplicationListResult2 **result);

        HRESULT STDMETHODCALLTYPE EndGetServiceList2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetServiceListResult2 **result);

        HRESULT STDMETHODCALLTYPE EndGetPartitionList2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetPartitionListResult2 **result);

        HRESULT STDMETHODCALLTYPE EndGetReplicaList2(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetReplicaListResult2 **result);

        //
        // IFabricQueryClient7 methods
        //
        HRESULT STDMETHODCALLTYPE BeginGetApplicationLoadInformation(
            /* [in] */ const FABRIC_APPLICATION_LOAD_INFORMATION_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationLoadInformation(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetApplicationLoadInformationResult **result);

        //
        // IFabricQueryClient8 methods
        //
        HRESULT STDMETHODCALLTYPE BeginGetServiceName(
            /* [in] */ const FABRIC_SERVICE_NAME_QUERY_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceName(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetServiceNameResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetApplicationName(
            /* [in] */ const FABRIC_APPLICATION_NAME_QUERY_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationName(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetApplicationNameResult **result);

        //
        // IFabricQueryClient9 methods
        //
        HRESULT STDMETHODCALLTYPE BeginGetApplicationTypePagedList(
            /* [in] */ const PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationTypePagedList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetApplicationTypePagedListResult **result);

        //
        // IFabricQueryClient10 methods
        //
        HRESULT STDMETHODCALLTYPE BeginGetDeployedApplicationPagedList(
            /* [in] */ const FABRIC_PAGED_DEPLOYED_APPLICATION_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetDeployedApplicationPagedList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetDeployedApplicationPagedListResult **result);

        //
        // IInternalFabricQueryClient methods.
        //
        HRESULT STDMETHODCALLTYPE BeginInternalQuery(
            /* [in] */ LPCWSTR queryName,
            /* [in] */ const FABRIC_STRING_PAIR_LIST * queryArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndInternalQuery(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IInternalFabricQueryResult ** result);

        //
        // IInternalFabricQueryClient2 methods.
        //
        HRESULT STDMETHODCALLTYPE EndInternalQuery2(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IInternalFabricQueryResult2 ** result);

        //
        // IFabricServiceGroupManagementClient methods
        //
        HRESULT STDMETHODCALLTYPE BeginCreateServiceGroup(
            /* [in] */ const FABRIC_SERVICE_GROUP_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateServiceGroup(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginCreateServiceGroupFromTemplate(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ ULONG InitializationDataSize,
            /* [size_is][in] */ BYTE *InitializationData,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateServiceGroupFromTemplate(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeleteServiceGroup(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteServiceGroup(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetServiceGroupDescription(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceGroupDescription(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceGroupDescriptionResult **result);

        HRESULT STDMETHODCALLTYPE BeginUpdateServiceGroup(
            /* [in] */ FABRIC_URI name,
            /* [in] */ const FABRIC_SERVICE_GROUP_UPDATE_DESCRIPTION *serviceGroupUpdateDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpdateServiceGroup(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginCreateServiceGroupFromTemplate2(
            /* [in] */ const FABRIC_SERVICE_GROUP_FROM_TEMPLATE_DESCRIPTION *serviceGroupFromTemplateDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateServiceGroupFromTemplate2(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricServiceManagementClient methods
        //
        HRESULT STDMETHODCALLTYPE BeginCreateService(
            /* [in] */ const FABRIC_SERVICE_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateService(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginCreateServiceFromTemplate(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ ULONG InitializationDataSize,
            /* [size_is][in] */ BYTE *InitializationData,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateServiceFromTemplate(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginUpdateService(
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ const FABRIC_SERVICE_UPDATE_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpdateService(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeleteService(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteService(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetServiceDescription(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceDescription(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceDescriptionResult **result);

        HRESULT STDMETHODCALLTYPE RegisterServicePartitionResolutionChangeHandler(
            /* [in] */ FABRIC_URI name,
            /* [in] */ FABRIC_PARTITION_KEY_TYPE keyType,
            /* [in] */ const void *partitionKey,
            /* [in] */ IFabricServicePartitionResolutionChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);

        HRESULT STDMETHODCALLTYPE UnregisterServicePartitionResolutionChangeHandler(
            /* [in] */ LONGLONG callbackHandle);

        HRESULT STDMETHODCALLTYPE BeginResolveServicePartition(
            /* [in] */ FABRIC_URI name,
            /* [in] */ FABRIC_PARTITION_KEY_TYPE partitionKeyType,
            /* [in] */ const void *partitionKey,
            /* [in] */ IFabricResolvedServicePartitionResult *previousResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndResolveServicePartition(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricResolvedServicePartitionResult **result);

         HRESULT STDMETHODCALLTYPE BeginGetServiceManifest(
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetServiceManifest(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        HRESULT STDMETHODCALLTYPE BeginRemoveReplica(
            /* [in] */ const FABRIC_REMOVE_REPLICA_DESCRIPTION * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRemoveReplica(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRestartReplica(
            /* [in] */ const FABRIC_RESTART_REPLICA_DESCRIPTION * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartReplica(
            /* [in]  */IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRegisterServiceNotificationFilter(
             /* [in] */ const FABRIC_SERVICE_NOTIFICATION_FILTER_DESCRIPTION * description,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback * callback,
             /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRegisterServiceNotificationFilter(
             /* [in] */ IFabricAsyncOperationContext * context,
             /* [out, retval] */ LONGLONG * filterId);

        HRESULT STDMETHODCALLTYPE BeginUnregisterServiceNotificationFilter(
             /* [in] */ LONGLONG filterId,
             /* [in] */ DWORD timeoutMilliseconds,
             /* [in] */ IFabricAsyncOperationCallback * callback,
             /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndUnregisterServiceNotificationFilter(
             /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginMovePrimary(
            /* [in] */ const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION *movePrimaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndMovePrimary(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginMoveSecondary(
            /* [in] */ const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION *moveSecondaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndMoveSecondary(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetCachedServiceDescription(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetCachedServiceDescription(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceDescriptionResult **result);

        HRESULT BeginDeleteService2(
            /* [in] */ const FABRIC_DELETE_SERVICE_DESCRIPTION * deleteDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT EndDeleteService2(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginCreateServiceFromTemplate2(
            /* [in] */ const FABRIC_SERVICE_FROM_TEMPLATE_DESCRIPTION *serviceFromTemplateDesc,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateServiceFromTemplate2(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricApplicationManagementClient methods.
        //

        HRESULT STDMETHODCALLTYPE BeginProvisionApplicationType(
            /* [in] */ LPCWSTR applicationBuildPath,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndProvisionApplicationType(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginCreateApplication(
            /* [in] */ const FABRIC_APPLICATION_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateApplication(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginUpgradeApplication(
            /* [in] */ const FABRIC_APPLICATION_UPGRADE_DESCRIPTION *upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpgradeApplication(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetApplicationUpgradeProgress(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationUpgradeProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricApplicationUpgradeProgressResult2 **result);

        HRESULT STDMETHODCALLTYPE BeginMoveNextApplicationUpgradeDomain(
            /* [in] */ IFabricApplicationUpgradeProgressResult2 *progress,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndMoveNextApplicationUpgradeDomain(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeleteApplication(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteApplication(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginUnprovisionApplicationType(
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUnprovisionApplicationType(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricApplicationManagementClient2 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginGetApplicationManifest(
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetApplicationManifest(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        HRESULT STDMETHODCALLTYPE BeginMoveNextApplicationUpgradeDomain2(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ LPCWSTR nextUpgradeDomain,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndMoveNextApplicationUpgradeDomain2(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricApplicationManagementClient3 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginUpdateApplicationUpgrade(
            /* [in] */ const FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION *upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpdateApplicationUpgrade(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginRestartDeployedCodePackage(
            /*[in]*/ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION * restartCodePackageDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartDeployedCodePackage(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRestartDeployedCodePackageInternal(
            /*[in]*/ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION * restartCodePackageDescription,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartDeployedCodePackageInternal(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE CopyApplicationPackage(
            /* [in] */ LPCWSTR imageStoreConnectionString,
            /* [in] */ LPCWSTR applicationPackagePath,
            /* [in] */ LPCWSTR applicationPackagePathInImageStore);

        HRESULT STDMETHODCALLTYPE RemoveApplicationPackage(
            /* [in] */ LPCWSTR imageStoreConnectionString,
            /* [in] */ LPCWSTR applicationPackagePathInImageStore);

        //
        // IFabricApplicationManagementClient4 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginDeployServicePackageToNode(
            /* [in] */ LPCWSTR applicationTypeName,
            /* [in] */ LPCWSTR applicationTypeVersion,
            /* [in] */ LPCWSTR serviceManifestName,
            /* [in] */ const FABRIC_PACKAGE_SHARING_POLICY_LIST * sharedPackages,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndDeployServicePackageToNode(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricApplicationManagementClient5 methods.
        //

        HRESULT STDMETHODCALLTYPE BeginRollbackApplicationUpgrade(
            /* [in] */ FABRIC_URI applicationName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndRollbackApplicationUpgrade(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricApplicationManagementClient6 methods.
        //
        HRESULT BeginUpdateApplication(
            /* [in] */ const FABRIC_APPLICATION_UPDATE_DESCRIPTION * applicationUpdateDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT EndUpdateApplication(
            /* [in] */ IFabricAsyncOperationContext * context);

        //
        // IFabricApplicationManagementClient7 methods.
        //
        HRESULT BeginDeleteApplication2(
            /* [in] */ const FABRIC_DELETE_APPLICATION_DESCRIPTION * deleteDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT EndDeleteApplication2(
            /* [in] */ IFabricAsyncOperationContext * context);

        // IFabricApplicationManagementClient8 methods.
        //
        HRESULT BeginProvisionApplicationType2(
            /* [in] */ const FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT EndProvisionApplicationType2(
            /* [in] */ IFabricAsyncOperationContext * context);

        // IFabricApplicationManagementClient9 methods.
        //

        HRESULT BeginUnprovisionApplicationType2(
            /* [in] */ const FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT EndUnprovisionApplicationType2(
            /* [in] */ IFabricAsyncOperationContext * context);

        // IFabricApplicationManagementClient10 methods.
        //
        HRESULT BeginProvisionApplicationType3(
            /* [in] */ const FABRIC_PROVISION_APPLICATION_TYPE_DESCRIPTION_BASE * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT EndProvisionApplicationType3(
            /* [in] */ IFabricAsyncOperationContext * context);

        //
        // IInternalFabricInfrastructureServiceClient
        //
        HRESULT STDMETHODCALLTYPE BeginRunCommand(
            /* [in] */ const LPCWSTR command,
            /* [in] */ FABRIC_PARTITION_ID targetPartitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRunCommand(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);

        HRESULT STDMETHODCALLTYPE BeginReportStartTaskSuccess(
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndReportStartTaskSuccess(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginReportFinishTaskSuccess(
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndReportFinishTaskSuccess(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginReportTaskFailure(
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndReportTaskFailure(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricTestClient methods.
        //
        HRESULT STDMETHODCALLTYPE GetNthNamingPartitionId(
            /* [in] */ ULONG n,
            /* [retval][out] */ FABRIC_PARTITION_ID * partitionId);

        HRESULT STDMETHODCALLTYPE BeginResolvePartition(
            /* [in] */ FABRIC_PARTITION_ID * partitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndResolvePartition(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricResolvedServicePartitionResult ** result);

        HRESULT STDMETHODCALLTYPE BeginResolveNameOwner(
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndResolveNameOwner(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricResolvedServicePartitionResult ** result);

        HRESULT STDMETHODCALLTYPE NodeIdFromNameOwnerAddress(
            /* [in] */ LPCWSTR address,
            _Out_writes_bytes_(sizeof(Federation::Nodeid)) void * federationNodeId);

        HRESULT STDMETHODCALLTYPE NodeIdFromFMAddress(
            /* [in] */ LPCWSTR address,
            _Out_writes_bytes_(sizeof(Federation::Nodeid)) void * federationNodeId);

        HRESULT STDMETHODCALLTYPE GetCurrentGatewayAddress(
            /* [retval][out] */ IFabricStringResult ** gatewayAddress);

        //
        // IFabricRepairManagementClient methods
        //
        HRESULT STDMETHODCALLTYPE BeginCreateRepairTask(
            /* [in] */ const FABRIC_REPAIR_TASK * repairTask,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndCreateRepairTask(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion);

        HRESULT STDMETHODCALLTYPE BeginCancelRepairTask(
            /* [in] */ const FABRIC_REPAIR_CANCEL_DESCRIPTION * requestDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndCancelRepairTask(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion);

        HRESULT STDMETHODCALLTYPE BeginForceApproveRepairTask(
            /* [in] */ const FABRIC_REPAIR_APPROVE_DESCRIPTION * requestDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndForceApproveRepairTask(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion);

        HRESULT STDMETHODCALLTYPE BeginDeleteRepairTask(
            /* [in] */ const FABRIC_REPAIR_DELETE_DESCRIPTION * requestDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndDeleteRepairTask(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginUpdateRepairExecutionState(
            /* [in] */ const FABRIC_REPAIR_TASK * repairTask,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndUpdateRepairExecutionState(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion);

        HRESULT STDMETHODCALLTYPE BeginGetRepairTaskList(
            /* [in] */ const FABRIC_REPAIR_TASK_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetRepairTaskList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricGetRepairTaskListResult ** result);

        //
        // IFabricRepairManagementClient2 methods
        //
        HRESULT STDMETHODCALLTYPE BeginUpdateRepairTaskHealthPolicy(
            /* [in] */ const FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION * requestDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndUpdateRepairTaskHealthPolicy(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * commitVersion);

        //
        // IFabricAccessControl methods
        //
        HRESULT STDMETHODCALLTYPE BeginSetAcl(
            /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
            /* [in] */ const FABRIC_SECURITY_ACL *acl,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndSetAcl(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE BeginGetAcl(
            /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndGetAcl(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetAclResult **acl) override;

        //
        // IImageStoreClient methods
        //
        HRESULT STDMETHODCALLTYPE Upload(
            /* [in] */ LPCWSTR sourceFullPath,
            /* [in] */ LPCWSTR destinationRelativePath,
            /* [in] */ BOOLEAN shouldOverwrite);

        HRESULT STDMETHODCALLTYPE Delete(
            /* [in] */ LPCWSTR relativePath);

        //
        // IFabricInfrastructureServiceClient methods
        //
        HRESULT STDMETHODCALLTYPE BeginInvokeInfrastructureCommand(
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ const LPCWSTR command,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndInvokeInfrastructureCommand(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricStringResult ** result);

        HRESULT STDMETHODCALLTYPE BeginInvokeInfrastructureQuery(
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ const LPCWSTR command,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndInvokeInfrastructureQuery(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricStringResult ** result);

        //
        // IFabricTestManagementClient methods
        //

        HRESULT ComFabricClient::BeginStartPartitionDataLoss(
            /* [in] */ const FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndStartPartitionDataLoss(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetPartitionDataLossProgress(
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetPartitionDataLossProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionDataLossProgressResult **result);

        // InvokeQuorumLoss
        HRESULT ComFabricClient::BeginStartPartitionQuorumLoss(
            /* [in] */ const FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndStartPartitionQuorumLoss(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetPartitionQuorumLossProgress(
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetPartitionQuorumLossProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionQuorumLossProgressResult **result);

        // RestartPartition
        HRESULT ComFabricClient::BeginStartPartitionRestart(
            /* [in] */ const FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndStartPartitionRestart(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetPartitionRestartProgress(
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetPartitionRestartProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionRestartProgressResult **result);

        HRESULT ComFabricClient::BeginGetTestCommandStatusList(
            /* [in] */ const FABRIC_TEST_COMMAND_LIST_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT ComFabricClient::EndGetTestCommandStatusList(
            /* [in] */ IFabricAsyncOperationContext *context,
              /* [retval][out] */ IFabricTestCommandStatusResult**result);

        HRESULT ComFabricClient::BeginCancelTestCommand(
           /* [in] */ const FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION * description,
           /* [in] */ DWORD timeoutMilliseconds,
           /* [in] */ IFabricAsyncOperationCallback * callback,
           /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndCancelTestCommand(
           /* [in] */ IFabricAsyncOperationContext * context);

        //
        // IFabricTestManagementClient2 methods
        //

        HRESULT STDMETHODCALLTYPE BeginStartChaos(
            /* [in] */ const FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndStartChaos(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginStopChaos(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndStopChaos(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetChaosReport(
            /* [in] */ const FABRIC_GET_CHAOS_REPORT_DESCRIPTION * getChaosReportDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndGetChaosReport(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricChaosReportResult **result);

        HRESULT ComFabricClient::BeginStartNodeTransition(
           const FABRIC_NODE_TRANSITION_DESCRIPTION * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndStartNodeTransition(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT ComFabricClient::BeginGetNodeTransitionProgress(
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT ComFabricClient::EndGetNodeTransitionProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricNodeTransitionProgressResult **result);

        //
        // IFabricTestManagementClient4 methods
        //
        HRESULT ComFabricClient::BeginGetChaos(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval, out] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndGetChaos(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval, out] */ IFabricChaosDescriptionResult ** result);

        HRESULT ComFabricClient::BeginGetChaosSchedule(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval, out] */ IFabricAsyncOperationContext ** context);
        HRESULT ComFabricClient::EndGetChaosSchedule(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval, out] */ IFabricChaosScheduleDescriptionResult ** result);

        HRESULT ComFabricClient::BeginSetChaosSchedule(
            /* [in] */ const FABRIC_CHAOS_SERVICE_SCHEDULE_DESCRIPTION* setChaosScheduleDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval, out] */ IFabricAsyncOperationContext **context);
        HRESULT ComFabricClient::EndSetChaosSchedule(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT ComFabricClient::BeginGetChaosEvents(
            /* [in] */ const FABRIC_CHAOS_EVENTS_SEGMENT_DESCRIPTION * chaosEventsDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT EndGetChaosEvents(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricChaosEventsSegmentResult **result);

        //
        // IFabricTestManagementClientInternal methods
        //
        HRESULT STDMETHODCALLTYPE BeginAddUnreliableLeaseBehavior(
            /*[in]*/ LPCWSTR nodeName,
            /*[in]*/ LPCWSTR behaviorString,
            /*[in]*/ LPCWSTR alias,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndAddUnreliableLeaseBehavior(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRemoveUnreliableLeaseBehavior(
            /*[in]*/ LPCWSTR nodeName,
            /*[in]*/ LPCWSTR alias,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRemoveUnreliableLeaseBehavior(
            /*[in]*/ IFabricAsyncOperationContext * context);

        //
        // IFabricFaultManagementClient methods
        //
        HRESULT STDMETHODCALLTYPE BeginRestartNode(
            /*[in]*/ const FABRIC_RESTART_NODE_DESCRIPTION2 * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartNode(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricRestartNodeResult **result);

        HRESULT STDMETHODCALLTYPE BeginStartNode(
            /*[in]*/ const FABRIC_START_NODE_DESCRIPTION2 * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStartNode(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricStartNodeResult **result);

        HRESULT STDMETHODCALLTYPE BeginStopNode(
            /*[in]*/ const FABRIC_STOP_NODE_DESCRIPTION2 * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStopNode(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricStopNodeResult **result);

        HRESULT STDMETHODCALLTYPE BeginStopNodeInternal(
            /*[in]*/ const FABRIC_STOP_NODE_DESCRIPTION_INTERNAL * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndStopNodeInternal(
            /*[in]*/ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginRestartDeployedCodePackage(
            /*[in]*/ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION2 * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndRestartDeployedCodePackage(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricRestartDeployedCodePackageResult **result);

        HRESULT STDMETHODCALLTYPE BeginMovePrimary(
            /*[in]*/ const FABRIC_MOVE_PRIMARY_DESCRIPTION2 * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndMovePrimary(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricMovePrimaryResult **result);

        HRESULT STDMETHODCALLTYPE BeginMoveSecondary(
            /*[in]*/ const FABRIC_MOVE_SECONDARY_DESCRIPTION2 * description,
            /*[in]*/ DWORD timeoutMilliseconds,
            /*[in]*/ IFabricAsyncOperationCallback * callback,
            /*[out, retval]*/ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndMoveSecondary(
            /*[in]*/ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricMoveSecondaryResult **result);

        //
        // Support for Docker Compose applications.
        //

        HRESULT STDMETHODCALLTYPE BeginCreateComposeDeployment(
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION *applicationDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCreateComposeDeployment(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginGetComposeDeploymentStatusList(
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndGetComposeDeploymentStatusList(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetComposeDeploymentStatusListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginDeleteComposeDeployment(
            /* [in] */ const FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteComposeDeployment(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginUpgradeComposeDeployment(
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION * upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndUpgradeComposeDeployment(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetComposeDeploymentUpgradeProgress(
            /* [in] */ LPCWSTR deploymentName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetComposeDeploymentUpgradeProgress(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricComposeDeploymentUpgradeProgressResult **result);

        HRESULT STDMETHODCALLTYPE BeginRollbackComposeDeployment(
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_ROLLBACK_DESCRIPTION *rollbackDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRollbackComposeDeployment(
            /* [in] */ IFabricAsyncOperationContext *context);

        //
        // IFabricSecretStoreClient APIs
        //

        HRESULT STDMETHODCALLTYPE BeginGetSecrets(
            /* [in] */ const FABRIC_SECRET_REFERENCE_LIST *secretReferences,
            /* [in] */ BOOLEAN includeValue,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndGetSecrets(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricSecretsResult ** result);

        HRESULT STDMETHODCALLTYPE BeginSetSecrets(
            /* [in] */ const FABRIC_SECRET_LIST *secrets,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndSetSecrets(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricSecretsResult **result);

        HRESULT STDMETHODCALLTYPE BeginRemoveSecrets(
            /* [in] */ const FABRIC_SECRET_REFERENCE_LIST *secretReferences,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRemoveSecrets(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricSecretReferencesResult **result);

        HRESULT STDMETHODCALLTYPE BeginGetSecretVersions(
            /* [in] */ const FABRIC_SECRET_REFERENCE_LIST *secretReferences,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndGetSecretVersions(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricSecretReferencesResult **result);

        // IFabricNetworkManagementClient methods
        //
        HRESULT STDMETHODCALLTYPE BeginCreateNetwork(
            /* [in] */ LPCWSTR networkName,
            /* [in] */ const FABRIC_NETWORK_DESCRIPTION * description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndCreateNetwork(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginDeleteNetwork(
            /* [in] */ const FABRIC_DELETE_NETWORK_DESCRIPTION * deleteDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndDeleteNetwork(
            /* [in] */ IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE BeginGetNetworkList(
            /* [in] */ const FABRIC_NETWORK_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetNetworkList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricGetNetworkListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginGetNetworkApplicationList(
            /* [in] */ const FABRIC_NETWORK_APPLICATION_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetNetworkApplicationList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricGetNetworkApplicationListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginGetNetworkNodeList(
            /* [in] */ const FABRIC_NETWORK_NODE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetNetworkNodeList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricGetNetworkNodeListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginGetApplicationNetworkList(
            /* [in] */ const FABRIC_APPLICATION_NETWORK_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetApplicationNetworkList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricGetApplicationNetworkListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedNetworkList(
            /* [in] */ const FABRIC_DEPLOYED_NETWORK_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetDeployedNetworkList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricGetDeployedNetworkListResult ** result);

        HRESULT STDMETHODCALLTYPE BeginGetDeployedNetworkCodePackageList(
            /* [in] */ const FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_DESCRIPTION * queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [retval][out] */ IFabricAsyncOperationContext ** context);
        HRESULT STDMETHODCALLTYPE EndGetDeployedNetworkCodePackageList(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [retval][out] */ IFabricGetDeployedNetworkCodePackageListResult ** result);

    private:
        HRESULT Initialize();

        HRESULT STDMETHODCALLTYPE BeginPutProperty(
            /* [string][in] */ FABRIC_URI name,
            /* [string][in] */ LPCWSTR propertyName,
            /* [in] */ ULONG dataLength,
            /* [in] */ BYTE const * data,
            /* [in] */ FABRIC_PROPERTY_TYPE_ID typeId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out, retval] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndPutProperty(
            /* [in] */ IFabricAsyncOperationContext *context);

        class ClientAsyncOperation;
        class CreateApplicationOperation;
        class UpdateApplicationOperation;
        class DeleteApplicationOperation;
        class ProvisionApplicationTypeOperation;
        class UnprovisionApplicationTypeOperation;
        class UpgradeApplicationOperation;
        class UpdateApplicationUpgradeOperation;
        class RollbackApplicationUpgradeOperation;
        class GetApplicationUpgradeProgressOperation;
        class GetComposeDeploymentUpgradeProgressOperation;
        class MoveNextUpgradeDomainOperation;
        class MoveNextUpgradeDomainOperation2;
        class GetApplicationManifestOperation;
        class DeployServicePackageToNodeOperation;
        class NodeStateRemovedAsyncOperation;
        class RecoverPartitionsAsyncOperation;
        class RecoverPartitionAsyncOperation;
        class RecoverServicePartitionsAsyncOperation;
        class RecoverSystemPartitionsAsyncOperation;
        class ResetPartitionLoadAsyncOperation;
        class ToggleVerboseServicePlacementHealthReportingAsyncOperation;
        class DeActivateNodeAsyncOperation;
        class ActivateNodeAsyncOperation;
        class ProvisionFabricAsyncOperation;
        class UnprovisionFabricAsyncOperation;
        class UpgradeFabricAsyncOperation;
        class UpdateFabricUpgradeAsyncOperation;
        class RollbackFabricUpgradeAsyncOperation;
        class GetFabricUpgradeProgressAsyncOperation;
        class MoveNextUpgradeDomainAsyncOperation;
        class MoveNextUpgradeDomainAsyncOperation2;
        class StartInfrastructureTaskAsyncOperation;
        class FinishInfrastructureTaskAsyncOperation;
        class GetClusterManifestAsyncOperation;
        class CreateNameAsyncOperation;
        class DeleteNameAsyncOperation;
        class NameExistsAsyncOperation;
        class EnumerateSubNamesAsyncOperation;
        class PutPropertyAsyncOperation;
        class DeletePropertyAsyncOperation;
        class GetPropertyAsyncOperation;
        class PropertyBatchAsyncOperation;
        class EnumeratePropertiesAsyncOperation;
        class PutCustomPropertyAsyncOperation;
        class QueryOperation;
        class CreateServiceGroupAsyncOperation;
        class CreateServiceGroupFromTemplateAsyncOperation;
        class DeleteServiceGroupAsyncOperation;
        class GetDescriptionAsyncOperation;
        class UpdateServiceGroupAsyncOperation;
        class CreateServiceOperation;
        class CreateServiceFromTemplateOperation;
        class UpdateServiceOperation;
        class DeleteServiceOperation;
        class GetServiceDescriptionOperation;
        class GetServiceManifestOperation;
        class ReportFaultAsyncOperation;
        class ResolveServiceOperation;
        class ReportTaskStateAsyncOperation;
        class ReportTaskStartAsyncOperation;
        class ReportTaskFinishAsyncOperation;
        class ReportTaskFailureAsyncOperation;
        class RunCommandAsyncOperation;
        class InvokeInfrastructureCommandAsyncOperation;
        class InvokeTestabilityCommandAsyncOperation;
        class InvokeDataLossAsyncOperation;
        class StartClusterConfigurationUpgradeAsyncOperation;
        class GetClusterConfigurationUpgradeStatusAsyncOperation;
        class GetClusterConfigurationAsyncOperation;
        class GetUpgradesPendingApprovalAsyncOperation;
        class StartApprovedUpgradesAsyncOperation;
        class GetUpgradeOrchestrationServiceStateAsyncOperation;
        class SetUpgradeOrchestrationServiceStateAsyncOperation;
        class GetSecretsAsyncOperation;
        class SetSecretsAsyncOperation;
        class RemoveSecretsAsyncOperation;
        class GetSecretVersionsAsyncOperation;
        class GetInvokeDataLossProgressAsyncOperation;
        class InvokeQuorumLossAsyncOperation;
        class GetInvokeQuorumLossProgressAsyncOperation;
        class RestartPartitionAsyncOperation;
        class GetRestartPartitionProgressAsyncOperation;
        class GetTestCommandStatusListOperation;
        class CancelTestCommandAsyncOperation;
        class StartNodeTransitionAsyncOperation;
        class GetNodeTransitionProgressAsyncOperation;
        class StartChaosAsyncOperation;
        class StopChaosAsyncOperation;
        class GetChaosReportAsyncOperation;
        class GetChaosEventsAsyncOperation;
        class RestartNodeAsyncOperation;
        class StartNodeAsyncOperation;
        class StopNodeAsyncOperation;
        class RestartDeployedCodePackageAsyncOperation;
        class MovePrimaryAsyncOperation;
        class MoveSecondaryAsyncOperation;
        class ResolvePartitionAsyncOperation;
        class ResolveNameOwnerAsyncOperation;
        class GetClusterHealthChunkOperation;
        class GetClusterHealthOperation;
        class GetNodeHealthOperation;
        class GetApplicationHealthOperation;
        class GetServiceHealthOperation;
        class GetPartitionHealthOperation;
        class GetReplicaHealthOperation;
        class GetDeployedApplicationHealthOperation;
        class GetDeployedServicePackageHealthOperation;
        class GetNodeHealthStateListOperation;
        class GetApplicationHealthStateListOperation;
        class GetServiceHealthStateListOperation;
        class GetPartitionHealthStateListOperation;
        class GetReplicaHealthStateListOperation;
        class GetDeployedApplicationHealthStateListOperation;
        class GetDeployedServicePackageHealthStateListOperation;
        class GetNodeListOperation;
        class GetSystemServiceListOperation;
        class GetApplicationTypeListOperation;
        class GetApplicationTypePagedListOperation;
        class GetServiceTypeListOperation;
        class TransportBehaviorListOperation;
        class GetServiceGroupMemberTypeListOperation;
        class GetApplicationListOperation;
        class GetComposeDeploymentStatusListOperation;
        class GetServiceListOperation;
        class GetServiceGroupMemberListOperation;
        class GetPartitionListOperation;
        class GetPartitionLoadInformationOperation;
        class GetReplicaListOperation;
        class GetDeployedApplicationListOperation;
        class GetDeployedApplicationPagedListOperation;
        class GetDeployedServiceManifestListOperation;
        class GetDeployedServiceTypeListOperation;
        class GetDeployedCodePackageListOperation;
        class GetDeployedReplicaListOperation;
        class SetAclOperation;
        class GetAclOperation;
        class GetDeployedServiceReplicaDetailOperation;
        class AddUnreliableTransportBehaviorOperation;
        class RemoveUnreliableTransportBehaviorOperation;
        class AddUnreliableLeaseBehaviorOperation;
        class RemoveUnreliableLeaseBehaviorOperation;
        class StopNodeOperation;
        class StartNodeOperation;
        class PromoteToPrimaryOperation;
        class MovePrimaryOperation;
        class MoveSecondaryOperation;
        class RestartDeployedCodePackageOperation;
        class CreateRepairRequestAsyncOperation;
        class CancelRepairRequestAsyncOperation;
        class ForceApproveRepairAsyncOperation;
        class DeleteRepairRequestAsyncOperation;
        class UpdateRepairExecutionStateAsyncOperation;
        class GetRepairTaskListAsyncOperation;
        class UpdateRepairTaskHealthPolicyAsyncOperation;
        class GetClusterLoadInformationOperation;
        class GetFabricCodeVersionListOperation;
        class GetFabricConfigVersionListOperation;
        class GetNodeLoadInformationOperation;
        class GetReplicaLoadInformationOperation;
        class GetUnplacedReplicaInformationOperation;
        class RegisterServiceNotificationFilterAsyncOperation;
        class UnregisterServiceNotificationFilterAsyncOperation;
        class GetApplicationLoadInformationOperation;
        class GetServiceNameOperation;
        class GetApplicationNameOperation;
        class StopNodeInternalAsyncOperation;
        class CreateComposeDeploymentOperation;
        class DeleteComposeDeploymentOperation;
        class UpgradeComposeDeploymentOperation;
        class RollbackComposeDeploymentOperation;
        class GetChaosAsyncOperation;
        class GetChaosScheduleAsyncOperation;
        class SetChaosScheduleAsyncOperation;
        class CreateNetworkOperation;
        class DeleteNetworkOperation;
        class GetNetworkListOperation;
        class GetNetworkApplicationListOperation;
        class GetNetworkNodeListOperation;
        class GetApplicationNetworkListOperation;
        class GetDeployedNetworkListOperation;
        class GetDeployedNetworkCodePackageListOperation;

        IClientFactoryPtr factoryPtr_;
        IClientSettingsPtr settingsClient_;
        IClusterManagementClientPtr clusterMgmtClient_;
        IRepairManagementClientPtr repairMgmtClient_;
        IAccessControlClientPtr accessControlClient_;
        IHealthClientPtr healthClient_;
        IPropertyManagementClientPtr propertyMgmtClient_;
        IQueryClientPtr queryClient_;
        IServiceGroupManagementClientPtr serviceGroupMgmtClient_;
        IServiceManagementClientPtr serviceMgmtClient_;
        IApplicationManagementClientPtr appMgmtClient_;
        IInfrastructureServiceClientPtr infrastructureClient_;
        ITestManagementClientPtr testManagementClient_;
        ITestManagementClientInternalPtr testManagementClientInternal_;
        IFaultManagementClientPtr faultManagementClient_;
        IInternalInfrastructureServiceClientPtr internalInfrastructureClient_;
        IImageStoreClientPtr imageStoreClient_;
        ITestClientPtr testClient_;
        IComposeManagementClientPtr composeMgmtClient_;
        ISecretStoreClientPtr secretStoreClient_;
        IResourceManagementClientPtr resourceMgmtClient_;
        INetworkManagementClientPtr networkMgmtClient_;
        IGatewayResourceManagerClientPtr gatewayResourceManagerClient_;

        std::map<LONGLONG, LocationChangeCallbackAdapterSPtr> serviceLocationChangeHandlers_;
        Common::RwLock serviceLocationChangeTrackerLock_;
   };
}
