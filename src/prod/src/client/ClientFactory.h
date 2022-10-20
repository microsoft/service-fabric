// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    //
    // This is a helper class for the creation of fabric client. This class maintains the
    // number of external 'uses' of the fabric client, and calls close on the client when the
    // last reference goes away.
    //
    class ClientFactory
        : public Api::IClientFactory
        , public Common::ComponentRoot
    {
    public:
        ClientFactory();
        virtual ~ClientFactory();

        static  Common::ErrorCode CreateClientFactory(
            USHORT connectionStringSize,
            __deref_in_ecount(connectionStringSize) LPCWSTR const *connectionStrings,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static  Common::ErrorCode CreateClientFactory(
            USHORT connectionStringSize,
            __deref_in_ecount(connectionStringSize) LPCWSTR const *connectionStrings,
            Api::IServiceNotificationEventHandlerPtr const &,
            Api::IClientConnectionEventHandlerPtr const &,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static Common::ErrorCode CreateClientFactory(
            std::vector<std::wstring> &&connectionStringList,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static Common::ErrorCode CreateClientFactory(
            FabricClientImplSPtr const & fabricClientImpl,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static Common::ErrorCode CreateLocalClientFactory(
            Common::FabricNodeConfigSPtr const&,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static Common::ErrorCode CreateLocalClientFactory(
            Common::FabricNodeConfigSPtr const&,
            Transport::RoleMask::Enum clientRole,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static Common::ErrorCode CreateLocalClientFactory(
            Common::FabricNodeConfigSPtr const&,
            Transport::RoleMask::Enum clientRole,
            Api::IServiceNotificationEventHandlerPtr const &,
            Api::IClientConnectionEventHandlerPtr const &,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static Common::ErrorCode CreateLocalClientFactory(
            Api::IServiceNotificationEventHandlerPtr const &,
            Naming::INamingMessageProcessorSPtr const &,
            __out Api::IClientFactoryPtr &clientFactorPtr);


        // This method is used by Http gateway to create fabric client where it has already authenticated
        // the incoming connection and wants to communicate with TCP gateway without requiring the client
        // to be authenticated again.
        static Common::ErrorCode CreateLocalClientFactory(
            Common::FabricNodeConfigSPtr const&,
            Transport::RoleMask::Enum clientRole,
            bool isClientRoleAuthorized,
            __out Api::IClientFactoryPtr &clientFactorPtr);

        static ServiceModel::FabricClientSettings GetFabricClientDefaultSettings();

        Common::ErrorCode CreateQueryClient(__out Api::IQueryClientPtr &);
        Common::ErrorCode CreateSettingsClient(__out Api::IClientSettingsPtr &);
        Common::ErrorCode CreateServiceManagementClient(__out Api::IServiceManagementClientPtr &);
        Common::ErrorCode CreateApplicationManagementClient(__out Api::IApplicationManagementClientPtr &);
        Common::ErrorCode CreateClusterManagementClient(__out Api::IClusterManagementClientPtr& );
        Common::ErrorCode CreateAccessControlClient(_Out_ Api::IAccessControlClientPtr &) override;
        Common::ErrorCode CreateHealthClient(__out Api::IHealthClientPtr &);
        Common::ErrorCode CreatePropertyManagementClient(__out Api::IPropertyManagementClientPtr &);
        Common::ErrorCode CreateServiceGroupManagementClient(__out Api::IServiceGroupManagementClientPtr &);
        Common::ErrorCode CreateInfrastructureServiceClient(__out Api::IInfrastructureServiceClientPtr &);
        Common::ErrorCode CreateInternalInfrastructureServiceClient(__out Api::IInternalInfrastructureServiceClientPtr &);
        Common::ErrorCode CreateTestManagementClient(__out Api::ITestManagementClientPtr &);
        Common::ErrorCode CreateTestManagementClientInternal(__out Api::ITestManagementClientInternalPtr &);
        Common::ErrorCode CreateFaultManagementClient(__out Api::IFaultManagementClientPtr &);
        Common::ErrorCode CreateImageStoreClient(__out Api::IImageStoreClientPtr &);
        Common::ErrorCode CreateFileStoreServiceClient(__out Api::IFileStoreServiceClientPtr &);
        Common::ErrorCode CreateInternalFileStoreServiceClient(__out Api::IInternalFileStoreServiceClientPtr &);
        Common::ErrorCode CreateTestClient(__out Api::ITestClientPtr &);
        Common::ErrorCode CreateInternalTokenValidationServiceClient(__out Api::IInternalTokenValidationServiceClientPtr &);
        Common::ErrorCode CreateRepairManagementClient(__out Api::IRepairManagementClientPtr &);
        Common::ErrorCode CreateComposeManagementClient(__out Api::IComposeManagementClientPtr &);
        Common::ErrorCode CreateSecretStoreClient(__out Api::ISecretStoreClientPtr &);
        Common::ErrorCode CreateResourceManagerClient(__out Api::IResourceManagerClientPtr &);
        Common::ErrorCode CreateResourceManagementClient(__out Api::IResourceManagementClientPtr &);
        Common::ErrorCode CreateNetworkManagementClient(__out Api::INetworkManagementClientPtr &);
        Common::ErrorCode CreateGatewayResourceManagerClient(__out Api::IGatewayResourceManagerClientPtr &);

    private:

        ClientFactory(FabricClientImplSPtr const & fabricClientImpl);

        static Common::ErrorCode ParseConnectionStringsParameter(
            USHORT connectionStringsSize,
            LPCWSTR const *connectionStrings,
            __inout std::vector<std::wstring> & connectionStringList);

        Common::ErrorCode ValidateFabricClient();

        FabricClientImplSPtr fabricClientSPtr_;
        Common::ExclusiveLock lock_;
    };
}
