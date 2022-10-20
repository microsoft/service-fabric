// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        //
        // This class does CM specific logging and then forwards the calls to the fabric client implementation.
        //
        class FabricClientProxy : public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
            DENY_COPY(FabricClientProxy);

        public:
            __declspec(property(get=get_ClientFactory)) Api::IClientFactoryPtr const & Factory;
            Api::IClientFactoryPtr const & get_ClientFactory() const { return clientFactory_; }

        public:
            FabricClientProxy(Api::IClientFactoryPtr const &, Store::PartitionedReplicaId const &);

            Common::ErrorCode Initialize();

            Common::AsyncOperationSPtr BeginCreateName(
                Common::NamingUri const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndCreateName(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginDeleteName(
                Common::NamingUri const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndDeleteName(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginSubmitPropertyBatch(
                Naming::NamePropertyOperationBatch &&,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndSubmitPropertyBatch(
                Common::AsyncOperationSPtr const &,
                __inout Api::IPropertyBatchResultPtr &) const;

            Common::AsyncOperationSPtr BeginCreateService(
                Naming::PartitionedServiceDescriptor const &,
                ServiceModelVersion const & packageVersion,
                uint64 packageInstance,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginUpdateService(
                Common::NamingUri const &,
                Naming::ServiceUpdateDescription const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndUpdateService(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginDeleteService(
                Common::NamingUri const & appName,
                ServiceModel::DeleteServiceDescription const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndDeleteService(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginGetServiceDescription(
                Common::NamingUri const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndGetServiceDescription(
                Common::AsyncOperationSPtr const &,
                __out Naming::PartitionedServiceDescriptor &) const;

            Common::AsyncOperationSPtr BeginGetProperty(
                Common::NamingUri const &,
                std::wstring const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndGetProperty(
                Common::AsyncOperationSPtr const &,
                __inout Naming::NamePropertyResult &) const;

            Common::AsyncOperationSPtr BeginPutProperty(
                Common::NamingUri const &,
                std::wstring const &,
                std::vector<byte> const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndPutProperty(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginDeleteProperty(
                Common::NamingUri const &,
                std::wstring const &,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndDeleteProperty(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginDeactivateNodesBatch(
                std::map<Federation::NodeId, Reliability::NodeDeactivationIntent::Enum> const &,
                std::wstring const & batchId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndDeactivateNodesBatch(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginRemoveNodeDeactivations(
                std::wstring const & batchId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndRemoveNodeDeactivations(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginGetNodeDeactivationStatus(
                std::wstring const & batchId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndGetNodeDeactivationStatus(
                Common::AsyncOperationSPtr const &,
                __out Reliability::NodeDeactivationStatus::Enum &) const;

            Common::AsyncOperationSPtr BeginNodeStateRemoved(
                std::wstring const & nodeName,
                Federation::NodeId const & nodeId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndNodeStateRemoved(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginReportStartTaskSuccess(
                TaskInstance const & taskInstance,
                Common::Guid const & targetPartitionId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndReportStartTaskSuccess(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginReportFinishTaskSuccess(
                TaskInstance const & taskInstance,
                Common::Guid const & targetPartitionId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndReportFinishTaskSuccess(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginReportTaskFailure(
                TaskInstance const & taskInstance,
                Common::Guid const & targetPartitionId,
                Common::ActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndReportTaskFailure(Common::AsyncOperationSPtr const &) const;

            Common::AsyncOperationSPtr BeginUnprovisionApplicationType(
                Management::ClusterManager::UnprovisionApplicationTypeDescription const &,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const &) const;
            Common::ErrorCode EndUnprovisionApplicationType(
                Common::AsyncOperationSPtr const &) const;

        private:
            class ServiceAsyncOperationBase;
            class CreateServiceAsyncOperation;
            class DeleteServiceAsyncOperation;

            Api::IClientFactoryPtr clientFactory_;
            Api::IInternalInfrastructureServiceClientPtr infrastructureClient_;
            Api::IServiceManagementClientPtr serviceMgmtClient_;
            Api::IPropertyManagementClientPtr propertyMgmtClient_;
            Api::IClusterManagementClientPtr clusterMgmtClient_;
            Api::IApplicationManagementClientPtr applicationManagementClient_;
        };
    }
}
