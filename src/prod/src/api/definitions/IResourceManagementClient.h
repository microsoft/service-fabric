//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Api
{
    class IResourceManagementClient
    {
    public:
        virtual ~IResourceManagementClient() {};

        //
        // Volume resource
        //
        virtual Common::AsyncOperationSPtr BeginCreateVolume(
            Management::ClusterManager::VolumeDescriptionSPtr const & volumeDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateVolume(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetVolumeResourceList(
            ServiceModel::ModelV2::VolumeQueryDescription const & volumeQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetVolumeResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<ServiceModel::ModelV2::VolumeQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteVolume(
            std::wstring const & volumeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteVolume(
            Common::AsyncOperationSPtr const &) = 0;

        //
        // Application resource
        //
        virtual Common::AsyncOperationSPtr BeginCreateOrUpdateApplicationResource(
            ServiceModel::ModelV2::ApplicationDescription && description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateOrUpdateApplicationResource(
            Common::AsyncOperationSPtr const &,
            __out ServiceModel::ModelV2::ApplicationDescription & description) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationResourceList(
            ServiceModel::ApplicationQueryDescription && applicationQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetApplicationResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<ServiceModel::ModelV2::ApplicationDescriptionQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteSingleInstanceDeployment(
            ServiceModel::DeleteSingleInstanceDeploymentDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteSingleInstanceDeployment(
            Common::AsyncOperationSPtr const &) = 0;

        //
        // Service
        //
        virtual Common::AsyncOperationSPtr BeginGetServiceResourceList(
            ServiceModel::ServiceQueryDescription const & serviceQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetServiceResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<ServiceModel::ModelV2::ContainerServiceQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) = 0;

        //
        // Replica
        // 
        virtual Common::AsyncOperationSPtr BeginGetReplicaResourceList(
            ServiceModel::ReplicasResourceQueryDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetReplicaResourceList(
            Common::AsyncOperationSPtr const & operation,
            std::vector<ServiceModel::ReplicaResourceQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) = 0;

        virtual Common::AsyncOperationSPtr BeginGetContainerCodePackageLogs(
            Common::NamingUri const & applicationName,
            Common::NamingUri const & serviceName,
            std::wstring replicaName,
            std::wstring const & codePackageName,
            ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetContainerCodePackageLogs(
            Common::AsyncOperationSPtr const & operation,
            __inout wstring &containerInfo) = 0;

    };
}
