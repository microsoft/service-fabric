// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "MockFabricClientImpl.h"

namespace ClientTest
{
    class TestClientFactory
        : public Api::IClientFactory
        , public Common::ComponentRoot
    {
        DENY_COPY(TestClientFactory)
    public:
        TestClientFactory();
        virtual ~TestClientFactory() {};

        static Common::ErrorCode CreateLocalClientFactory(
            __in Common::FabricNodeConfigSPtr const&,
            __out Api::IClientFactoryPtr &clientFactorPtr);

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
        Common::ErrorCode CreateFileStoreServiceClient(__out Api::IFileStoreServiceClientPtr &);
        Common::ErrorCode CreateInternalFileStoreServiceClient(__out Api::IInternalFileStoreServiceClientPtr &);
        Common::ErrorCode CreateImageStoreClient(__out Api::IImageStoreClientPtr &);
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
        TestClientFactory(__in Common::FabricNodeConfigSPtr config);

        Common::FabricNodeConfigSPtr config_;
        std::shared_ptr<MockFabricClientImpl> clientSPtr_;
    };
}
