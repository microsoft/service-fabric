// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceEventSource
        {
        public:

            Common::TraceEventWriter<std::wstring, uint64, std::wstring> ServiceCreated;
            Common::TraceEventWriter<std::wstring, ServiceInfo, int64, int64> ServiceAdded;
            Common::TraceEventWriter<std::wstring> ServiceToBeDeleted;
            Common::TraceEventWriter<std::wstring, uint64> ServiceDeleted;
            Common::TraceEventWriter<Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring, uint64, bool, int, int, int, std::wstring, Common::Guid> ServiceCreatedOperational;
            Common::TraceEventWriter<Common::Guid, std::wstring, std::wstring, std::wstring, std::wstring, uint64, bool, int, int, int, std::wstring> ServiceDeletedOperational;
            
            Common::TraceEventWriter<std::wstring, ServiceInfo, int64> ServiceRemoved;
            Common::TraceEventWriter<std::wstring, ServiceInfo, int64, int64> ServiceUpdate;
            Common::TraceEventWriter<std::wstring, ServiceDescription> CreateService_;
            Common::TraceEventWriter<std::wstring, ServiceDescription> CreateServiceDuringRebuild;
            Common::TraceEventWriter<std::wstring, ServiceDescription, size_t, size_t> UpdateService;
            Common::TraceEventWriter<std::wstring, Naming::ServiceUpdateDescription> UpdateSystemService;
            Common::TraceEventWriter<std::wstring, ServiceInfo> ServiceState;
            Common::TraceEventWriter<std::wstring, Common::ErrorCode, int64, int64> ServiceUpdateFailure;

            Common::TraceEventWriter<std::wstring, ServiceInfo, int64> UpdatedHealthSequence;
            Common::TraceEventWriter<std::wstring, ServiceInfo, Common::ErrorCode, int64> UpdateHealthSequenceFailed;
            Common::TraceEventWriter<std::wstring, ServiceModel::ApplicationIdentifier, Common::ErrorCode, int64, int64> AppUpdateFailure;
            
            Common::TraceEventWriter<ServiceCounts> ServiceCounts;

            Common::TraceEventWriter<ServiceTableUpdateMessageBody> LookupTableBroadcast;
            Common::TraceEventWriter<Common::VersionRangeCollection, Common::VersionRangeCollection> LookupTableBroadcastRanges;

            ServiceEventSource(Common::TraceTaskCodes::Enum id) :

                ServiceCreated(id, 221, "_Services_ServiceCreated", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Service {0} inst {1} created with partition {2}", "id", "dca_instance", "partitionId"),
                ServiceAdded(id, 222, "Added_ServiceUpdate", Common::LogLevel::Info, "Service added: {1}\r\nCommit Duration = {2} ms, PLB Duration {3} ms", "id", "serviceInfo", "commitDuration", "plbDuration"),
                ServiceToBeDeleted(id, 223, "ServiceToBeDeleted", Common::LogLevel::Info, "Service marked as ToBeDeleted: {0}", "id"),
                ServiceDeleted(id, 224, "_Services_DEL_ServiceDeleted", Common::LogLevel::Info, Common::TraceChannelType::Debug, TRACE_KEYWORDS2(Default, ForQuery), "Service {0} instance {1} deleted", "id", "dca_instance"),

                // Only FM emits these as Operational traces
                ServiceCreatedOperational(Common::ExtendedEventMetadata::Create(L"ServiceCreated", OperationalLifeCycleCategory), id, 225, "_ServicesOps_ServiceCreatedOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, "Service Created: Service {1} partition {11} of ServiceType {2} created in Application {3} ApplicationType {4}.", "eventInstanceId", "serviceName", "serviceTypeName", "applicationName", "applicationTypeName", "serviceInstance", "isStateful", "partitionCount", "targetReplicaSetSize", "minReplicaSetSize", "servicePackageVersion", "partitionId"),
                ServiceDeletedOperational(Common::ExtendedEventMetadata::Create(L"ServiceDeleted", OperationalLifeCycleCategory), id, 226, "_ServicesOps_ServiceDeletedOperational", Common::LogLevel::Info, id == Common::TraceTaskCodes::FM ? Common::TraceChannelType::Operational : Common::TraceChannelType::Debug, "Service Deleted: Service {1} of ServiceType {2} deleted in Application {3} ApplicationType {4}.", "eventInstanceId", "serviceName", "serviceTypeName", "applicationName", "applicationTypeName", "serviceInstance", "isStateful", "partitionCount", "targetReplicaSetSize", "minReplicaSetSize", "servicePackageVersion"),

                ServiceRemoved(id, 227, "Removed_ServiceUpdate", Common::LogLevel::Info, "Service removed: {1}\r\nCommit Duration = {2} ms", "id", "serviceInfo", "commitDuration"),
                ServiceUpdate(id, 228, "ServiceUpdate", Common::LogLevel::Info, "Service updated: {1}\r\nCommit Duration = {2} ms, PLB Duration = {3} ms", "id", "serviceInfo", "commitDuration", "plbDuration"),
                CreateService_(id, 229, "CreateService_Service", Common::LogLevel::Noise, "CreateService for {1}", "id", "serviceDescription"),
                CreateServiceDuringRebuild(id, 230, "CreateServiceDuringRebuild_Service", Common::LogLevel::Info, "CreateServiceDuringRebuild for {1}", "id", "serviceDescription"),
                UpdateService(id, 231, "UpdateService_Service", Common::LogLevel::Info, "UpdateService for {1}, Added={2}, Removed={3}", "id", "serviceDescription", "added", "removed"),
                UpdateSystemService(id, 232, "UpdateSystemService_Service", Common::LogLevel::Info, "UpdateService for {1}", "id", "serviceUpdateDescription"),
                ServiceState(id, 233, "ServiceState", Common::LogLevel::Info, "{1}", "id", "serviceInfo"),
                ServiceUpdateFailure(id, 234, "ServiceUpdateFailure_Service", Common::LogLevel::Warning, "Update of service {0} failed with {1}. Commit Duration = {2} ms, PLB Duration = {3} ms", "id", "error", "commitDuration", "plbDuration"),
                
                UpdatedHealthSequence(id, 235, "UpdatedHealthSequence_Service", Common::LogLevel::Info, "Updated health sequence for {1}\r\nCommit Duration = {2} ms", "id", "serviceInfo", "commitDuration"),
                UpdateHealthSequenceFailed(id, 236, "UpdateHealthSequenceFailed_Service", Common::LogLevel::Warning, "Update health sequence failed with {2}: {1}\r\nCommit Duration = {3} ms", "id", "serviceInfo", "error", "commitDuration"),
                AppUpdateFailure(id, 237, "AppUpdateFailure", Common::LogLevel::Warning, "Update of application {1} failed with {2}\r\nCommit Duration = {3} ms, PLB Duration = {4} ms", "id", "applicationId", "error", "commitDuration", "plbDuration"),
                
                ServiceCounts(id, 238, "ServiceCounts", Common::LogLevel::Info, "{0}", "serviceCounts"),

                LookupTableBroadcast(id, 239, "Broadcast_LookupTable", Common::LogLevel::Info, "Broadcasting ServiceTableUpdate: {0}", "broadcastBody"),
                LookupTableBroadcastRanges(id, 240, "Ranges_LookupTable", Common::LogLevel::Info, "Current VersionRangeCollection: {0}, broadcast excluding ranges: {1}", "versionRangeCollection", "excludingRanges")

                // Event id limit: 255
            {
            }

        };
    }
}
