// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class FabricClientWrapper
        : public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(FabricClientWrapper)

    public:

        static Common::ErrorCode CreateFabricClient(
            __in Common::FabricNodeConfigSPtr &config,
            __out FabricClientWrapperSPtr &,
            __in Transport::RoleMask::Enum role = Transport::RoleMask::None);

        Common::ErrorCode SetSecurity(Transport::SecuritySettings &&);

        __declspec(property(get=get_NativeImageStoreClient)) Api::INativeImageStoreClientPtr & NativeImageStoreClient;
        __declspec(property(get=get_AppMgmtClient)) Api::IApplicationManagementClientPtr &AppMgmtClient;
        __declspec(property(get=get_ComposeAppMgmtClient)) Api::IComposeManagementClientPtr &ComposeAppMgmtClient;
        __declspec(property(get=get_ResourceMgmtClient)) Api::IResourceManagementClientPtr &ResourceMgmtClient;
        __declspec(property(get=get_QueryClient)) Api::IQueryClientPtr &QueryClient;
        __declspec(property(get=get_ServiceMgmtClient)) Api::IServiceManagementClientPtr &ServiceMgmtClient;
        __declspec(property(get=get_ServiceGroupMgmtClient)) Api::IServiceGroupManagementClientPtr &ServiceGroupMgmtClient;
        __declspec(property(get=get_HealthClient)) Api::IHealthClientPtr &HealthClient;
        __declspec(property(get=get_ClusterMgmtClient)) Api::IClusterManagementClientPtr &ClusterMgmtClient;
        __declspec(property(get=get_FaultMgmtClient)) Api::IFaultManagementClientPtr &FaultMgmtClient;
        __declspec(property(get = get_SecretStoreClient)) Api::ISecretStoreClientPtr &SecretStoreClient;
        __declspec(property(get=get_TvsMgmtClient)) Api::IInternalTokenValidationServiceClientPtr &TvsClient;
        __declspec(property(get=get_InfrastructureClient)) Api::IInfrastructureServiceClientPtr &InfrastructureClient;
        __declspec(property(get=get_RepairMgmtClient)) Api::IRepairManagementClientPtr &RepairMgmtClient;
        __declspec(property(get=get_TestMgmtClient)) Api::ITestManagementClientPtr &TestMgmtClient;
        __declspec(property(get=get_PropertyMgmtClient)) Api::IPropertyManagementClientPtr &PropertyMgmtClient;
        __declspec(property(get = get_NetworkMgmtClient)) Api::INetworkManagementClientPtr &NetworkMgmtClient;
        __declspec(property(get = get_GatewayResourceManagerClient)) Api::IGatewayResourceManagerClientPtr &GatewayResourceManagerClient;
        __declspec(property(get = get_ClientRole)) Transport::RoleMask::Enum ClientRole;


        Api::INativeImageStoreClientPtr const & get_NativeImageStoreClient() const { return nativeImageStoreClientPtr_; }
        Api::IApplicationManagementClientPtr const& get_AppMgmtClient() const{ return appMgmtClientPtr_; }
        Api::IComposeManagementClientPtr const & get_ComposeAppMgmtClient() const { return composeAppMgmtClientPtr_; }
        Api::IResourceManagementClientPtr const & get_ResourceMgmtClient() const { return resourceMgmtClientPtr_; }
        Api::IQueryClientPtr const& get_QueryClient() const{ return queryClientPtr_; }
        Api::IServiceManagementClientPtr const& get_ServiceMgmtClient() const{ return serviceMgmtClientPtr_; }
        Api::IServiceGroupManagementClientPtr const& get_ServiceGroupMgmtClient() const{ return serviceGroupMgmtClientPtr_; }
        Api::IHealthClientPtr const& get_HealthClient() const{ return healthClientPtr_; }
        Api::IClusterManagementClientPtr const& get_ClusterMgmtClient() const{ return clusterMgmtClientPtr_; }
        Api::IFaultManagementClientPtr const& get_FaultMgmtClient() const { return faultMgmtClientPtr_; }
        Api::ISecretStoreClientPtr const& get_SecretStoreClient() const { return secretStoreClientPtr_; }
        Api::IInternalTokenValidationServiceClientPtr const& get_TvsMgmtClient() const{ return tvsClientPtr_; }
        Api::IInfrastructureServiceClientPtr const& get_InfrastructureClient() const{ return infraSvcClientPtr_; }
        Api::IRepairManagementClientPtr const& get_RepairMgmtClient() const{ return repairMgmtClientPtr_; };
        Api::ITestManagementClientPtr const& get_TestMgmtClient() const{ return testMgmtClientPtr_; };
        Api::IPropertyManagementClientPtr const& get_PropertyMgmtClient() const{ return propertyMgmtClientPtr_; };
        Api::INetworkManagementClientPtr const& get_NetworkMgmtClient() const { return networkMgmtClientPtr_; };
        Api::IGatewayResourceManagerClientPtr const& get_GatewayResourceManagerClient() const { return gatewayResourceManagerClientPtr_; };
        Transport::RoleMask::Enum get_ClientRole() { return role_; }

    private:

        FabricClientWrapper(__in Transport::RoleMask::Enum role)
        : role_(role)
        {
        }

        Common::ErrorCode Initialize(__in Common::FabricNodeConfigSPtr &);

        Api::INativeImageStoreClientPtr nativeImageStoreClientPtr_;
        Api::IApplicationManagementClientPtr appMgmtClientPtr_;
        Api::IComposeManagementClientPtr composeAppMgmtClientPtr_;
        Api::IResourceManagementClientPtr resourceMgmtClientPtr_;
        Api::IQueryClientPtr queryClientPtr_;
        Api::IServiceManagementClientPtr serviceMgmtClientPtr_;
        Api::IServiceGroupManagementClientPtr serviceGroupMgmtClientPtr_;
        Api::IHealthClientPtr healthClientPtr_;
        Api::IClusterManagementClientPtr clusterMgmtClientPtr_;
        Api::IFaultManagementClientPtr faultMgmtClientPtr_;
        Api::ISecretStoreClientPtr secretStoreClientPtr_;
        Api::IInternalTokenValidationServiceClientPtr tvsClientPtr_;
        Api::IInfrastructureServiceClientPtr infraSvcClientPtr_;
        Api::IRepairManagementClientPtr repairMgmtClientPtr_;
        Api::ITestManagementClientPtr testMgmtClientPtr_;
        Api::IClientSettingsPtr settingsClientPtr_;
        Api::IPropertyManagementClientPtr propertyMgmtClientPtr_;
        Api::INetworkManagementClientPtr networkMgmtClientPtr_;

        Api::IGatewayResourceManagerClientPtr gatewayResourceManagerClientPtr_;

        Transport::RoleMask::Enum role_;
    };
}
