// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;

#define COMBINE(_X_, _Y_) *(_X_) + *Constants::MetadataSegment + *(_Y_)

//
// HTTP server constants
//
double Constants::DefaultFabricTimeoutMin            = 1;
double Constants::DefaultAccessCheckTimeoutMin       = 2;
ULONG Constants::MaximumActiveListeners              = 100;
ULONG Constants::DefaultEntityBodyChunkSize          = 4096;
ULONG Constants::DefaultEntityBodyForUploadChunkSize = 1048576; //1MB
ULONG Constants::MaxEntityBodyForUploadChunkSize     = 10485760; //10MB
ULONG Constants::DefaultHeaderBufferSize             = 8192;
GlobalWString Constants::HttpGatewayTraceId          = make_global<wstring>(L"HttpGateway");
double Constants::V1ApiVersion = 1.0;
double Constants::V2ApiVersion = 2.0;
double Constants::V3ApiVersion = 3.0;
double Constants::V4ApiVersion = 4.0;
double Constants::V5ApiVersion = 5.0;
double Constants::V6ApiVersion = 6.0;
double Constants::V61ApiVersion = 6.1;
double Constants::V62ApiVersion = 6.2;
double Constants::V63ApiVersion = 6.3;
double Constants::V64ApiVersion = 6.4;
GlobalWString Constants::PreviewApiVersion           = make_global<wstring>(L"-preview");

ULONG Constants::AllocationTag                       = 'ttHF';
GlobalWString Constants::ApplicationsHandlerPath     = make_global<wstring>(L"/Applications/");
GlobalWString Constants::ApplicationTypesHandlerPath = make_global<wstring>(L"/ApplicationTypes/");
GlobalWString Constants::ClusterManagementHandlerPath = make_global<wstring>(L"/");
GlobalWString Constants::ComposeDeploymentsHandlerPath         = make_global<wstring>(L"/ComposeDeployments/");
GlobalWString Constants::VolumesHandlerPath          = make_global<wstring>(L"/Resources/Volumes/");
GlobalWString Constants::NodesHandlerPath            = make_global<wstring>(L"/Nodes/");
GlobalWString Constants::ServicesHandlerPath         = make_global<wstring>(L"/Services/");
GlobalWString Constants::PartitionsHandlerPath       = make_global<wstring>(L"/Partitions/");
GlobalWString Constants::ImageStoreHandlerPath       = make_global<wstring>(L"/ImageStore/");
GlobalWString Constants::TestCommandsHandlerPath     = make_global<wstring>(L"/TestCommands/");
GlobalWString Constants::ToolsHandlerPath            = make_global<wstring>(L"/Tools/");
GlobalWString Constants::FaultsHandlerPath           = make_global<wstring>(L"/Faults/");
GlobalWString Constants::NamesHandlerPath            = make_global<wstring>(L"/Names/");
GlobalWString Constants::BackupRestoreHandlerPath    = make_global<wstring>(L"/BackupRestore/");
GlobalWString Constants::ApplicationsResourceHandlerPath    = make_global<wstring>(L"/Resources/Applications/");
GlobalWString Constants::SecretsResourceHandlerPath  = make_global<wstring>(L"/Resources/Secrets/");
GlobalWString Constants::GatewaysHandlerPath         = make_global<wstring>(L"/Resources/Gateways/");

GlobalWString Constants::NetworksHandlerPath         = make_global<wstring>(L"/Resources/Networks/");

GlobalWString Constants::ContentTypeHeader           = make_global<wstring>(L"Content-Type");
GlobalWString Constants::ContentLengthHeader         = make_global<wstring>(L"Content-Length");
GlobalWString Constants::ContentRangeHeader          = make_global<wstring>(L"Content-Range");
GlobalWString Constants::ContentRangeUnit            = make_global<wstring>(L"bytes");
GlobalWString Constants::WWWAuthenticateHeader       = make_global<wstring>(L"WWW-Authenticate");
GlobalWString Constants::TransferEncodingHeader      = make_global<wstring>(L"Transfer-Encoding");
GlobalWString Constants::LocationHeader              = make_global<wstring>(L"Location");
GlobalWString Constants::AuthorizationHeader         = make_global<wstring>(L"Authorization");
GlobalWString Constants::Bearer                      = make_global<wstring>(L"Bearer");
GlobalWString Constants::ClusterIdHeader             = make_global<wstring>(L"ClusterId");
GlobalWString Constants::ServerRedirectResponse      = make_global<wstring>(L"Moved Permanently");
GlobalWString Constants::ServiceFabricHttpClientRequestIdHeader = make_global<wstring>(L"X-ServiceFabricRequestId");
GlobalWString Constants::ContentTypeOptionsHeader    = make_global<wstring>(L"X-Content-Type-Options");
GlobalWString Constants::ContentTypeNoSniffValue     = make_global<wstring>(L"nosniff");
GlobalWString Constants::HSTSHeader                  = make_global<wstring>(L"Strict-Transport-Security");

GlobalWString Constants::JsonContentType             = make_global<wstring>(L"application/json; charset=utf-8");
GlobalWString Constants::HtmlContentType             = make_global<wstring>(L"text/html; charset=utf-8");
GlobalWString Constants::CssContentType              = make_global<wstring>(L"text/css; charset=utf-8");
GlobalWString Constants::JavaScriptContentType       = make_global<wstring>(L"text/javascript; charset=utf-8");
GlobalWString Constants::PngContentType              = make_global<wstring>(L"image/png");
GlobalWString Constants::IcoContentType              = make_global<wstring>(L"image/ico");
GlobalWString Constants::EotContentType              = make_global<wstring>(L"application/vnd.ms-fontobject");
GlobalWString Constants::SvgContentType              = make_global<wstring>(L"image/svg+xml");
GlobalWString Constants::TtfContentType              = make_global<wstring>(L"application/font-ttf");
GlobalWString Constants::WoffContentType             = make_global<wstring>(L"application/font-woff");
GlobalWString Constants::Woff2ContentType            = make_global<wstring>(L"application/font-woff2");
GlobalWString Constants::MapContentType              = make_global<wstring>(L"application/octet-stream");

GlobalWString Constants::ServiceKindString           = make_global<wstring>(L"ServiceKind");
GlobalWString Constants::ApplicationDefinitionKindFilterString = make_global<wstring>(L"ApplicationDefinitionKindFilter");
GlobalWString Constants::ApplicationIdString         = make_global<wstring>(L"ApplicationId");
GlobalWString Constants::DeploymentNameString        = make_global<wstring>(L"DeploymentName");
GlobalWString Constants::ContainerNameString         = make_global<wstring>(L"ContainerName");
GlobalWString Constants::ApplicationNameString       = make_global<wstring>(L"ApplicationName");
GlobalWString Constants::ServiceIdString             = make_global<wstring>(L"ServiceId");
GlobalWString Constants::PartitionIdString           = make_global<wstring>(L"PartitionId");
GlobalWString Constants::ServicePackageActivationIdString = make_global<wstring>(L"ServicePackageActivationId");
GlobalWString Constants::ReplicaIdString             = make_global<wstring>(L"ReplicaId");
GlobalWString Constants::ReplicaStatusFilterString   = make_global<wstring>(L"ReplicaStatusFilter");
GlobalWString Constants::NodeStatusFilterString      = make_global<wstring>(L"NodeStatusFilter");
GlobalWString Constants::MaxResultsString            = make_global<wstring>(L"MaxResults");
GlobalWString Constants::ExcludeApplicationParametersString     = make_global<wstring>(L"ExcludeApplicationParameters");
GlobalWString Constants::IncludeHealthStateString    = make_global<wstring>(L"IncludeHealthState");
GlobalWString Constants::NameString                  = make_global<wstring>(L"Name");
GlobalWString Constants::RecursiveString             = make_global<wstring>(L"Recursive");
GlobalWString Constants::IncludeValuesString         = make_global<wstring>(L"IncludeValues");
GlobalWString Constants::PropertyNameString          = make_global<wstring>(L"PropertyName");
GlobalWString Constants::InstanceIdString            = make_global<wstring>(L"InstanceId");
GlobalWString Constants::VolumeNameString            = make_global<wstring>(L"VolumeName");
GlobalWString Constants::SecretNameString            = make_global<wstring>(L"SecretName");
GlobalWString Constants::SecretVersionString         = make_global<wstring>(L"SecretVersion");
GlobalWString Constants::IncludeValueString          = make_global<wstring>(L"IncludeValue");
GlobalWString Constants::NetworkNameString           = make_global<wstring>(L"NetworkName");
GlobalWString Constants::NetworkStatusFilterString   = make_global<wstring>(L"NetworkStatusFilter");
GlobalWString Constants::GatewayNameString           = make_global<wstring>(L"GatewayName");

// This actually represents the node name, not the node id
GlobalWString Constants::NodeIdString                = make_global<wstring>(L"NodeId");

GlobalWString Constants::NodeInstanceIdString        = make_global<wstring>(L"NodeInstanceId");

GlobalWString Constants::OperationIdString           = make_global<wstring>(L"OperationId");
GlobalWString Constants::StartTimeUtcString          = make_global<wstring>(L"StartTimeUtc");
GlobalWString Constants::EndTimeUtcString            = make_global<wstring>(L"EndTimeUtc");

GlobalWString Constants::Services                      = make_global<wstring>(L"Services");
GlobalWString Constants::OperationType               = make_global<wstring>(L"OperationType");

GlobalWString Constants::Mode                        = make_global<wstring>(L"Mode");
GlobalWString Constants::DataLossMode                = make_global<wstring>(L"DataLossMode");
GlobalWString Constants::QuorumLossMode              = make_global<wstring>(L"QuorumLossMode");
GlobalWString Constants::RestartPartitionMode        = make_global<wstring>(L"RestartPartitionMode");
GlobalWString Constants::QuorumLossDuration=           make_global<wstring>(L"QuorumLossDuration");

GlobalWString Constants::ApplicationTypeDefinitionKindFilterString = make_global<wstring>(L"ApplicationTypeDefinitionKindFilter");
GlobalWString Constants::ApplicationTypeNameString   = make_global<wstring>(L"ApplicationTypeName");
GlobalWString Constants::ServiceManifestNameIdString = make_global<wstring>(L"ServiceManifestName");
GlobalWString Constants::CodePackageNameIdString     = make_global<wstring>(L"CodePackageName");
GlobalWString Constants::ServiceTypeNameIdString     = make_global<wstring>(L"ServiceTypeName");
GlobalWString Constants::ServiceTypeNameString       = make_global<wstring>(L"ServiceTypeName");
GlobalWString Constants::CodePackageInstanceIdString = make_global<wstring>(L"CodePackageInstanceId");
GlobalWString Constants::CommandString               = make_global<wstring>(L"Command");
GlobalWString Constants::SessionIdString             = make_global<wstring>(L"session-id");
GlobalWString Constants::ApiVersionString            = make_global<wstring>(L"api-version");
GlobalWString Constants::TimeoutString               = make_global<wstring>(L"timeout");
GlobalWString Constants::ApplicationTypeVersionString = make_global<wstring>(L"ApplicationTypeVersion");
GlobalWString Constants::ClusterManifestVersionString = make_global<wstring>(L"ClusterManifestVersion");
GlobalWString Constants::ConfigurationApiVersionString = make_global<wstring>(L"ConfigurationApiVersion");
GlobalWString Constants::ServiceManifestNameString   = make_global<wstring>(L"ServiceManifestName");
GlobalWString Constants::UpgradeDomainNameString     = make_global<wstring>(L"UpgradeDomainName");
GlobalWString Constants::ContinuationTokenString     = make_global<wstring>(L"ContinuationToken");
GlobalWString Constants::NativeImageStoreRelativePathString = make_global<wstring>(L"RelativePath");

GlobalWString Constants::TestCommandsString           = make_global<wstring>(L"TestCommands");
GlobalWString Constants::StateFilterString = make_global<wstring>(L"StateFilter");
GlobalWString Constants::TypeFilterString  = make_global<wstring>(L"TypeFilter");
GlobalWString Constants::PartitionSelectorTypeString  = make_global<wstring>(L"PartitionSelectorType");
GlobalWString Constants::ModeString                   = make_global<wstring>(L"Mode");
GlobalWString Constants::CommandType                  = make_global<wstring>(L"CommandType");

GlobalWString Constants::CreateServiceFromTemplate   = make_global<wstring>(L"CreateFromTemplate");
GlobalWString Constants::Create                      = make_global<wstring>(L"Create");
GlobalWString Constants::CreateServiceGroup          = make_global<wstring>(L"CreateServiceGroup");
GlobalWString Constants::Copy                        = make_global<wstring>(L"Copy");
GlobalWString Constants::Update                      = make_global<wstring>(L"Update");
GlobalWString Constants::UpdateServiceGroup          = make_global<wstring>(L"UpdateServiceGroup");
GlobalWString Constants::Delete                      = make_global<wstring>(L"Delete");
GlobalWString Constants::DeleteServiceGroup          = make_global<wstring>(L"DeleteServiceGroup");
GlobalWString Constants::GetServiceDescription       = make_global<wstring>(L"GetDescription");
GlobalWString Constants::GetServiceGroupDescription  = make_global<wstring>(L"GetServiceGroupDescription");
GlobalWString Constants::GetServiceGroupMembers      = make_global<wstring>(L"GetServiceGroupMembers");
GlobalWString Constants::UploadChunk                 = make_global<wstring>(L"UploadChunk");
GlobalWString Constants::DeleteUploadSession         = make_global<wstring>(L"DeleteUploadSession");
GlobalWString Constants::GetUploadSession            = make_global<wstring>(L"GetUploadSession");
GlobalWString Constants::CommitUploadSession         = make_global<wstring>(L"CommitUploadSession");
GlobalWString Constants::Provision                   = make_global<wstring>(L"Provision");
GlobalWString Constants::UnProvision                 = make_global<wstring>(L"Unprovision");
GlobalWString Constants::Upgrade                     = make_global<wstring>(L"Upgrade");
GlobalWString Constants::UpdateUpgrade               = make_global<wstring>(L"UpdateUpgrade");
GlobalWString Constants::RollbackUpgrade             = make_global<wstring>(L"RollbackUpgrade");
GlobalWString Constants::GetUpgradeProgress          = make_global<wstring>(L"GetUpgradeProgress");
GlobalWString Constants::MoveToNextUpgradeDomain     = make_global<wstring>(L"MoveToNextUpgradeDomain");
GlobalWString Constants::GetClusterManifest          = make_global<wstring>(L"GetClusterManifest");
GlobalWString Constants::GetClusterVersion           = make_global<wstring>(L"GetClusterVersion");
GlobalWString Constants::GetApplicationManifest      = make_global<wstring>(L"GetApplicationManifest");
GlobalWString Constants::GetServicePackage           = make_global<wstring>(L"GetServicePackages");
GlobalWString Constants::GetServiceManifest          = make_global<wstring>(L"GetServiceManifest");
GlobalWString Constants::Activate                    = make_global<wstring>(L"Activate");
GlobalWString Constants::Stop                        = make_global<wstring>(L"Stop");
GlobalWString Constants::Start                       = make_global<wstring>(L"Start");
GlobalWString Constants::Deactivate                  = make_global<wstring>(L"Deactivate");
GlobalWString Constants::RemoveNodeState             = make_global<wstring>(L"RemoveNodeState");
GlobalWString Constants::GetServiceTypes             = make_global<wstring>(L"GetServiceTypes");
GlobalWString Constants::GetServices                 = make_global<wstring>(L"GetServices");
GlobalWString Constants::GetServiceGroups            = make_global<wstring>(L"GetServiceGroups");
GlobalWString Constants::GetSystemServices           = make_global<wstring>(L"GetSystemServices");
GlobalWString Constants::GetPartitions               = make_global<wstring>(L"GetPartitions");
GlobalWString Constants::GetReplicas                 = make_global<wstring>(L"GetReplicas");
GlobalWString Constants::GetApplications             = make_global<wstring>(L"GetApplications");
GlobalWString Constants::Applications                = make_global<wstring>(L"Applications");
GlobalWString Constants::GetNodes                    = make_global<wstring>(L"GetNodes");
GlobalWString Constants::Nodes                       = make_global<wstring>(L"Nodes");
GlobalWString Constants::GetNetworks                 = make_global<wstring>(L"GetNetworks");
GlobalWString Constants::GetCodePackages             = make_global<wstring>(L"GetCodePackages");
GlobalWString Constants::GetHealth                   = make_global<wstring>(L"GetHealth");
GlobalWString Constants::GetLoadInformation          = make_global<wstring>(L"GetLoadInformation");
GlobalWString Constants::GetUnplacedReplicaInformation = make_global<wstring>(L"GetUnplacedReplicaInformation");
GlobalWString Constants::ReportHealth                = make_global<wstring>(L"ReportHealth");
GlobalWString Constants::StartPartitionRestart       = make_global<wstring>(L"StartPartitionRestart");
GlobalWString Constants::StartPartitionQuorumLoss    = make_global<wstring>(L"StartPartitionQuorumLoss");
GlobalWString Constants::StartPartitionDataLoss      = make_global<wstring>(L"StartPartitionDataLoss");
GlobalWString Constants::Report                      = make_global<wstring>(L"Report");
GlobalWString Constants::Instances                   = make_global<wstring>(L"Instances");
GlobalWString Constants::Schedule                    = make_global<wstring>(L"Schedule");

GlobalWString Constants::GetPartitionRestartProgress = make_global<wstring>(L"GetPartitionRestartProgress");
GlobalWString Constants::GetTestCommands             = make_global<wstring>(L"GetTestCommands");
GlobalWString Constants::Cancel                      = make_global<wstring>(L"Cancel");
GlobalWString Constants::StartClusterConfigurationUpgrade     = make_global<wstring>(L"StartClusterConfigurationUpgrade");
GlobalWString Constants::GetClusterConfigurationUpgradeStatus = make_global<wstring>(L"GetClusterConfigurationUpgradeStatus");
GlobalWString Constants::GetClusterConfiguration     = make_global<wstring>(L"GetClusterConfiguration");
GlobalWString Constants::GetUpgradesPendingApproval  = make_global<wstring>(L"GetUpgradesPendingApproval");
GlobalWString Constants::StartApprovedUpgrades       = make_global<wstring>(L"StartApprovedUpgrades");
GlobalWString Constants::GetUpgradeOrchestrationServiceState  = make_global<wstring>(L"GetUpgradeOrchestrationServiceState");
GlobalWString Constants::SetUpgradeOrchestrationServiceState = make_global<wstring>(L"SetUpgradeOrchestrationServiceState");

GlobalWString Constants::GetClusterHealth            = make_global<wstring>(L"GetClusterHealth");
GlobalWString Constants::GetClusterHealthChunk       = make_global<wstring>(L"GetClusterHealthChunk");
GlobalWString Constants::ReportClusterHealth         = make_global<wstring>(L"ReportClusterHealth");
GlobalWString Constants::Recover                     = make_global<wstring>(L"Recover");
GlobalWString Constants::RecoverAllPartitions        = make_global<wstring>(L"RecoverAllPartitions");
GlobalWString Constants::RecoverSystemPartitions     = make_global<wstring>(L"RecoverSystemPartitions");
GlobalWString Constants::ResetLoad                   = make_global<wstring>(L"ResetLoad");
GlobalWString Constants::ToggleServicePlacementHealthReportingVerbosity         = make_global<wstring>(L"ToggleServicePlacementHealthReportingVerbosity");
GlobalWString Constants::GetdSTSMetadata             = make_global<wstring>(L"GetDstsMetadata");
GlobalWString Constants::GetAadMetadata              = make_global<wstring>(L"GetAadMetadata");
GlobalWString Constants::Restart                     = make_global<wstring>(L"Restart");
GlobalWString Constants::Logs                        = make_global<wstring>(L"Logs");
GlobalWString Constants::ContainerLogs               = make_global<wstring>(L"ContainerLogs");
GlobalWString Constants::ContainerApi                = make_global<wstring>(L"ContainerApi");
GlobalWString Constants::ContainerApiPathString = make_global<wstring>(L"ContainerApiPath");
GlobalWString Constants::ContainerApiPath = make_global<wstring>(*ContainerApi + L"/{" + *ContainerApiPathString + L'}');
GlobalWString Constants::Remove                      = make_global<wstring>(L"Remove");
GlobalWString Constants::GetHealthStateList          = make_global<wstring>(L"GetHealthStateList");
GlobalWString Constants::GetReplicaHealthStateList   = make_global<wstring>(L"GetReplicaHealthStateList");
GlobalWString Constants::InvokeInfrastructureCommand = make_global<wstring>(L"InvokeInfrastructureCommand");
GlobalWString Constants::InvokeInfrastructureQuery   = make_global<wstring>(L"InvokeInfrastructureQuery");
GlobalWString Constants::GetDeployedApplicationHealthStateList      = make_global<wstring>(L"GetDeployedApplicationHealthStateList");
GlobalWString Constants::GetDeployedServicePackageHealthStateList   = make_global<wstring>(L"GetDeployedServicePackageHealthStateList");
GlobalWString Constants::GetDeployedReplicaDetail                   = make_global<wstring>(L"GetDetail");
GlobalWString Constants::GetProvisionedFabricCodeVersions           = make_global<wstring>(L"GetProvisionedCodeVersions");
GlobalWString Constants::GetProvisionedFabricConfigVersions         = make_global<wstring>(L"GetProvisionedConfigVersions");
GlobalWString Constants::EventsHealthStateFilterString = make_global<wstring>(L"EventsHealthStateFilter");
GlobalWString Constants::NodesHealthStateFilterString = make_global<wstring>(L"NodesHealthStateFilter");
GlobalWString Constants::ReplicasHealthStateFilterString = make_global<wstring>(L"ReplicasHealthStateFilter");
GlobalWString Constants::PartitionsHealthStateFilterString = make_global<wstring>(L"PartitionsHealthStateFilter");
GlobalWString Constants::ServicesHealthStateFilterString = make_global<wstring>(L"ServicesHealthStateFilter");
GlobalWString Constants::ApplicationsHealthStateFilterString = make_global<wstring>(L"ApplicationsHealthStateFilter");
GlobalWString Constants::DeployedApplicationsHealthStateFilterString = make_global<wstring>(L"DeployedApplicationsHealthStateFilter");
GlobalWString Constants::DeployedServicePackagesHealthStateFilterString = make_global<wstring>(L"DeployedServicePackagesHealthStateFilter");
GlobalWString Constants::ExcludeHealthStatisticsString = make_global<wstring>(L"ExcludeHealthStatistics");
GlobalWString Constants::IncludeSystemApplicationHealthStatisticsString = make_global<wstring>(L"IncludeSystemApplicationHealthStatistics");
GlobalWString Constants::ResolvePartition            = make_global<wstring>(L"ResolvePartition");

GlobalWString Constants::CreateRepairTask           = make_global<wstring>(L"CreateRepairTask");
GlobalWString Constants::CancelRepairTask           = make_global<wstring>(L"CancelRepairTask");
GlobalWString Constants::ForceApproveRepairTask     = make_global<wstring>(L"ForceApproveRepairTask");
GlobalWString Constants::DeleteRepairTask           = make_global<wstring>(L"DeleteRepairTask");
GlobalWString Constants::UpdateRepairExecutionState = make_global<wstring>(L"UpdateRepairExecutionState");
GlobalWString Constants::GetRepairTaskList          = make_global<wstring>(L"GetRepairTaskList");
GlobalWString Constants::UpdateRepairTaskHealthPolicy = make_global<wstring>(L"UpdateRepairTaskHealthPolicy");
GlobalWString Constants::TaskIdFilter               = make_global<wstring>(L"TaskIdFilter");
GlobalWString Constants::StateFilter                = make_global<wstring>(L"StateFilter");
GlobalWString Constants::ExecutorFilter             = make_global<wstring>(L"ExecutorFilter");
GlobalWString Constants::GetServiceName             = make_global<wstring>(L"GetServiceName");
GlobalWString Constants::GetApplicationName         = make_global<wstring>(L"GetApplicationName");
GlobalWString Constants::GetSubNames                = make_global<wstring>(L"GetSubNames");
GlobalWString Constants::GetProperty                = make_global<wstring>(L"GetProperty");
GlobalWString Constants::GetProperties              = make_global<wstring>(L"GetProperties");
GlobalWString Constants::SubmitBatch                = make_global<wstring>(L"SubmitBatch");

GlobalWString Constants::DeployServicePackageToNode = make_global<wstring>(L"DeployServicePackage");

GlobalWString Constants::PartitionKeyType            = make_global<wstring>(L"PartitionKeyType");
GlobalWString Constants::PartitionKeyValue           = make_global<wstring>(L"PartitionKeyValue");
GlobalWString Constants::PreviousRspVersion          = make_global<wstring>(L"PreviousRspVersion");

GlobalWString Constants::ForceRemove                 = make_global<wstring>(L"ForceRemove");
GlobalWString Constants::CreateFabricDump            = make_global<wstring>(L"CreateFabricDump");

GlobalWString Constants::SegmentDelimiter            = make_global<wstring>(L"/");
GlobalWString Constants::MetadataSegment             = make_global<wstring>(L"/$/");
GlobalWString Constants::QueryStringDelimiter        = make_global<wstring>(L"?");
GlobalWString Constants::ApplicationTypesEntitySetPath = make_global<wstring>(L"ApplicationTypes/");
GlobalWString Constants::TestCommandProgressPath = make_global<wstring>(L"GetProgress");

GlobalWString Constants::BackupRestorePrefix         = make_global<wstring>(L"BackupRestore");

GlobalWString Constants::BackupRestorePath           = make_global<wstring>(L"BackupRestore/{Path}");
GlobalWString Constants::BackupRestoreServiceName    = make_global<wstring>(L"System/BackupRestoreService");
GlobalWString Constants::EnableBackup                = make_global<wstring>(L"EnableBackup");
GlobalWString Constants::GetBackupConfigurationInfo  = make_global<wstring>(L"GetBackupConfigurationInfo");
GlobalWString Constants::DisableBackup               = make_global<wstring>(L"DisableBackup");
GlobalWString Constants::SuspendBackup               = make_global<wstring>(L"SuspendBackup");
GlobalWString Constants::ResumeBackup                = make_global<wstring>(L"ResumeBackup");
GlobalWString Constants::GetBackups                  = make_global<wstring>(L"GetBackups");
GlobalWString Constants::Restore                     = make_global<wstring>(L"Restore");
GlobalWString Constants::GetRestoreProgress          = make_global<wstring>(L"GetRestoreProgress");
GlobalWString Constants::Backup                      = make_global<wstring>(L"Backup");
GlobalWString Constants::GetBackupProgress           = make_global<wstring>(L"GetBackupProgress");

GlobalWString Constants::RequestMetadataHeaderName   = make_global<wstring>(L"X-Request-Metadata");
GlobalWString Constants::ClientRoleHeaderName        = make_global<wstring>(L"X-Role");

GlobalWString Constants::ApplicationTypesEntityKeyPath = make_global<wstring>(L"ApplicationTypes/{ApplicationTypeName}/");
GlobalWString Constants::ServiceTypesEntitySetPath   = make_global<wstring>(COMBINE(Constants::ApplicationTypesEntityKeyPath, Constants::GetServiceTypes));
GlobalWString Constants::ServiceTypesEntityKeyPath   = make_global<wstring>(*Constants::ServiceTypesEntitySetPath + L"/{ServiceTypeName}/");

GlobalWString Constants::SecretsResourceValueListAction           = make_global<wstring>(L"list_value");
GlobalWString Constants::SecretsResourceEntitySetPath             = make_global<wstring>(L"Resources/Secrets/");
GlobalWString Constants::SecretsResourceEntityKeyPath             = make_global<wstring>(*Constants::SecretsResourceEntitySetPath + L"{" + *Constants::SecretNameString + L"}/");
GlobalWString Constants::SecretsResourceVersionEntitySetPath      = make_global<wstring>(*Constants::SecretsResourceEntityKeyPath + L"values/");
GlobalWString Constants::SecretsResourceVersionEntityKeyPath      = make_global<wstring>(*Constants::SecretsResourceVersionEntitySetPath + L"{" + *Constants::SecretVersionString + L"}/");
GlobalWString Constants::SecretsResourceVersionEntityKeyValuePath = make_global<wstring>(*Constants::SecretsResourceVersionEntityKeyPath + *Constants::SecretsResourceValueListAction);

GlobalWString Constants::ApplicationsEntitySetPath   = make_global<wstring>(L"Applications/");
GlobalWString Constants::ApplicationsEntityKeyPath   = make_global<wstring>(L"Applications/{ApplicationId}/");
GlobalWString Constants::ComposeDeploymentsEntitySetPath       = make_global<wstring>(L"ComposeDeployments/");
GlobalWString Constants::ComposeDeploymentsEntityKeyPath      = make_global<wstring>(L"ComposeDeployments/{DeploymentName}/");
GlobalWString Constants::ContainerEntityKeyPath = make_global<wstring>(L"Containers/{ContainerName}/");
GlobalWString Constants::VolumesEntitySetPath = make_global<wstring>(L"Resources/Volumes/");
GlobalWString Constants::VolumesEntityKeyPath = make_global<wstring>(L"Resources/Volumes/{VolumeName}/");
GlobalWString Constants::ApplicationsResourceEntitySetPath = make_global<wstring>(L"Resources/Applications/");
GlobalWString Constants::ApplicationsResourceEntityKeyPath = make_global<wstring>(L"Resources/Applications/{ApplicationId}/");
GlobalWString Constants::ServicesResourceEntitySetPath = make_global<wstring>(*Constants::ApplicationsResourceEntityKeyPath + L"Services/");
GlobalWString Constants::ServicesResourceEntityKeyPath = make_global<wstring>(*Constants::ServicesResourceEntitySetPath + L"{ServiceId}/");
GlobalWString Constants::ReplicasResourceEntitySetPath = make_global<wstring>(*Constants::ServicesResourceEntityKeyPath + L"Replicas/");
GlobalWString Constants::ReplicasResourceEntityKeyPath = make_global<wstring>(*Constants::ReplicasResourceEntitySetPath + L"{ReplicaId}/");
GlobalWString Constants::ContainerCodePackageKeyPath = make_global<wstring>(*Constants::ReplicasResourceEntityKeyPath + L"CodePackages/{CodePackageName}/");
GlobalWString Constants::ContainerCodePackageLogsKeyPath = make_global<wstring>(*Constants::ContainerCodePackageKeyPath + L"Logs");
GlobalWString Constants::ServicesEntitySetPathViaApplication = make_global<wstring>(COMBINE(Constants::ApplicationsEntityKeyPath, Constants::GetServices));
GlobalWString Constants::ServiceGroupMembersEntitySetPathViaApplication = make_global<wstring>(COMBINE(Constants::ApplicationsEntityKeyPath, Constants::GetServiceGroupMembers));
GlobalWString Constants::ServiceGroupsEntitySetPathViaApplication = make_global<wstring>(COMBINE(Constants::ApplicationsEntityKeyPath, Constants::GetServiceGroups));
GlobalWString Constants::ServicesEntityKeyPathViaApplication = make_global<wstring>(*Constants::ServicesEntitySetPathViaApplication + L"/{ServiceId}/");
GlobalWString Constants::ServiceGroupsEntityKeyPathViaApplication = make_global<wstring>(*Constants::ServiceGroupsEntitySetPathViaApplication + L"/{ServiceId}/");
GlobalWString Constants::ServicePartitionEntitySetPathViaApplication = make_global<wstring>(COMBINE(Constants::ServicesEntityKeyPathViaApplication, Constants::GetPartitions));
GlobalWString Constants::ServicePartitionEntityKeyPathViaApplication = make_global<wstring>(*Constants::ServicePartitionEntitySetPathViaApplication + L"/{PartitionId}");
GlobalWString Constants::ServiceReplicaEntitySetPathViaApplication = make_global<wstring>(*Constants::ServicePartitionEntitySetPathViaApplication + L"/{PartitionId}" + *Constants::MetadataSegment + *Constants::GetReplicas);
GlobalWString Constants::ServiceReplicaEntityKeyPathViaApplication = make_global<wstring>(*Constants::ServiceReplicaEntitySetPathViaApplication + L"/{ReplicaId}");

GlobalWString Constants::GatewaysResourceEntitySetPath = make_global<wstring>(L"Resources/Gateways/");
GlobalWString Constants::GatewaysResourceEntityKeyPath = make_global<wstring>(L"Resources/Gateways/{GatewayName}/");

GlobalWString Constants::ServicesEntitySetPath    = make_global<wstring>(L"Services/");
GlobalWString Constants::ServiceGroupsEntitySetPath    = make_global<wstring>(L"ServiceGroups/");
GlobalWString Constants::ServicesEntityKeyPath    = make_global<wstring>(*Constants::ServicesEntitySetPath + L"/{ServiceId}/");
GlobalWString Constants::ServiceGroupsEntityKeyPath    = make_global<wstring>(*Constants::ServiceGroupsEntitySetPath + L"/{ServiceId}/");
GlobalWString Constants::ServicePartitionEntitySetPathViaService = make_global<wstring>(COMBINE(Constants::ServicesEntityKeyPath, Constants::GetPartitions));
GlobalWString Constants::ServicePartitionEntityKeyPathViaService = make_global<wstring>(*Constants::ServicePartitionEntitySetPathViaService + L"/{PartitionId}");
GlobalWString Constants::ServiceReplicaEntitySetPathViaService = make_global<wstring>(*Constants::ServicePartitionEntitySetPathViaService + L"/{PartitionId}" + *Constants::MetadataSegment + *Constants::GetReplicas);
GlobalWString Constants::ServiceReplicaEntityKeyPathViaService = make_global<wstring>(*Constants::ServiceReplicaEntitySetPathViaService + L"/{ReplicaId}");

GlobalWString Constants::ServicePartitionEntitySetPath = make_global<wstring>(L"Partitions/");
GlobalWString Constants::ServicePartitionEntityKeyPath = make_global<wstring>(*Constants::ServicePartitionEntitySetPath + L"/{PartitionId}");
GlobalWString Constants::ServiceReplicaEntitySetPathViaPartition = make_global<wstring>(*Constants::ServicePartitionEntitySetPath + L"/{PartitionId}" + *Constants::MetadataSegment + *Constants::GetReplicas);
GlobalWString Constants::ServiceReplicaEntityKeyPathViaPartition = make_global<wstring>(*Constants::ServiceReplicaEntitySetPathViaPartition + L"/{ReplicaId}");

GlobalWString Constants::SystemEntitySetPath          = make_global<wstring>(L"");
GlobalWString Constants::NodesEntitySetPath          = make_global<wstring>(L"Nodes/");
GlobalWString Constants::NodesEntityKeyPath         = make_global<wstring>(L"Nodes/{NodeId}/");

GlobalWString Constants::NetworksEntitySetPath                      = make_global<wstring>(L"Resources/Networks");
GlobalWString Constants::NetworksEntityKeyPath                      = make_global<wstring>(L"Resources/Networks/{NetworkName}");
GlobalWString Constants::ApplicationsEntitySetPathViaNetwork        = make_global<wstring>(*Constants::NetworksEntityKeyPath + L"/ApplicationRefs");
GlobalWString Constants::ApplicationsEntityKeyPathViaNetwork        = make_global<wstring>(*Constants::ApplicationsEntitySetPathViaNetwork + L"/{ApplicationId}");
GlobalWString Constants::NodesEntitySetPathViaNetwork               = make_global<wstring>(*Constants::NetworksEntityKeyPath + L"/DeployedNodes");
GlobalWString Constants::NodesEntityKeyPathViaNetwork               = make_global<wstring>(*Constants::NodesEntitySetPathViaNetwork + L"/{NodeId}");
GlobalWString Constants::NetworksEntitySetPathViaApplication        = make_global<wstring>(COMBINE(Constants::ApplicationsEntityKeyPath, Constants::GetNetworks));
GlobalWString Constants::NetworksOnNodeEntitySetPath                = make_global<wstring>(COMBINE(Constants::NodesEntityKeyPath, Constants::GetNetworks));
GlobalWString Constants::NetworksOnNodeEntityKeyPath                = make_global<wstring>(*Constants::NetworksOnNodeEntitySetPath + L"/{NetworkName}");
GlobalWString Constants::CodePackagesEntitySetPathViaNetworkViaNode = make_global<wstring>(COMBINE(Constants::NetworksOnNodeEntityKeyPath, Constants::GetCodePackages));
GlobalWString Constants::CodePackagesEntityKeyPathViaNetworkViaNode = make_global<wstring>(*Constants::CodePackagesEntitySetPathViaNetworkViaNode + L"/{CodePackageName}");


GlobalWString Constants::TestCommandsSetPath          = make_global<wstring>(L"TestCommands/");
GlobalWString Constants::TestCommandsEntityKeyPath         = make_global<wstring>(L"TestCommands/{OperationId}/");

GlobalWString Constants::ToolsEntitySetPath        = make_global<wstring>(L"Tools/");
GlobalWString Constants::ToolsEntityKeyPath        = make_global<wstring>(L"Tools/{ToolName}/");
GlobalWString Constants::ChaosEntityKeyPath        = make_global<wstring>(L"Tools/Chaos");
GlobalWString Constants::ChaosEventSegmentsSetPath = make_global<wstring>(*Constants::ChaosEntityKeyPath + *Constants::SegmentDelimiter + L"Events");
GlobalWString Constants::ChaosScheduleKeyPath      = make_global<wstring>(*Constants::ChaosEntityKeyPath + *Constants::SegmentDelimiter + *Constants::Schedule);

GlobalWString Constants::FaultsEntitySetPath = make_global<wstring>(L"Faults/");

GlobalWString Constants::NamesEntitySetPath             = make_global<wstring>(L"Names/");
GlobalWString Constants::NamesEntityKeyPath             = make_global<wstring>(L"Names/{Name}/");
GlobalWString Constants::PropertyEntityKeyPathViaName   = make_global<wstring>(COMBINE(Constants::NamesEntityKeyPath, Constants::GetProperty));
GlobalWString Constants::PropertiesEntityKeyPathViaName = make_global<wstring>(COMBINE(Constants::NamesEntityKeyPath, Constants::GetProperties));

GlobalWString Constants::StartPartitionFaultPrefix = make_global<wstring>(*Constants::FaultsEntitySetPath + L"Services" + L"/{ServiceId}" + *Constants::MetadataSegment + *Constants::GetPartitions + L"/{PartitionId}" + *Constants::MetadataSegment);

GlobalWString Constants::StartDataLoss= make_global<wstring>(*Constants::StartPartitionFaultPrefix + L"/StartDataLoss");
GlobalWString Constants::StartQuorumLoss   = make_global<wstring>(*Constants::StartPartitionFaultPrefix + L"/StartQuorumLoss");
GlobalWString Constants::StartRestart   = make_global<wstring>(*Constants::StartPartitionFaultPrefix + L"/StartRestart");

GlobalWString Constants::GetDataLossProgress = make_global<wstring>(*Constants::StartPartitionFaultPrefix + L"/GetDataLossProgress");
GlobalWString Constants::GetQuorumLossProgress = make_global<wstring>(*Constants::StartPartitionFaultPrefix  + L"/GetQuorumLossProgress");
GlobalWString Constants::GetRestartProgress = make_global<wstring>(*Constants::StartPartitionFaultPrefix  + L"/GetRestartProgress");

GlobalWString Constants::StartNodeFaultPrefix = make_global<wstring>(*Constants::FaultsEntitySetPath + L"Nodes" + L"/{NodeId}" + *Constants::MetadataSegment);
GlobalWString Constants::StartNodeTransition    = make_global<wstring>(*Constants::StartNodeFaultPrefix + L"/StartTransition");
GlobalWString Constants::StartNodeTransitionProgress = make_global<wstring>(*Constants::StartNodeFaultPrefix + L"GetTransitionProgress");

// <endpoint>/Faults/Services/<service name>/
GlobalWString Constants::FaultServicesEntityKeyPath    = make_global<wstring>(*Constants::FaultsEntitySetPath + *Constants::ServicesEntitySetPath + L"{ServiceId}");

// <endpoint>/Faults/Services/<service name>/$/GetPartitions
GlobalWString Constants::FaultServicePartitionEntitySetPathViaService = make_global<wstring>(*Constants::FaultServicesEntityKeyPath + *Constants::MetadataSegment + *Constants::GetPartitions);

GlobalWString Constants::FaultServicePartitionEntityKeyPathViaService = make_global<wstring>(*Constants::FaultServicePartitionEntitySetPathViaService + L"/{PartitionId}");

GlobalWString Constants::SystemServicesEntitySetPath = make_global<wstring>(COMBINE(Constants::SystemEntitySetPath, Constants::GetSystemServices));
GlobalWString Constants::SystemServicesEntityKeyPath = make_global<wstring>(*Constants::SystemServicesEntitySetPath + L"/{ServiceId}");
GlobalWString Constants::SystemServicePartitionEntitySetPath = make_global<wstring>(*Constants::SystemServicesEntityKeyPath + *Constants::MetadataSegment + *Constants::GetPartitions);
GlobalWString Constants::SystemServiceReplicaEntitySetPath = make_global<wstring>(*Constants::SystemServicePartitionEntitySetPath + L"/{PartitionId}" + *Constants::MetadataSegment + *Constants::GetReplicas);
GlobalWString Constants::ApplicationsOnNodeEntitySetPath = make_global<wstring>(COMBINE(Constants::NodesEntityKeyPath, Constants::GetApplications));
GlobalWString Constants::ApplicationsOnNodeEntityKeyPath = make_global<wstring>(*Constants::ApplicationsOnNodeEntitySetPath + L"/{ApplicationId}/");
GlobalWString Constants::ServicePackagesOnNodeEntitySetPath = make_global<wstring>(COMBINE(Constants::ApplicationsOnNodeEntityKeyPath, Constants::GetServicePackage));
GlobalWString Constants::ServicePackagesOnNodeEntityKeyPath = make_global<wstring>(*Constants::ServicePackagesOnNodeEntitySetPath + L"/{ServiceManifestName}");
GlobalWString Constants::CodePackagesOnNodeEntitySetPath = make_global<wstring>(COMBINE(Constants::ApplicationsOnNodeEntityKeyPath, Constants::GetCodePackages));
GlobalWString Constants::CodePackagesOnNodeEntityKeyPath = make_global<wstring>(*Constants::CodePackagesOnNodeEntitySetPath + L"/{CodePackageName}/");
GlobalWString Constants::ServiceReplicasOnNodeEntitySetPath = make_global<wstring>(COMBINE(Constants::ApplicationsOnNodeEntityKeyPath, Constants::GetReplicas));
GlobalWString Constants::ServiceTypesOnNodeEntitySetPath = make_global<wstring>(COMBINE(Constants::ApplicationsOnNodeEntityKeyPath, Constants::GetServiceTypes));
GlobalWString Constants::ServiceTypesOnNodeEntityKeyPath = make_global<wstring>(*Constants::ServiceTypesOnNodeEntitySetPath + L"/{ServiceTypeName}/");
GlobalWString Constants::ApplicationServiceTypesEntitySetPath = make_global<wstring>(COMBINE(Constants::ApplicationsEntityKeyPath, Constants::GetServiceTypes));
GlobalWString Constants::ApplicationServiceTypesEntityKeyPath = make_global<wstring>(*Constants::ApplicationServiceTypesEntitySetPath + L"/{ServiceTypeName}/");
GlobalWString Constants::ImageStoreRootPath = make_global<wstring>(L"ImageStore");
GlobalWString Constants::ImageStoreRelativePath = make_global<wstring>(*Constants::ImageStoreRootPath + L"/{RelativePath}");

// /Nodes/{NodeId}/$/Partitions/{PartitionId}
GlobalWString Constants::ServicePartitionEntitySetPathViaNode = make_global<wstring>(COMBINE(Constants::NodesEntityKeyPath, Constants::GetPartitions));
GlobalWString Constants::ServicePartitionEntityKeyPathViaNode = make_global<wstring>(*Constants::ServicePartitionEntitySetPathViaNode + L"/{PartitionId}");

// /Nodes/{NodeId}/$/Partitions/{PartitionId}/$/Replicas/{ReplicaId}
GlobalWString Constants::ServiceReplicaEntitySetPathViaPartitionViaNode = make_global<wstring>(COMBINE(Constants::ServicePartitionEntityKeyPathViaNode, Constants::GetReplicas));
GlobalWString Constants::ServiceReplicaEntityKeyPathViaPartitionViaNode = make_global<wstring>(*ServiceReplicaEntitySetPathViaPartitionViaNode + L"/{ReplicaId}");

GlobalWString Constants::HttpGetVerb                 = make_global<wstring>(L"GET");
GlobalWString Constants::HttpPostVerb                = make_global<wstring>(L"POST");
GlobalWString Constants::HttpDeleteVerb              = make_global<wstring>(L"DELETE");
GlobalWString Constants::HttpPutVerb                 = make_global<wstring>(L"PUT");

USHORT Constants::StatusOk                           = 200;
USHORT Constants::StatusCreated                      = 201;
USHORT Constants::StatusAccepted                     = 202;
USHORT Constants::StatusNoContent                    = 204;
USHORT Constants::StatusBadRequest                   = 400;
USHORT Constants::StatusAuthenticate                 = 401;
USHORT Constants::StatusUnauthorized                 = 403;
USHORT Constants::StatusNotFound                     = 404;
USHORT Constants::StatusConflict                     = 409;
USHORT Constants::StatusPreconditionFailed           = 412;
USHORT Constants::StatusRangeNotSatisfiable          = 416;
USHORT Constants::StatusMovedPermanently             = 301;
USHORT Constants::StatusServiceUnavailable           = 503;
GlobalWString Constants::StatusDescriptionOk         = make_global<wstring>(L"Ok");
GlobalWString Constants::StatusDescriptionCreated    = make_global<wstring>(L"Created");
GlobalWString Constants::StatusDescriptionNoContent  = make_global<wstring>(L"No Content");
GlobalWString Constants::StatusDescriptionBadRequest = make_global<wstring>(L"Bad Request");
GlobalWString Constants::StatusDescriptionAccepted   = make_global<wstring>(L"Accepted");
GlobalWString Constants::StatusDescriptionClientCertificateRequired = make_global<wstring>(L"Client certificate required");
GlobalWString Constants::StatusDescriptionClientCertificateInvalid = make_global<wstring>(L"Client certificate untrusted or invalid");
GlobalWString Constants::StatusDescriptionUnauthorized = make_global<wstring>(L"Unauthorized");
GlobalWString Constants::StatusDescriptionNotFound = make_global<wstring>(L"Not Found");
GlobalWString Constants::StatusDescriptionConflict = make_global<wstring>(L"Conflict");
GlobalWString Constants::StatusDescriptionServiceUnavailable    = make_global<wstring>(L"Service Unavailable");
GlobalWString Constants::NegotiateHeaderValue        = make_global<wstring>(L"Negotiate");
ULONG Constants::KtlAsyncDataContextTag              = 'pttH';


// Service Fabric Explorer
GlobalWString Constants::ExplorerPath = make_global<wstring>(L"/Explorer/{Path}");
GlobalWString Constants::ExplorerRootPath = make_global<wstring>(L"/Explorer/index.html");
GlobalWString Constants::ExplorerRedirectPath = make_global<wstring>(L"/Explorer");
GlobalWString Constants::ExplorerRedirectPath2 = make_global<wstring>(L"/explorer");
GlobalWString Constants::HtmlRootPath = make_global<wstring>(L"");
GlobalWString Constants::StaticContentPathArg = make_global<wstring>(L"Path");
#if defined(PLATFORM_UNIX)
GlobalWString Constants::DirectorySeparator = make_global<wstring>(L"/");
GlobalWString Constants::StaticFilesRootPath = make_global<wstring>(L"../Fabric.Data/");
GlobalWString Constants::ExplorerFilesRootPath = make_global<wstring>(L"html/Explorer/");
GlobalWString Constants::RootPageName = make_global<wstring>(L"html/ServiceFabric.html");
GlobalWString Constants::PathTraversalDisallowedString = make_global<wstring>(L"/../");
#else
GlobalWString Constants::DirectorySeparator = make_global<wstring>(L"\\");
GlobalWString Constants::StaticFilesRootPath = make_global<wstring>(L"..\\Fabric.Data\\");
GlobalWString Constants::ExplorerFilesRootPath = make_global<wstring>(L"html\\Explorer\\");
GlobalWString Constants::RootPageName = make_global<wstring>(L"html\\ServiceFabric.html");
GlobalWString Constants::PathTraversalDisallowedString = make_global<wstring>(L"\\..\\");
#endif
GlobalWString Constants::HtmlExtension = make_global<wstring>(L".html");
GlobalWString Constants::CssExtension = make_global<wstring>(L".css");
GlobalWString Constants::JavaScriptExtension = make_global<wstring>(L".js");
GlobalWString Constants::PngExtension = make_global<wstring>(L".png");
GlobalWString Constants::IcoExtension = make_global<wstring>(L".ico");
GlobalWString Constants::EotExtension = make_global<wstring>(L".eot");
GlobalWString Constants::SvgExtension = make_global<wstring>(L".svg");
GlobalWString Constants::TtfExtension = make_global<wstring>(L".ttf");
GlobalWString Constants::WoffExtension = make_global<wstring>(L".woff");
GlobalWString Constants::Woff2Extension = make_global<wstring>(L".woff2");
GlobalWString Constants::MapExtension = make_global<wstring>(L".map");
uint Constants::MaxSingleThreadedReadSize = 4 * 1024 * 1024; // Temporarily change the limit to 4 MB to unblock Service Fabric Explorer.

// /Nodes?NodeStatusFilter={nodeStatusFilter}
GlobalWString Constants::NodeStatusFilterDefaultString = make_global<wstring>(L"default");
GlobalWString Constants::NodeStatusFilterAllString = make_global<wstring>(L"all");
GlobalWString Constants::NodeStatusFilterUpString = make_global<wstring>(L"up");
GlobalWString Constants::NodeStatusFilterDownString = make_global<wstring>(L"down");
GlobalWString Constants::NodeStatusFilterEnablingString = make_global<wstring>(L"enabling");
GlobalWString Constants::NodeStatusFilterDisablingString = make_global<wstring>(L"disabling");
GlobalWString Constants::NodeStatusFilterDisabledString = make_global<wstring>(L"disabled");
GlobalWString Constants::NodeStatusFilterUnknownString = make_global<wstring>(L"unknown");
GlobalWString Constants::NodeStatusFilterRemovedString = make_global<wstring>(L"removed");

GlobalWString Constants::NodeStatusFilterDefault = make_global<wstring>(L"0"); // value of FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT
GlobalWString Constants::NodeStatusFilterAll = make_global<wstring>(L"0xFFFF");
GlobalWString Constants::NodeStatusFilterUp = make_global<wstring>(L"0x0001");
GlobalWString Constants::NodeStatusFilterDown = make_global<wstring>(L"0x0002");
GlobalWString Constants::NodeStatusFilterEnabling = make_global<wstring>(L"0x0004");
GlobalWString Constants::NodeStatusFilterDisabling = make_global<wstring>(L"0x0008");
GlobalWString Constants::NodeStatusFilterDisabled = make_global<wstring>(L"0x0010");
GlobalWString Constants::NodeStatusFilterUnknown = make_global<wstring>(L"0x0020");
GlobalWString Constants::NodeStatusFilterRemoved = make_global<wstring>(L"0x0040");

GlobalWString Constants::TestCommandTypeDataLossString = make_global<wstring>(L"DataLoss");
GlobalWString Constants::TestCommandTypeQuorumLossString = make_global<wstring>(L"QuorumLoss");
GlobalWString Constants::TestCommandTypeRestartPartitionString = make_global<wstring>(L"RestartPartition");

GlobalWString Constants::TestCommandTypeDataLoss = make_global<wstring>(L"0x0001");
GlobalWString Constants::TestCommandTypeQuorumLoss = make_global<wstring>(L"0x0002");
GlobalWString Constants::TestCommandTypeRestartPartition = make_global<wstring>(L"0x0004");

// /Networks?NetworkStatusFilter={networkStatusFilter}
GlobalWString Constants::NetworkStatusFilterDefaultString = make_global<wstring>(L"Default");
GlobalWString Constants::NetworkStatusFilterAllString = make_global<wstring>(L"All");
GlobalWString Constants::NetworkStatusFilterReadyString = make_global<wstring>(L"Ready");
GlobalWString Constants::NetworkStatusFilterCreatingString = make_global<wstring>(L"Creating");
GlobalWString Constants::NetworkStatusFilterDeletingString = make_global<wstring>(L"Deleting");
GlobalWString Constants::NetworkStatusFilterUpdatingString = make_global<wstring>(L"Updating");
GlobalWString Constants::NetworkStatusFilterFailedString = make_global<wstring>(L"Failed");

GlobalWString Constants::NetworkStatusFilterDefault = make_global<wstring>(L"0x0000");
GlobalWString Constants::NetworkStatusFilterAll = make_global<wstring>(L"0xFFFF");
GlobalWString Constants::NetworkStatusFilterReady = make_global<wstring>(L"0x0001");
GlobalWString Constants::NetworkStatusFilterCreating = make_global<wstring>(L"0x0002");
GlobalWString Constants::NetworkStatusFilterDeleting = make_global<wstring>(L"0x0004");
GlobalWString Constants::NetworkStatusFilterUpdating = make_global<wstring>(L"0x0008");
GlobalWString Constants::NetworkStatusFilterFailed = make_global<wstring>(L"0x0010");


GlobalWString Constants::PartialDataLoss = make_global<wstring>(L"PartialDataLoss");
GlobalWString Constants::FullDataLoss = make_global<wstring>(L"FullDataLoss");

GlobalWString Constants::QuorumLossReplicas = make_global<wstring>(L"QuorumLossReplicas");
GlobalWString Constants::AllReplicas = make_global<wstring>(L"AllReplicas");

GlobalWString Constants::AllReplicasOrInstances = make_global<wstring>(L"AllReplicasOrInstances");
GlobalWString Constants::OnlyActiveSecondaries = make_global<wstring>(L"OnlyActiveSecondaries");

GlobalWString Constants::NodeTransitionType = make_global<wstring>(L"NodeTransitionType");
GlobalWString Constants::StopDurationInSeconds = make_global<wstring>(L"StopDurationInSeconds");

GlobalWString Constants::State = make_global<wstring>(L"State");
GlobalWString Constants::Type = make_global<wstring>(L"Type");

GlobalWString Constants::Force = make_global<wstring>(L"Force");
GlobalWString Constants::Immediate = make_global<wstring>(L"Immediate");

GlobalWString Constants::EventsStoreHandlerPath = make_global<wstring>(L"/EventsStore/");
GlobalWString Constants::EventsStoreServiceName = make_global<wstring>(L"System/EventStoreService");
GlobalWString Constants::EventsStorePrefix = make_global<wstring>(L"EventsStore");
