// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ClientTest
{
    class MockFabricClientImpl
        : public Api::IServiceManagementClient
        , public Common::ComponentRoot
    {
        DENY_COPY(MockFabricClientImpl);

    public:
        MockFabricClientImpl(Common::FabricNodeConfigSPtr const &);

        //
        // IServiceMangementClient methods
        //
        Common::AsyncOperationSPtr BeginCreateService(
            ServiceModel::PartitionedServiceDescWrapper const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginInternalCreateService(
            Naming::PartitionedServiceDescriptor const &,
            ServiceModel::ServicePackageVersion const&,
            ULONGLONG packageInstance,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalCreateService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateServiceFromTemplate(
            ServiceModel::ServiceFromTemplateDescription const & serviceFromTemplateDesc,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndCreateServiceFromTemplate(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginUpdateService(
            Common::NamingUri const &,
            Naming::ServiceUpdateDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndUpdateService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeleteService(
            ServiceModel::DeleteServiceDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndDeleteService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginInternalDeleteService(
            ServiceModel::DeleteServiceDescription const &,
            Common::ActivityId const &activityId_,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalDeleteService(
            Common::AsyncOperationSPtr const &operation,
            __inout bool & nameDeleted);

        Common::AsyncOperationSPtr BeginGetServiceDescription(
            Common::NamingUri const &name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description);

        Common::AsyncOperationSPtr BeginGetCachedServiceDescription(
            Common::NamingUri const &name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetCachedServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description);

        Common::AsyncOperationSPtr BeginInternalGetServiceDescription(
            Common::NamingUri  const &name,
            Common::ActivityId const& activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalGetServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description);

        Common::AsyncOperationSPtr BeginResolveServicePartition(
            Common::NamingUri const &serviceName,
            Naming::ServiceResolutionRequestData const &resolveRequest,
            Api::IResolvedServicePartitionResultPtr &previousResult,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::AsyncOperationSPtr BeginResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::PartitionKey const &key,
            Naming::ResolvedServicePartitionMetadataSPtr const &metadata,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IResolvedServicePartitionResultPtr &result);
        Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::ResolvedServicePartitionSPtr &result);

        Common::AsyncOperationSPtr BeginResolveSystemServicePartition(
            Common::NamingUri const & serviceName,
            Naming::PartitionKey const &key,
            Common::Guid const & activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndResolveSystemServicePartition(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::ResolvedServicePartitionSPtr &result);

        Common::AsyncOperationSPtr BeginPrefixResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            Api::IResolvedServicePartitionResultPtr & previousResult,
            bool bypassCache,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndPrefixResolveServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr & result);

        Common::ErrorCode RegisterServicePartitionResolutionChangeHandler(
            Common::NamingUri const &name,
            Naming::PartitionKey const &key,
            Api::ServicePartitionResolutionChangeCallback const &handler,
            __inout LONGLONG * callbackHandle);

        Common::ErrorCode UnRegisterServicePartitionResolutionChangeHandler(
            LONGLONG callbackHandle);

        Common::AsyncOperationSPtr BeginGetServiceManifest(
            std::wstring const& applicationTypeName,
            std::wstring const& applicationTypeVersion,
            std::wstring const& serviceManifestName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetServiceManifest(
            Common::AsyncOperationSPtr const &,
            __inout std::wstring &serviceManifest);

        Common::AsyncOperationSPtr BeginReportFault(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            int64 replicaId,
            Reliability::FaultType::Enum faultType,
            bool isForce,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndReportFault(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRegisterServiceNotificationFilter(
            Reliability::ServiceNotificationFilterSPtr const & filter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterServiceNotificationFilter(
            Common::AsyncOperationSPtr const & operation,
            __out uint64 & filterId);

        Common::AsyncOperationSPtr BeginUnregisterServiceNotificationFilter(
            uint64 filterId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUnregisterServiceNotificationFilter(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginMovePrimary(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            bool ignoreConstraints,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndMovePrimary(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginMoveSecondary(
            std::wstring const & currentNodeName,
            std::wstring const & newNodeName,
            Common::Guid const & partitionId,
            bool ignoreConstraints,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndMoveSecondary(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginPrefixResolveServicePartitionInternal(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            Api::IResolvedServicePartitionResultPtr & previousResult,
            bool bypassCache,
            Common::Guid const & activityId,
            bool suppressSuccessLogs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndPrefixResolveServicePartitionInternal(
            Common::AsyncOperationSPtr const & operation,
            bool suppressSuccessLogs,
            __inout Naming::ResolvedServicePartitionSPtr & result);

    private:
        Common::FabricNodeConfigSPtr config_;
    };
}
