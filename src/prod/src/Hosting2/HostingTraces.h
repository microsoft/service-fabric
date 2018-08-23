// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
#define HOSTING_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, Hosting, trace_id, trace_level, __VA_ARGS__)

#define HOSTING_STRUCTURED_QUERY_TRACE(tableName, trace_name, trace_id, trace_level, ...) \
    STRUCTURED_QUERY_TRACE(tableName, trace_name, Hosting, trace_id, trace_level, __VA_ARGS__)

#define HOSTING_OPERATIONAL_TRACE(table_prefix, trace_name, trace_id, trace_level, ...) \
    STRUCTURED_OPERATIONAL_TRACE(table_prefix, trace_name, Hosting, trace_id, trace_level, __VA_ARGS__)

    class HostingTraces
    {
    public:
        DECLARE_STRUCTURED_TRACE(ApplicationPackageDownloaded, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServicePackageDownloaded, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ApplicationActivated, std::wstring, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(ApplicationDeactivated, std::wstring, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(ServicePackageActivated, std::wstring, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(ServicePackageDeactivated, std::wstring, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(ServiceTypeRegistered, std::wstring, uint64, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(FabricRuntimeClosed, std::wstring, uint64, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ApplicationHostClosed, std::wstring, uint64, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(AvailableContainerImages, std::wstring, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(ServiceTypeDisabled, std::wstring, uint64, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServiceTypeEnabled, std::wstring, uint64, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(NodeVersionUpgraded, std::wstring, std::wstring, Common::FabricVersionInstance, Common::FabricVersionInstance);
        DECLARE_STRUCTURED_TRACE(NodeVersionUpgradeFailed, std::wstring, std::wstring, Common::FabricVersionInstance, Common::FabricVersionInstance, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(HostedServiceActivated, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(HostedServiceActivationFailed, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(HostedServiceDeactivated, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(HostedServiceDeactivationFailed, std::wstring, uint64, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(HostedServiceTerminated, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(HostedServiceUnexpectedTermination, std::wstring, std::wstring, int64, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServicePackageActivatedNotifyDca, int32, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServicePackageDeactivatedNotifyDca, int32, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ServicePackageUpgradedNotifyDca, int32, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(HostedServiceActivationLimitExceeded, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ApplicationInstanceFolderDeletionSuccess, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ApplicationInstanceFolderDeletionFailed, std::wstring, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(ApplicationTypeContentDeletionSuccess, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ApplicationTypeContentDeletionFailed, std::wstring, std::wstring, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(ProcessUnexpectedTermination, std::wstring, std::wstring, int64, std::wstring, int64, Common::DateTime);
        DECLARE_STRUCTURED_TRACE(ContainerActivatedOperational, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ContainerDeactivatedOperational, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ActiveExecutablesAndContainersStats, int64, int64);
        DECLARE_STRUCTURED_TRACE(ContainerTerminated, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ContainerActivated, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ContainerDeactivated, std::wstring, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_APPLICATIONS_OPERATIONAL_TRACE(ContainerExitedOperational, std::wstring, std::wstring, std::wstring, bool, std::wstring, ServiceModel::EntryPointType::Trace, std::wstring, std::wstring, std::wstring, int64, bool, Common::DateTime);
        DECLARE_APPLICATIONS_OPERATIONAL_TRACE(ProcessExitedOperational, std::wstring, std::wstring, std::wstring, bool, std::wstring, ServiceModel::EntryPointType::Trace, std::wstring, int64, std::wstring, int64, bool, Common::DateTime);
        DECLARE_STRUCTURED_TRACE(ApplicationHostTerminated, std::wstring, uint64, Common::ActivityDescription);

    public:
        HostingTraces() : 
        HOSTING_STRUCTURED_TRACE(
            ApplicationPackageDownloaded,
            4,
            Info,
            "Application {1}:{2} has been downloaded.",
            "id",
            "applicationId",
            "applicationVersion"),
        HOSTING_STRUCTURED_TRACE(
            ServicePackageDownloaded,
            5,
            Info,
            "ServicePackage {1}:{2} has been downloaded.",
            "id",
            "servicePackageId",
            "servicePackageVersion"),
        HOSTING_STRUCTURED_TRACE(
            ApplicationActivated,
            6,
            Info,
            "Application {1}:{2}:{3} is active.",
            "id",
            "applicationId",
            "applicationVersion",
            "applicationInstanceId"),
        HOSTING_STRUCTURED_TRACE(
            ApplicationDeactivated,
            7,
            Info,
            "Application {1}:{2}:{3} has been deactivated.",
            "id",
            "applicationId",
            "applicationVersion",
            "applicationInstanceId"),
        HOSTING_STRUCTURED_TRACE(
            ServicePackageActivated,
            8,
            Info,
            "ServicePackage {1}:{2}:{3} is active.",
            "id",
            "servicePackageId",
            "servicePackageVersion",
            "servicePackageInstanceId"),
        HOSTING_STRUCTURED_TRACE(
            ServicePackageDeactivated,
            9,
            Info,
            "ServicePackage {1}:{2}:{3} has been deactivated.",
            "id",
            "servicePackageId",
            "servicePackageVersion",
            "servicePackageInstanceId"),
        HOSTING_STRUCTURED_TRACE(
            ServiceTypeRegistered,
            10,
            Info,
            "{1}: ServiceType {2} with ActivationContext {3} and PublicActivationId {4} is registered from ApplicationHost {5} and Runtime {6}.",
            "id",
            "seqNo",
            "versionedServiceTypeId",
            "activationContext",
            "publicActivationId",
            "hostId",
            "runtimeId"),
        HOSTING_STRUCTURED_TRACE(
            FabricRuntimeClosed,
            12,
            Info,
            "{1}: FabricRuntime {2} with ActivationContext {3} and PublicActivationId {4} in ApplicationHost {5} was closed.",
            "id",
            "seqNo",
            "runtimeId",
            "activationContext",
            "publicActivationId",
            "hostId"),
        HOSTING_STRUCTURED_TRACE(
            ApplicationHostClosed,
            13,
            Info,
            "{1}: ApplicationHost {2} with ActivationContext {3} and PublicActivationId {4} was closed.",
            "id",
            "seqNo",
            "hostId",
            "activationContext",
            "publicActivationId"),
        HOSTING_STRUCTURED_TRACE(
            ServiceTypeDisabled,
            14,
            Info,
            "{1}: ServiceType {2} with ActivationContext {3} and PublicActivationId {4} has been disabled.",
            "id",
            "seqNo",
            "serviceTypeId",
            "activationContext",
            "publicActivationId"),
        HOSTING_STRUCTURED_TRACE(
            ServiceTypeEnabled,
            15,
            Info,
            "{1}: ServiceType {2} with ActivationContext {3} and PublicActivationId {4} is enabled.",
            "id",
            "seqNo",
            "serviceTypeId",
            "activationContext",
            "publicActivationId"),
        HOSTING_STRUCTURED_TRACE(
            NodeVersionUpgraded,
            16,
            Info,
            "Node: {1}, Specified Upgrade Version: {2}, UpdatedVersion: {3}",
            "id",
            "NodeId",
            "specVersion",
            "updatedVersion"),
        HOSTING_STRUCTURED_TRACE(
            NodeVersionUpgradeFailed,
            17,
            Warning,
            "Node: {1} version settings upgrade failed: Specified Version {2}, UpdatedVersion: {3}, Error: {4}",
            "id",
            "NodeId",
            "specVersion",
            "updatedVersion",
            "error"),
        HOSTING_STRUCTURED_TRACE(
            HostedServiceActivated,
            18,
            Info,
            "HostedService: {0} activated successfully with ProcessId {1}",
            "ServiceName",
            "ProcessId"),
        HOSTING_STRUCTURED_TRACE(
            HostedServiceActivationFailed,
            19,
            Info,
            "HostedService: {0} activation failed. Error: {1}",
            "ServiceName",
            "ErrorCode"),
        HOSTING_STRUCTURED_TRACE(
            HostedServiceDeactivated,
            20,
            Info,
            "HostedService: {0} with ProcessId {1} deactivated successfully",
            "ServiceName",
            "ProcessId"),
        HOSTING_STRUCTURED_TRACE(
            HostedServiceDeactivationFailed,
            21,
            Info,
            "HostedService: {0} with ProcessId {1} deactivation failed. Error: {2}",
            "ServiceName",
            "ProcessId",
            "ErrorCode"),
        HOSTING_STRUCTURED_TRACE(
            HostedServiceTerminated,
            22,
            Info,
            "HostedService: {0} terminated with code {1}",
            "ServiceName",
            "ReturnCode"),
        HOSTING_STRUCTURED_QUERY_TRACE(
            Nodes,
            HostedServiceUnexpectedTermination,
            23,
            Warning,
            Common::TraceChannelType::Admin,
            TRACE_KEYWORDS2(Default, ForQuery),
            "HostedService: {1} on node id {0} terminated unexpectedly with code {2} and process name {3}",
            "id",
            "ServiceName",
            "ReturnCode",
            "ProcessName"),
        HOSTING_STRUCTURED_TRACE(
            ServicePackageActivatedNotifyDca,
            24,
            Info,
            Common::TraceChannelType::Debug,
            Common::TraceKeywords::AppInstance,
            "Notify DCA: ServicePackage {1}:{3}:{5}:{6} is active.",
            "version",
            "nodeName",
            "runLayoutRoot",
            "applicationId",
            "applicationVersion",
            "servicePackageName",
            "servicePackageVersion"),
        HOSTING_STRUCTURED_TRACE(
            ServicePackageDeactivatedNotifyDca,
            25,
            Info,
            Common::TraceChannelType::Debug,
            Common::TraceKeywords::AppInstance,
            "Notify DCA: ServicePackage {1}:{2}:{3} has been deactivated.",
            "version",
            "nodeName",
            "applicationId",
            "servicePackageName"),
        HOSTING_STRUCTURED_TRACE(
            ServicePackageUpgradedNotifyDca,
            26,
            Info,
            Common::TraceChannelType::Debug,
            Common::TraceKeywords::AppInstance,
            "Notify DCA: ServicePackage {1}:{3}:{5}:{6} is active after upgrade.",
            "version",
            "nodeName",
            "runLayoutRoot",
            "applicationId",
            "applicationVersion",
            "servicePackageName",
            "servicePackageVersion"),
         HOSTING_STRUCTURED_TRACE(
            HostedServiceActivationLimitExceeded,
            27,
            Error,
            "Hosted service {1} cannot be rescheduled since it exceeded max retry limit. Terminating",
            "id",
            "ServiceName"),
        HOSTING_STRUCTURED_TRACE(
            ApplicationInstanceFolderDeletionSuccess,
            28,
            Info,
            "Application Instance folder {1} deleted successfully",
            "id",
            "ApplicationInstanceFolder"),
        HOSTING_STRUCTURED_TRACE(
            ApplicationInstanceFolderDeletionFailed,
            29,
            Warning,
            "Application Instance folder {1} deletion failed with Error {2}",
            "id",
            "ApplicationInstanceFolder",
            "ErrorCode"),
         HOSTING_STRUCTURED_TRACE(
            ApplicationTypeContentDeletionSuccess,
            30,
            Info,
            "ApplicationType {1} content {2} deleted successfully",
            "id",
            "ApplicationType",
            "PackageName"),
         HOSTING_STRUCTURED_TRACE(
            ApplicationTypeContentDeletionFailed,
            31,
            Warning,
            "ApplicationType {1} content {2} deletion failed with Error {3}",
            "id",
            "ApplicationType",
            "PackageName",
            "ErrorCode"),
        HOSTING_STRUCTURED_QUERY_TRACE(
            Nodes,
            ProcessUnexpectedTermination,
            33,
            Warning,
            Common::TraceChannelType::Admin,
            TRACE_KEYWORDS2(Default, ForQuery),
            "ServiceHostProcess: {3} started at {5} with Process Id {4} for ApplicationId {1} terminated unexpectedly with exit code {2} on node id {0}",
            "id",
            "AppId",
            "ReturnCode",
            "ProcessName",
            "ProcessId",
            "StartTime"),
        HOSTING_OPERATIONAL_TRACE(
            Container,
            ContainerActivatedOperational,
            34,
            Info,
            "Container {1} with isolation {2} for application {3} service {4} code package {5} is active.",
            "id",
            "Name",
            "IsolationMode",
            "ApplicationName",
            "ServiceName",
            "CodePackageName"),
        HOSTING_OPERATIONAL_TRACE(
            Container,
            ContainerDeactivatedOperational,
            35,
            Info,
            "Container {1}  with applicationname {2}, servicename {3} and code package {4} deactivated.",
            "id",
            "Name",
            "ApplicationName",
            "ServiceName",
            "CodePackageName"),
        HOSTING_STRUCTURED_TRACE(
            ActiveExecutablesAndContainersStats,
            36,
            Info,
            "Statistics: Number of activated Containers:{0}, Executables:{1}",
            "ContainersCount",
            "ApplicationsCount"),
        HOSTING_STRUCTURED_QUERY_TRACE(
            ContainerEvents,
            ContainerActivated,
            39,
            Info,
            Common::TraceChannelType::Admin,
            TRACE_KEYWORDS2(Default, ForQuery),
            "Container {1} with isolation {2} for application {3} service {4} code package {5} is active.",
            "id",
            "Name",
            "IsolationMode",
            "ApplicationName",
            "ServiceName",
            "CodePackageName"),
        HOSTING_STRUCTURED_QUERY_TRACE(
            ContainerEvents,
            ContainerDeactivated,
            40,
            Info,
            Common::TraceChannelType::Admin,
            TRACE_KEYWORDS2(Default, ForQuery),
            "Container {1}  with applicationname {2}, servicename {3} and code package {4} deactivated.",
            "id",
            "Name",
            "ApplicationName",
            "ServiceName",
            "CodePackageName"),
        HOSTING_STRUCTURED_QUERY_TRACE(
            ContainerEvents,
            ContainerTerminated,
            41,
            Info,
            Common::TraceChannelType::Admin,
            TRACE_KEYWORDS2(Default, ForQuery),
            "Container {1}  with applicationname {2}, servicename {3} and code package {4} terminated.",
            "id",
            "Name",
            "ApplicationName",
            "ServiceName",
            "CodePackageName"),
        APPLICATIONS_OPERATIONAL_TRACE(
            ContainerExitedOperational,
            L"ApplicationContainerInstanceExited",
            OperationalStateTransitionCategory,
            Hosting,
            42,
            Info,
            "ContainerHostTerminated: \r\n ApplicationId={1},\r\n ServiceName={2},\r\n ServicePackageName={3},\r\n ServicePackageActivationId={4},\r\n IsExclusive={5},\r\n CodePackageName={6},\r\n EntryPointType={7},\r\n ImageName={8},\r\n ContainerName={9},\r\n HostId={10},\r\n ExitCode={11},\r\n UnexpectedTermination={12},\r\n StartTime={13}.",
            "ServiceName",
            "ServicePackageName",
            "ServicePackageActivationId",
            "IsExclusive",
            "CodePackageName",
            "EntryPointType",
            "ImageName",
            "ContainerName",
            "HostId",
            "ExitCode",
            "UnexpectedTermination",
            "StartTime"),
        APPLICATIONS_OPERATIONAL_TRACE(
            ProcessExitedOperational,
            L"ApplicationProcessExited",
            OperationalStateTransitionCategory,
            Hosting,
            43,
            Info,
            "ApplicationHostTerminated: \r\n ApplicationId={1},\r\n ServiceName={2},\r\n ServicePackageName={3},\r\n ServicePackageActivationId={4},\r\n IsExclusive={5},\r\n CodePackageName={6},\r\n EntryPointType={7},\r\n ExeName={8},\r\n ProcessId={9},\r\n HostId={10},\r\n ExitCode={11},\r\n UnexpectedTermination={12},\r\n StartTime={13}.",
            "ServiceName",
            "ServicePackageName",
            "ServicePackageActivationId",
            "IsExclusive",
            "CodePackageName",
            "EntryPointType",
            "ExeName",
            "ProcessId",
            "HostId",
            "ExitCode",
            "UnexpectedTermination",
            "StartTime"),
        HOSTING_STRUCTURED_QUERY_TRACE(
            ApplicationHostEvents,
            ApplicationHostTerminated,
            44,
            Info,
            Common::TraceChannelType::Admin,
            TRACE_KEYWORDS2(Default, ForQuery),
            "Processing termination of activated Application Host: HostId={0}, ExitCode={1} ActivityDescription={2}",
            "HostId",
            "ExitCode",
            "ActivityDescription"),
        HOSTING_STRUCTURED_TRACE(
            AvailableContainerImages,
            45,
            Info,
            "Receiving available images from node: NodeId={1}, Number of available images={2}",
            "id",
            "nodeId",
            "numAvailableImages")
        {
        }
    };

    extern HostingTraces const hostingTrace;
}
