// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

GlobalWString Constants::AdhocApplicationTypeName = make_global<wstring>(L"AdhocApplicationType");
GlobalWString Constants::AdhocApplicationId = make_global<wstring>(L"");
GlobalWString Constants::SharedApplicationFolderName = make_global<wstring>(L"shared");
GlobalWString Constants::SecurityPrincipalIdentifier = make_global<wstring>(L"WinFabPrincipal");
GlobalWString Constants::ImplicitTypeHostName = make_global<wstring>(L"FabricTypeHost.exe");
GlobalWString Constants::ImplicitTypeHostCodePackageName = make_global<wstring>(L"FabricTypeHost-DF95B56E-29EE-4418-B124-061740D7282C");
GlobalWString Constants::EndpointConfiguration_FilteringEngineProviderName = make_global<wstring>(L"fabric_filter_provider");
GlobalWString Constants::EndpointConfiguration_FilteringEngineBlockAllFilterName = make_global<wstring>(L"fabric_filter_blockall");
GlobalWString Constants::EndpointConfiguration_FilteringEnginePortFilterName = make_global<wstring>(L"fabric_filter_allowed_port");
GlobalWString Constants::CtrlCSenderExeName = make_global<wstring>(L"CtrlCSender.exe");
GlobalWString Constants::SharedFolderName = make_global<wstring>(L"__shared");
GlobalWString Constants::DebugProcessIdParameter = make_global<wstring>(L"[ProcessId]");
GlobalWString Constants::DebugThreadIdParameter = make_global<wstring>(L"[ThreadId]");
GlobalWString Constants::ContainerNetworkName = make_global<wstring>(L"servicefabric_network");
//used for hosts that should be members of windows fabric administrator group
GlobalWString Constants::WindowsFabricAdministratorsGroupAllowedUser = make_global<wstring>(L"WindowsFabricAdministratorsGroupAllowedUser_4294967295");

GlobalWString Constants::EnvironmentVariable::RuntimeConnectionAddress = make_global<wstring>(L"Fabric_RuntimeConnectionAddress");
GlobalWString Constants::EnvironmentVariable::RuntimeSslConnectionAddress = make_global<wstring>(L"Fabric_RuntimeSslConnectionAddress");
GlobalWString Constants::EnvironmentVariable::RuntimeSslConnectionCertKey= make_global<wstring>(L"Fabric_RuntimeSslConnectionCertKey");
GlobalWString Constants::EnvironmentVariable::RuntimeSslConnectionCertEncodedBytes = make_global<wstring>(L"Fabric_RuntimeSslConnectionCertEncodedBytes");
GlobalWString Constants::EnvironmentVariable::RuntimeConnectionUseSsl = make_global<wstring>(L"Fabric_RuntimeConnectionUseSsl");
GlobalWString Constants::EnvironmentVariable::RuntimeSslConnectionCertThumbprint = make_global<wstring>(L"Fabric_RuntimeSslConnectionCertThumbprint");
GlobalWString Constants::EnvironmentVariable::NodeId = make_global<wstring>(L"Fabric_NodeId");
GlobalWString Constants::EnvironmentVariable::NodeName = make_global<wstring>(L"Fabric_NodeName");
GlobalWString Constants::EnvironmentVariable::NodeIPAddressOrFQDN = make_global<wstring>(L"Fabric_NodeIPOrFQDN");
GlobalWString Constants::EnvironmentVariable::EndpointPrefix = make_global<wstring>(L"Fabric_Endpoint_");
GlobalWString Constants::EnvironmentVariable::EndpointIPAddressOrFQDNPrefix = make_global<wstring>(L"Fabric_Endpoint_IPOrFQDN_");
GlobalWString Constants::EnvironmentVariable::FoldersPrefix = make_global<wstring>(L"Fabric_Folder_");
GlobalWString Constants::EnvironmentVariable::ContainerName = make_global<wstring>(L"Fabric_ContainerName");
GlobalWString Constants::EnvironmentVariable::ServiceName = make_global<wstring>(L"Fabric_ServiceName");


StringLiteral Constants::FabricUpgrade::MSIExecCommand = "msiexec /i \"{0}\" /l*v \"{1}\" /q IACCEPTEULA=Yes";
StringLiteral Constants::FabricUpgrade::DISMExecCommand = "DISM.exe /Online /Add-Package /PackagePath:\"{0}\"  /Quiet /LogPath:\"{1}\"";
StringLiteral Constants::FabricUpgrade::DISMExecUnInstallCommand = "DISM.exe /Online /Remove-Package /PackagePath:\"{0}\"  /Quiet /LogPath:\"{1}\"";

GlobalWString Constants::FabricUpgrade::StartFabricHostServiceCommand = make_global<wstring>(L"net start FabricHostSvc");
GlobalWString Constants::FabricUpgrade::StopFabricHostServiceCommand = make_global<wstring>(L"net stop FabricHostSvc");
StringLiteral Constants::FabricUpgrade::TargetInformationXmlContent = 
    "<?xml version=\"1.0\" ?>\n"
    "<TargetInformation xmlns=\"http://schemas.microsoft.com/2011/01/fabric\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n{0}\n{1}\n</TargetInformation>";
StringLiteral Constants::FabricUpgrade::CurrentInstallationElement = "<CurrentInstallation InstanceId=\"{0}\" TargetVersion=\"{1}\" ClusterManifestLocation=\"{2}\" MSILocation=\"{3}\" NodeName=\"{4}\"/>";
StringLiteral Constants::FabricUpgrade::TargetInstallationElement = "<TargetInstallation InstanceId=\"{0}\" TargetVersion=\"{1}\" ClusterManifestLocation=\"{2}\" MSILocation=\"{3}\" NodeName=\"{4}\"/>";
StringLiteral Constants::FabricUpgrade::CurrentInstallationElementForXCopy = "<CurrentInstallation TargetVersion=\"{1}\" InstanceId=\"{0}\" ClusterManifestLocation=\"{2}\" MSILocation=\"{3}\" NodeName=\"{4}\" UpgradeEntryPointExe=\"{5}\" UpgradeEntryPointExeParameters=\"{6}\" UndoUpgradeEntryPointExe=\"{7}\" UndoUpgradeEntryPointExeParameters=\"{8}\"/>";
StringLiteral Constants::FabricUpgrade::TargetInstallationElementForXCopy = "<TargetInstallation TargetVersion=\"{1}\" InstanceId=\"{0}\" ClusterManifestLocation=\"{2}\" MSILocation=\"{3}\" NodeName=\"{4}\" UpgradeEntryPointExe=\"{5}\" UpgradeEntryPointExeParameters=\"{6}\" UndoUpgradeEntryPointExe=\"{7}\" UndoUpgradeEntryPointExeParameters=\"{8}\"/>";
GlobalWString Constants::FabricUpgrade::TargetInformationFileName = make_global<wstring>(L"TargetInformation.xml");
GlobalWString Constants::FabricUpgrade::FabricInstallerServiceDirectoryName = make_global<wstring>(L"FabricInstallerService.Code");
GlobalWString Constants::FabricUpgrade::FabricInstallerServiceExeName = make_global<wstring>(L"FabricInstallerService.exe");
GlobalWString Constants::FabricUpgrade::FabricInstallerServiceName = make_global<wstring>(L"FabricInstallerSvc");
GlobalWString Constants::FabricUpgrade::FabricInstallerServiceDisplayName = make_global<wstring>(L"Service Fabric Installer Service");
GlobalWString Constants::FabricUpgrade::FabricInstallerServiceDescription = make_global<wstring>(L"Performs reliable code upgrades for Service Fabric.");
GlobalWString Constants::FabricUpgrade::FabricWindowsUpdateContainedFile = make_global<wstring>(L"update.mum");
int const Constants::FabricUpgrade::FabricInstallerServiceDelayInSecondsBeforeRestart = 5;
int const Constants::FabricUpgrade::FabricUpgradeFailureCountResetPeriodInDays = 1;
StringLiteral Constants::FabricUpgrade::ErrorMessageFormat = "{0}, ErrorCode:{1}";
GlobalWString Constants::FabricUpgrade::LinuxPackageInstallerScriptFileName = make_global<wstring>(L"startservicefabricupdater.sh");

#if defined(PLATFORM_UNIX)   
GlobalWString Constants::FabricDeployer::ExeName = make_global<wstring>(L"FabricDeployer.sh");

// TODO: figure out why passing in quotes in the arg is causing failure in Linux. We need to support spaces in paths
StringLiteral Constants::FabricDeployer::ConfigUpgradeArguments = "/operation:Update /cm:{0} /targetVersion:{1} /instanceId:{2} /nodeName:{3} /error:{4}";
StringLiteral Constants::FabricDeployer::InstanceIdOnlyUpgradeArguments = "/operation:UpdateInstanceId /targetVersion:{0} /instanceId:{1} /nodeName:{2} /error:{3}";
StringLiteral Constants::FabricDeployer::ValidateAndAnalyzeArguments = "/operation:Validate /cm:{0} /nodeName:{1} /nodeTypeName:{2} /output:\"{3}\" /error:{4}";

const DWORD Constants::FabricDeployer::ExitCode_RestartRequired = 256;
const DWORD Constants::FabricDeployer::ExitCode_RestartNotRequired = 512;
#else
GlobalWString Constants::FabricDeployer::ExeName = make_global<wstring>(L"FabricDeployer.exe");
StringLiteral Constants::FabricDeployer::ConfigUpgradeArguments = "/operation:Update /cm:\"{0}\" /targetVersion:{1} /instanceId:{2} /nodeName:{3} /error:\"{4}\"";
StringLiteral Constants::FabricDeployer::InstanceIdOnlyUpgradeArguments = "/operation:UpdateInstanceId /targetVersion:{0} /instanceId:{1} /nodeName:{2} /error:\"{3}\"";
StringLiteral Constants::FabricDeployer::ValidateAndAnalyzeArguments = "/operation:Validate /cm:\"{0}\" /nodeName:{1} /nodeTypeName:\"{2}\" /output:\"{3}\" /error:\"{4}\"";

const DWORD Constants::FabricDeployer::ExitCode_RestartRequired = 1;
const DWORD Constants::FabricDeployer::ExitCode_RestartNotRequired = 2;
#endif 


GlobalWString Constants::FabricSetup::ExeName = make_global<wstring>(L"FabricSetup.exe");
GlobalWString Constants::FabricSetup::InstallArguments = make_global<wstring>(L"/operation:addnodestate");
GlobalWString Constants::FabricSetup::UpgradeArguments = make_global<wstring>(L"/operation:upgrade");
GlobalWString Constants::FabricSetup::UndoUpgradeArguments = make_global<wstring>(L"/operation:undoupgrade");
GlobalWString Constants::FabricSetup::RelativePathToFabricHost = make_global<wstring>(L"\\Fabric\\Fabric.Code");
TimeSpan const Constants::FabricSetup::TimeoutInterval = TimeSpan::FromMinutes(5);

//
// Container label key names/values. These names must match with key names specified
// in ConfigHelper.cs file under src/Managed/FabricCAS folder.
//
wstring const Constants::ContainerLabels::ApplicationNameLabelKeyName = L"ApplicationName";
wstring const Constants::ContainerLabels::ApplicationIdLabelKeyName = L"ApplicationId";
wstring const Constants::ContainerLabels::ServiceNameLabelKeyName = L"ServiceName";
wstring const Constants::ContainerLabels::CodePackageNameLabelKeyName = L"CodePackageName";
wstring const Constants::ContainerLabels::ServicePackageActivationIdLabelKeyName = L"ServicePackageActivationId";
wstring const Constants::ContainerLabels::PlatformLabelKeyName = L"Platform";
wstring const Constants::ContainerLabels::PlatformLabelKeyValue = L"ServiceFabric";
wstring const Constants::ContainerLabels::QueryFilterLabelKeyName = L"ServiceName_CodePackageName_PartitionId";

// In filtering engine we add block all filter with weight = 1, all the other filters which enable the port allocation have higher weight
// This enables a service to open only specific ports.
const UINT64 Constants::EndpointConfiguration_FilteringEngineBlockAllFilterWeight = 1;
// Default enum size for going thru the BFE rules
const UINT Constants::EndpointConfiguration_FilteringEngineEnumSize = 50;

const size_t Constants::ApplicationHostTraceIdMaxLength = 1024;

// When using docker we translate cores to nano cpus so that 10^9 of nano cpus = 1 core
const int64 Constants::DockerNanoCpuMultiplier = 1000000000;

wstring const Constants::ActivatorConfigFileName = L"FabricHostSettings.xml";

wstring const Constants::ConfigSectionName = L"FabricActivatorService";
wstring const Constants::PrivateObjectNamespaceAlias = L"WindowsFabricObjectPrivateNamespace-{0FC73FB6-F925-4340-A1C1-4425BCB56B31}";

wstring const Constants::FabricContainerActivatorServiceName = L"FabricContainerActivatorService";
wstring const Constants::FabricContainerActivatorServiceExeName = L"FabricCAS.exe";

wstring const Constants::WorkingDirectory = L"WorkingDirectory";

wstring const Constants::HostedServiceSectionName = L"HostedService/";
wstring const Constants::HostedServiceParamExeName = L"ExePath";
wstring const Constants::HostedServiceParamArgs = L"Arguments";
wstring const Constants::HostedServiceParamDisabled = L"Disabled";
wstring const Constants::HostedServiceParamCtrlCSenderPath = L"CtrlCSenderPath";
wstring const Constants::HostedServiceParamServiceNodeName = L"ServiceNodeName";
wstring const Constants::HostedServiceEnvironmentParam = L"Environment";
wstring const Constants::HostedServicePortParam = L"Port";
wstring const Constants::HostedServiceProtocolParam = L"Protocol";

wstring const Constants::RunasAccountName = L"RunAsAccountName";
wstring const Constants::RunasAccountType = L"RunAsAccountType";
wstring const Constants::RunasPassword = L"RunAsPassword";
wstring const Constants::HostedServiceSSLCertStoreName = L"ServerAuthX509StoreName";
wstring const Constants::HostedServiceSSLCertFindType = L"ServerAuthX509FindType";
wstring const Constants::HostedServiceSSLCertFindValue = L"ServerAuthX509FindValue";

wstring const Constants::FabricActivatorAddressEnvVariable = L"FabricActivatorAddress";

int const Constants::RestartManager::NodePollingIntervalForRepairTaskInSeconds = 10;

// 600 seconds
// There is no foolproof way to predict a good timeout value here
// as it depends on the density of apps being hosted and how long they
// take to release all the allocations in ktl after all pending IO's 
// that are completed
ULONG const Constants::Ktl::AbortTimeoutMilliseconds = 600000;

wstring const Constants::Log = L"Log";

wstring const Constants::NativeSystemServiceHostId = L"9E7B4F14-6685-4BF5-BC5B-DDCD5305EAD3";

WStringLiteral const Constants::ContainerApiResult = L"ContainerApiResult";

#if defined(PLATFORM_UNIX)
wstring const Constants::AppRunAsAccount      = L"sfappsuser";
wstring const Constants::AppRunAsAccountGroup = L"sfuser";
wstring const Constants::CgroupPrefix = L"fabric";
// When using cgroups, we will use this value to set code package period
const int32 Constants::CgroupsCpuPeriod = 100000;

string const Constants::DockerInfo = "/v1.24/info";
string const Constants::Containers = "/containers";
StringLiteral Constants::ContainersLogs = "/containers/{0}/logs?stderr=1&stdout=1&timestamps=0&tail={1}";
string const Constants::VolumesCreate = "/volumes/create";
string const Constants::ContainersCreate = "/containers/create";
string const Constants::ContainersStart = "/start";
StringLiteral Constants::ContainersStop = "/containers/{0}/stop?t={1}";
StringLiteral Constants::ContainersForceRemove = "/containers/{0}?force=1";
GlobalString const Constants::ContentTypeJson = make_global<string>("application/json");
string const Constants::DefaultContainerLogsTail = "100";

#else
// When using job objects, 10000 represents all available cores.
const int32 Constants::JobObjectCpuCyclesNumber = 10000;

wstring const Constants::DockerInfo = L"{0}/v1.24/info";
wstring const Constants::ContainersLogsUriPath = L"containers/{0}/logs?stderr=1&stdout=1&timestamps=0&tail={1}";
wstring const Constants::EventsSince = L"{0}/events?since={1}&filters={2}";
wstring const Constants::EventsSinceUntil = L"{0}/events?since={1}&until={2}&filters={3}";
wstring const Constants::EventsFilter = L"{\"type\":[\"container\"],\"event\":[\"die\",\"stop\"]}";
wstring const Constants::EventsFilterWithHealth = L"{\"type\":[\"container\"],\"event\":[\"die\",\"stop\",\"health_status\"]}";
wstring const Constants::ContainersCreate = L"{0}/containers/create?name={1}";
wstring const Constants::ContainersInspect = L"{0}/containers/{1}/json";
wstring const Constants::ContainersExec = L"{0}/containers/{1}/exec";
wstring const Constants::ContainersStart = L"{0}/containers/{1}/start";
wstring const Constants::ContainersStop = L"{0}/containers/{1}/stop?t={2}";
wstring const Constants::ContainersForceRemove = L"{0}/containers/{1}?force=1";
wstring const Constants::VolumesCreate = L"{0}/volumes/create";
wstring const Constants::NetworkNatConnect = L"{0}/networks/nat/connect";
wstring const Constants::ExecStart = L"{0}/exec/{1}/start";
GlobalWString const Constants::ContentTypeJson = make_global<wstring>(L"application/json");
wstring const Constants::DefaultContainerLogsTail = L"100";
#endif
