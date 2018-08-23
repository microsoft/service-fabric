// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    typedef std::function<void(LONGLONG handlerId, IResolvedServicePartitionResultPtr const& result, Common::ErrorCode error)> ServicePartitionResolutionChangeCallback;

    class IServiceManagementClient
    {
    public:
        virtual ~IServiceManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginCreateService(
            ServiceModel::PartitionedServiceDescWrapper const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalCreateService(
            Naming::PartitionedServiceDescriptor const &,
            ServiceModel::ServicePackageVersion const&,
            ULONGLONG packageInstance,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndInternalCreateService(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateService(
            Common::NamingUri const & name,
            Naming::ServiceUpdateDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUpdateService(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCreateServiceFromTemplate(
            ServiceModel::ServiceFromTemplateDescription const & svcFromTemplateDesc,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateServiceFromTemplate(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteService(
            ServiceModel::DeleteServiceDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteService(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalDeleteService(
            ServiceModel::DeleteServiceDescription const & description,
            Common::ActivityId const& activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndInternalDeleteService(
            Common::AsyncOperationSPtr const &,
            __inout bool & nameDeleted) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceDescription(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetServiceDescription(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::PartitionedServiceDescriptor & description) = 0;

        virtual Common::AsyncOperationSPtr BeginGetCachedServiceDescription(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetCachedServiceDescription(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::PartitionedServiceDescriptor & description) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalGetServiceDescription(
            Common::NamingUri  const &name,
            Common::ActivityId const& activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndInternalGetServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description) = 0;

        virtual Common::AsyncOperationSPtr BeginResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            IResolvedServicePartitionResultPtr & previousResult,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::AsyncOperationSPtr BeginResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::PartitionKey const &key,
            std::shared_ptr<Naming::ResolvedServicePartitionMetadata> const &metadata,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout IResolvedServicePartitionResultPtr & result) = 0;
        virtual Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr & result) = 0;

        virtual Common::AsyncOperationSPtr BeginResolveSystemServicePartition(
            Common::NamingUri const & serviceName,
            Naming::PartitionKey const &key,
            Common::Guid const & activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndResolveSystemServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr & result) = 0;

        virtual Common::AsyncOperationSPtr BeginPrefixResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            IResolvedServicePartitionResultPtr & previousResult,
            bool bypassCache,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndPrefixResolveServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr & result) = 0;

        virtual Common::ErrorCode RegisterServicePartitionResolutionChangeHandler(
            Common::NamingUri const& name,
            Naming::PartitionKey const& key,
            ServicePartitionResolutionChangeCallback const &handler,
            __inout LONGLONG * callbackHandle) = 0;
        virtual Common::ErrorCode UnRegisterServicePartitionResolutionChangeHandler(
            LONGLONG callbackHandle) = 0;

         virtual Common::AsyncOperationSPtr BeginGetServiceManifest(
            std::wstring const& applicationTypeName,
            std::wstring const& applicationTypeVersion,
            std::wstring const& serviceManifestName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetServiceManifest(
            Common::AsyncOperationSPtr const &,
            __inout std::wstring &serviceManifest) = 0;

        virtual Common::AsyncOperationSPtr BeginReportFault(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            int64 replicaId,
            Reliability::FaultType::Enum faultType,
            bool isForce,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndReportFault(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRegisterServiceNotificationFilter(
            Reliability::ServiceNotificationFilterSPtr const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRegisterServiceNotificationFilter(
            Common::AsyncOperationSPtr const &,
            __out uint64 & filterId) = 0;

        virtual Common::AsyncOperationSPtr BeginUnregisterServiceNotificationFilter(
            uint64 filterId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUnregisterServiceNotificationFilter(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginMovePrimary(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            bool ignoreConstraints,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndMovePrimary(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginMoveSecondary(
            std::wstring const & currentNodeName,
            std::wstring const & newNodeName,
            Common::Guid const & partitionId,
            bool ignoreConstraints,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndMoveSecondary(
            Common::AsyncOperationSPtr const &) = 0;

        //
        // Internal methods, not exposed through COM/public API
        //
        virtual Common::AsyncOperationSPtr BeginPrefixResolveServicePartitionInternal(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            IResolvedServicePartitionResultPtr & previousResult,
            bool bypassCache,
            Common::Guid const & activityId,
            bool suppressSuccessLogs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndPrefixResolveServicePartitionInternal(
            Common::AsyncOperationSPtr const & operation,
            bool suppressSuccessLogs,
            __inout Naming::ResolvedServicePartitionSPtr & result) = 0;
    };
}
