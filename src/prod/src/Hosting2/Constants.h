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
        static Common::GlobalWString SharedApplicationFolderName;
        static Common::GlobalWString SecurityPrincipalIdentifier;
        static Common::GlobalWString ImplicitTypeHostName;
        static Common::GlobalWString ImplicitTypeHostCodePackageName;
        static Common::GlobalWString EndpointConfiguration_FilteringEngineProviderName;
        static Common::GlobalWString EndpointConfiguration_FilteringEngineBlockAllFilterName;
        static Common::GlobalWString EndpointConfiguration_FilteringEnginePortFilterName;
        static Common::GlobalWString CtrlCSenderExeName;
        static Common::GlobalWString SharedFolderName;
        static Common::GlobalWString WindowsFabricAdministratorsGroupAllowedUser;
        
        static Common::GlobalWString DebugProcessIdParameter;
        static Common::GlobalWString DebugThreadIdParameter;

        static Common::GlobalWString ContainerNetworkName;

        static UINT64 const EndpointConfiguration_FilteringEngineBlockAllFilterWeight;
        static UINT const EndpointConfiguration_FilteringEngineEnumSize;
        static size_t const ApplicationHostTraceIdMaxLength;

        static int64 const DockerNanoCpuMultiplier;

        static const Common::WStringLiteral ContainerApiResult; 

        class EnvironmentVariable
        {
        public:
            static Common::GlobalWString RuntimeConnectionAddress;
            static Common::GlobalWString RuntimeSslConnectionAddress;
            static Common::GlobalWString RuntimeSslConnectionCertKey;
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

            static Common::GlobalWString LinuxPackageInstallerScriptFileName;

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
            static std::wstring const ServiceNameLabelKeyName;
            static std::wstring const CodePackageNameLabelKeyName;
            static std::wstring const ServicePackageActivationIdLabelKeyName;
            static std::wstring const PlatformLabelKeyName;
            static std::wstring const PlatformLabelKeyValue;
            static std::wstring const QueryFilterLabelKeyName;
        };

        //FabricActivator constants
        static std::wstring const ActivatorConfigFileName;
        static std::wstring const ConfigSectionName;
        static std::wstring const PrivateObjectNamespaceAlias;

        static std::wstring const FabricContainerActivatorServiceName;
        static std::wstring const FabricContainerActivatorServiceExeName;

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
                

        static Common::GlobalStringT const ContentTypeJson;

#if defined(PLATFORM_UNIX)
        static std::wstring const AppRunAsAccount;
        static std::wstring const AppRunAsAccountGroup;
        static std::wstring const CgroupPrefix;
        static int32 const CgroupsCpuPeriod;
        
        // Docker REST URI format constants
        static std::string const Containers;
        static std::string const DockerInfo;
        static Common::StringLiteral ContainersLogs;
        static std::string const ContainersCreate;
        static std::string const ContainersStart;
        static Common::StringLiteral ContainersStop;
        static Common::StringLiteral ContainersForceRemove;
        static std::string const VolumesCreate;
        static std::string const DefaultContainerLogsTail;
#else
        static int32 const JobObjectCpuCyclesNumber;
        
        // Docker REST URI format constants
        static std::wstring const DockerInfo;
        static std::wstring const ContainersLogsUriPath;
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
        static std::wstring const DefaultContainerLogsTail;
#endif
    };
};
