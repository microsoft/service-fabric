// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class Constants
    {
    public:
        static Common::GlobalWString AdhocApplicationId;
        static Common::GlobalWString AdhocApplicationTypeName;
        static Common::GlobalWString DockerProcessFile;
        static Common::GlobalWString DockerProcessIdFileDirectory;
        static Common::GlobalWString DockerLogDirectory;
        static Common::GlobalWString SharedApplicationFolderName;
        static Common::GlobalWString SecurityPrincipalIdentifier;
        static Common::GlobalWString ImplicitTypeHostName;
        static Common::GlobalWString ImplicitTypeHostCodePackageName;
        static Common::GlobalWString BlockStoreServiceCodePackageName;
        static Common::GlobalWString EndpointConfiguration_FilteringEngineProviderName;
        static Common::GlobalWString EndpointConfiguration_FilteringEngineBlockAllFilterName;
        static Common::GlobalWString EndpointConfiguration_FilteringEnginePortFilterName;
        static Common::GlobalWString CtrlCSenderExeName;
        static Common::GlobalWString SharedFolderName;
        static Common::GlobalWString WindowsFabricAdministratorsGroupAllowedUser;
        static Common::GlobalWString DebugProcessIdParameter;
        static Common::GlobalWString DebugThreadIdParameter;
        static Common::GlobalWString DockerTempDirName;

        static UINT64 const EndpointConfiguration_FilteringEngineBlockAllFilterWeight;
        static UINT const EndpointConfiguration_FilteringEngineEnumSize;
        static size_t const ApplicationHostTraceIdMaxLength;
        static int64 const DockerNanoCpuMultiplier;

        static const Common::WStringLiteral ContainerApiResult;

        static Common::GlobalWString WellKnownValueDelimiter;
        static Common::GlobalWString WellKnownPartitionIdFormat;
        static Common::GlobalWString WellKnownServiceNameFormat;
        static Common::GlobalWString WellKnownApplicationNameFormat;

        static Common::GlobalWString SecretsStoreRef;
        static Common::GlobalWString Encrypted;

        static Common::GlobalWString FileStoreServiceUserGroup;

        class EnvironmentVariable
        {
        public:
            static Common::GlobalWString RuntimeConnectionAddress;
            static Common::GlobalWString RuntimeSslConnectionAddress;
            static Common::GlobalWString RuntimeSslConnectionCertKey;
            static Common::GlobalWString RuntimeSslConnectionCertKeyFilePath;
            static Common::GlobalWString RuntimeSslConnectionCertFilePath;
            static Common::GlobalWString RuntimeSslConnectionCertEncodedBytes;
            static Common::GlobalWString RuntimeSslConnectionCertThumbprint;
            static Common::GlobalWString RuntimeConnectionUseSsl;
            static Common::GlobalWString NodeId;
            static Common::GlobalWString NodeName;
            static Common::GlobalWString NodeIPAddressOrFQDN;
            static Common::GlobalWString EndpointPrefix;
            static Common::GlobalWString EndpointIPAddressOrFQDNPrefix;
            static Common::GlobalWString FoldersPrefix;
            static Common::GlobalWString ContainerName;
            static Common::GlobalWString ServiceName;
            static Common::GlobalWString PartitionId;
            static Common::GlobalWString TempDir;
            static Common::GlobalWString DockerTempDir;
            static Common::GlobalWString Epoch;
            static Common::GlobalWString ReplicaName;
        };

        class FabricDeployer
        {
        public:
            static Common::GlobalWString ExeName;
            static Common::StringLiteral ConfigUpgradeArguments;
            static Common::StringLiteral InstanceIdOnlyUpgradeArguments;
            static Common::StringLiteral ValidateAndAnalyzeArguments;
            static DWORD const ExitCode_RestartRequired;
            static DWORD const ExitCode_RestartNotRequired;
        };

        class FabricSetup
        {
        public:
            static Common::GlobalWString ExeName; 
            static Common::GlobalWString InstallArguments;
            static Common::GlobalWString UpgradeArguments;
            static Common::GlobalWString UndoUpgradeArguments;
            static Common::GlobalWString RelativePathToFabricHost;
            static Common::TimeSpan const TimeoutInterval;
        };

        class FabricUpgrade
        {
        public:
            static Common::StringLiteral MSIExecCommand;
            static Common::StringLiteral DISMExecCommand;
            static Common::StringLiteral DISMExecUnInstallCommand;
            static Common::GlobalWString StartFabricHostServiceCommand;
            static Common::GlobalWString StopFabricHostServiceCommand;
            static Common::StringLiteral TargetInformationXmlContent;
            static Common::StringLiteral CurrentInstallationElement;
            static Common::StringLiteral TargetInstallationElement;
            static Common::StringLiteral CurrentInstallationElementForXCopy;
            static Common::StringLiteral TargetInstallationElementForXCopy;
            static Common::GlobalWString TargetInformationFileName;

#if defined(PLATFORM_UNIX)
            static Common::GlobalWString LinuxPackageInstallerScriptFileName;
            static Common::GlobalWString LinuxUpgradeScriptFileName;
            static Common::GlobalWString LinuxUpgradeScriptMetaFileNameFormat;
            static DWORD const LinuxUpgradeScript_RollbackCutoffVersion;
            static Common::GlobalWString LinuxUpgradeMetadataDirectoryName;
            static Common::GlobalString MetaPayloadDefaultExtractPath;
#endif

            static Common::GlobalWString FabricInstallerServiceDirectoryName;
            static Common::GlobalWString FabricInstallerServiceExeName;
            static Common::GlobalWString FabricInstallerServiceName;
            static Common::GlobalWString FabricInstallerServiceDisplayName;
            static Common::GlobalWString FabricInstallerServiceDescription;
            static Common::GlobalWString FabricWindowsUpdateContainedFile;
            static const int FabricInstallerServiceDelayInSecondsBeforeRestart;
            static const int FabricUpgradeFailureCountResetPeriodInDays;
            static Common::StringLiteral ErrorMessageFormat;
        };

        class RestartManager
        {
        public:
            static const int NodePollingIntervalForRepairTaskInSeconds;
        };

        class ContainerLabels
        {
        public:
            static std::wstring const ApplicationNameLabelKeyName;
            static std::wstring const ApplicationIdLabelKeyName;
            static std::wstring const DigestedApplicationNameLabelKeyName; // used for container log paths
            static std::wstring const ServiceNameLabelKeyName;
            static std::wstring const SimpleApplicationNameLabelKeyName;
            static std::wstring const CodePackageNameLabelKeyName;
            static std::wstring const CodePackageInstanceLabelKeyName;
            static std::wstring const PartitionIdLabelKeyName;
            static std::wstring const ServicePackageActivationIdLabelKeyName;
            static std::wstring const PlatformLabelKeyName;
            static std::wstring const PlatformLabelKeyValue;
            static std::wstring const LogBasePathKeyName;
            static std::wstring const SrppQueryFilterLabelKeyName;
            static std::wstring const MrppQueryFilterLabelKeyName;
        };

        class NetworkSetup
        {
        public:
#if defined(PLATFORM_UNIX)
            static std::string const OverlayNetworkBaseUrl;
            static std::string const OverlayNetworkUpdateRoutes;
            static std::string const OverlayNetworkAttachContainer;
            static std::string const OverlayNetworkDetachContainer;
#else
            static std::wstring const OverlayNetworkUpdateRoutes;
            static std::wstring const OverlayNetworkAttachContainer;
            static std::wstring const OverlayNetworkDetachContainer;
#endif
        };

        //FabricActivator constants
        static std::wstring const ActivatorConfigFileName;
        static std::wstring const ConfigSectionName;
        static std::wstring const PrivateObjectNamespaceAlias;
        static std::wstring const FabricContainerActivatorServiceName;
        static std::wstring const FabricContainerActivatorServiceExeName;

        static std::wstring const FabricServiceName;
        static std::wstring const WorkingDirectory;

        //Hosted Service Parameters
        static std::wstring const HostedServiceSectionName;
        static std::wstring const HostedServiceParamExeName;
        static std::wstring const HostedServiceParamArgs;
        static std::wstring const HostedServiceParamDisabled;
        static std::wstring const HostedServiceParamCtrlCSenderPath;
        static std::wstring const HostedServiceParamServiceNodeName;
        static std::wstring const HostedServiceEnvironmentParam;
        static std::wstring const HostedServicePortParam;
        static std::wstring const HostedServiceProtocolParam;
        static std::wstring const HostedServiceSSLCertStoreName;
        static std::wstring const HostedServiceSSLCertFindType;
        static std::wstring const HostedServiceSSLCertFindValue;

        static std::wstring const HostedServiceRgPolicies;
        static std::wstring const HostedServiceCpusetCpus;
        static std::wstring const HostedServiceCpuShares;
        static std::wstring const HostedServiceMemoryInMB;
        static std::wstring const HostedServiceMemorySwapInMB;

        static std::wstring const RunasAccountName;
        static std::wstring const RunasAccountType;
        static std::wstring const RunasPassword;
        static std::wstring const FabricActivatorAddressEnvVariable;

        //Application Service Parameters
        static std::wstring const Log;

        // Defines the host id for system services hosted within fabric.exe
        static std::wstring const NativeSystemServiceHostId;

        class Ktl
        {
        public:
            static const ULONG AbortTimeoutMilliseconds;
        };

        static Common::GlobalWString HttpContentTypeJson;
        static Common::GlobalWString HttpGetVerb;
        static Common::GlobalWString HttpPostVerb;
        static Common::GlobalWString HttpPutVerb;
        static Common::GlobalWString HttpDeleteVerb;
        static Common::GlobalWString HttpHeadVerb;
        static Common::GlobalWString HttpOptionsVerb;
        static Common::GlobalWString HttpTraceVerb;
        static Common::GlobalWString HttpConnectVerb;
        static std::wstring const ContainersLogsUriPath;
        static std::string const ContentTypeJson;
        static std::wstring const DefaultContainerLogsTail;

        static std::wstring const ContainerLogDriverOptionLogBasePathKey;

        static std::wstring const ContainerLogRemovalMarkerFile;
        static std::wstring const ContainerLogProcessMarkerFile;

#if defined(PLATFORM_UNIX)
        static std::wstring const AppRunAsAccount;
        static std::wstring const AppRunAsAccountGroup;
        static std::wstring const CgroupPrefix;
        static int32 const CgroupsCpuPeriod;

        static std::wstring const CrioCgroupName;

        // Docker REST URI format constants
        static std::string const Containers;
        static std::string const DockerInfo;
        static std::string const ContainersCreate;
        static std::string const ContainersStart;
        static Common::StringLiteral ContainersStop;
        static Common::StringLiteral ContainersForceRemove;
        static std::string const VolumesCreate;
#else
        static int32 const JobObjectCpuCyclesNumber;

        // Docker REST URI format constants
        static std::wstring const DockerInfo;
        static std::wstring const ContainersCreate;
        static std::wstring const ContainersStart;
        static std::wstring const ContainersStop;
        static std::wstring const ContainersForceRemove;
        static std::wstring const VolumesCreate;
        static std::wstring const EventsSince;
        static std::wstring const EventsSinceUntil;
        static std::wstring const EventsFilter;
        static std::wstring const EventsFilterWithHealth;
        static std::wstring const ContainersInspect;
        static std::wstring const ContainersExec;
        static std::wstring const NetworkNatConnect;
        static std::wstring const ExecStart;
#endif
    };
};
