// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class IClientFactory
    {
    public:
        virtual ~IClientFactory() {};

        virtual Common::ErrorCode CreateQueryClient(__out IQueryClientPtr &) = 0;

        virtual Common::ErrorCode CreateSettingsClient(__out IClientSettingsPtr &) = 0;

        virtual Common::ErrorCode CreateServiceManagementClient(__out IServiceManagementClientPtr& ) = 0;

        virtual Common::ErrorCode CreateApplicationManagementClient(__out IApplicationManagementClientPtr &) = 0;

        virtual Common::ErrorCode CreateClusterManagementClient(__out IClusterManagementClientPtr& ) = 0;

        virtual Common::ErrorCode CreateAccessControlClient(_Out_ IAccessControlClientPtr &) = 0;

        virtual Common::ErrorCode CreateHealthClient(__out IHealthClientPtr &) = 0;

        virtual Common::ErrorCode CreatePropertyManagementClient(__out IPropertyManagementClientPtr &) = 0;

        virtual Common::ErrorCode CreateServiceGroupManagementClient(__out IServiceGroupManagementClientPtr &) = 0;

        virtual Common::ErrorCode CreateInfrastructureServiceClient(__out IInfrastructureServiceClientPtr &) = 0;

        virtual Common::ErrorCode CreateInternalInfrastructureServiceClient(__out IInternalInfrastructureServiceClientPtr &) = 0;

        virtual Common::ErrorCode CreateTestManagementClient(__out ITestManagementClientPtr &) = 0;

        virtual Common::ErrorCode CreateTestManagementClientInternal(__out ITestManagementClientInternalPtr &) = 0;

        virtual Common::ErrorCode CreateFaultManagementClient(__out IFaultManagementClientPtr &) = 0;

        virtual Common::ErrorCode CreateImageStoreClient(__out IImageStoreClientPtr &) = 0;

        virtual Common::ErrorCode CreateFileStoreServiceClient(__out IFileStoreServiceClientPtr &) = 0;

        virtual Common::ErrorCode CreateInternalFileStoreServiceClient(__out IInternalFileStoreServiceClientPtr &) = 0;

        virtual Common::ErrorCode CreateTestClient(__out ITestClientPtr &) = 0;

        virtual Common::ErrorCode CreateInternalTokenValidationServiceClient(__out IInternalTokenValidationServiceClientPtr &) = 0;

        virtual Common::ErrorCode CreateRepairManagementClient(__out IRepairManagementClientPtr& ) = 0;

        virtual Common::ErrorCode CreateComposeManagementClient(__out IComposeManagementClientPtr& ) = 0;
    };
}
