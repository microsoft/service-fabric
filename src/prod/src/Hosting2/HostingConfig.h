// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostingConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(HostingConfig, "Hosting")

        // ---------------   fabricruntime to fabric communication

        //The timeout value for the sync FabricCreateRuntime call
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", CreateFabricRuntimeTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The timeout value for the sync Register(Stateless/Stateful)ServiceFactory call
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ServiceFactoryRegistrationTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The timeout value for the FabricRegisterCodePackageHost sync call. This is applicable for only multi code package application hosts like FWP
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", RegisterCodePackageHostTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The timeout value for the CodePackageActivationContext calls . This is not applicable to ad-hoc services.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", GetCodePackageActivationContextTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //When Fabric exit is detected in a self activated processes, FabricRuntime closes all of the replicas in the user's host (applicationhost) process. This is the timeout for the that close operation.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ApplicationHostCloseTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //This represents the timeout for communication between the user's application host and Fabric process for various hosting related operations such as factory registration, runtime registration.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", RequestTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        
        // ----------------- deployment settings

        //Back-off interval for the deployment failure. On every continuous deployment failure the system will retry the deployment for up to the MaxDeploymentFailureCount. The retry interval is a product of continuous deployment failure and the deployment backoff interval.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeploymentRetryBackoffInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Max retry interval for the deployment. On every continuous failure the retry interval is calculated as Min( DeploymentMaxRetryInterval, Continuous Failure Count * DeploymentRetryBackoffInterval)
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeploymentMaxRetryInterval, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Application deployment will be retried for DeploymentMaxFailureCount times before failing the deployment of that application on the node.
        PUBLIC_CONFIG_ENTRY(int, L"Hosting", DeploymentMaxFailureCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        //The retry interval for trying to create file store service users of the creation failed during DownloadManager open
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", FileStoreServiceUserCreationRetryTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The timeout allowed for transitioning from a state to another in the application principals state machine
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ApplicationPrincipalsTransitionTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // On every continuous failure system will generate a randomized time between 0 to DeploymentFailedRetryIntervalRange. Retry backoff interval will be Random(0, DeploymentFailedRetryIntervalRange) + failureCount*DeploymentRetryBackoffInterval.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeploymentFailedRetryIntervalRange, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Keep retrying on the Internal failure
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", RetryOnInternalError, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        //Max Retry on Internal Errors
        INTERNAL_CONFIG_ENTRY(uint, L"Hosting", MaxRetryOnInternalError, 15, Common::ConfigEntryUpgradePolicy::Dynamic);

        // ---------------activation settings

        //The timeout for application activation, deactivation and upgrade.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ActivationTimeout, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Backoff interval on every activation failure,On every continuous activation failure the system will retry the activation for up to the MaxActivationFailureCount. The retry interval on every try is a product of continuous activation failure and the activation back-off interval.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ActivationRetryBackoffInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Max retry interval for Activation. On every continuous failure the retry interval is calculated as Min( ActivationMaxRetryInterval, Continuous Failure Count * ActivationRetryBackoffInterval)
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ActivationMaxRetryInterval, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Dynamic);
        //This is the maximum count for which system will retry failed activation before giving up. 
        PUBLIC_CONFIG_ENTRY(int, L"Hosting", ActivationMaxFailureCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        //The activated process is created in the background without any console.
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", EnableActivateNoWindow, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        //If false the activated process will honor the JIT debugging policies of the machine, otherwise the activated process will simply die and restarted as per the retry intervals
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", ActivatedProcessDieOnUnhandledExceptionEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        //Enables running code packages as local user other than the user under which fabric process is running. In order to enable this policy Fabric must be running as SYSTEM or as user who has SeAssignPrimaryTokenPrivilege.
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", RunAsPolicyEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        //Enables support for using NTLM by the code packages that are running as other users so that the processes across machines can communicate securely.
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", NTLMAuthenticationEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        //Is an encrypted has that is used to generate the password for NTLM users. Has to be set if NTLMAuthenticationEnabled is true. Validated by the deployer.
        PUBLIC_CONFIG_ENTRY(Common::SecureString, L"Hosting", NTLMAuthenticationPasswordSecret, Common::SecureString(L""), Common::ConfigEntryUpgradePolicy::Static);
        //Enables management of Endpoint resources by Fabric. Requires specification of start and end application port range in FabricNode.
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", EndpointProviderEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        //Enables management of IP addresses.
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", IPProviderEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
		//How often hosting should attempt to find and acl new cluster certificates
		INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ClusterCertificateAclingInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

#if defined(PLATFORM_UNIX)
        //The name of the plugin used for flat networks
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", AzureVnetPluginName, L"azure-cns", Common::ConfigEntryUpgradePolicy::Static);
        //The timeout for initializing the azure vnet plugin process manager
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", AzureVnetPluginProcessManagerInitTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The full path of the sock file created for the flat network plugin
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", AzureVnetPluginSockPath, L"/run/docker/plugins/azure-vnet.sock", Common::ConfigEntryUpgradePolicy::Static);
        //This is the maximum count for which the system will retry azure vnet plugin activation before giving up. 
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", AzureVnetPluginActivationMaxFailureCount, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        //This is the interval during which if we reach max retry activation count, we will assert. 
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", AzureVnetPluginActivationExceptionInterval, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Backoff interval on every retry to start azure vnet plugin.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", AzureVnetPluginActivationRetryBackoffInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);
#endif

        //Enables opening firewall ports for Endpoint resources with explicit ports specified in ServiceManifest
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", FirewallPolicyEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        //The timeout for the ServiceType to register with fabric
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ServiceTypeRegistrationTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Enables launching application hosts under debugger
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", EnableProcessDebugging, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        //The timeout to reset the continuous exit failure count for code package
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", CodePackageContinuousExitFailureResetInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The CodePackage failure count after which warning is reported
        INTERNAL_CONFIG_ENTRY(uint, L"Hosting", CodePackageHealthWarningThreshold, 1, Common::ConfigEntryUpgradePolicy::Dynamic);
        //The CodePackage failure count after which error is reported
        INTERNAL_CONFIG_ENTRY(uint, L"Hosting", CodePackageHealthErrorThreshold, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
        //The maximum number of parallel threads allowed to process Hosting queries
        INTERNAL_CONFIG_ENTRY(uint, L"Hosting", MaxQueryOperationThreads, 15, Common::ConfigEntryUpgradePolicy::Static);
        //The timeout for FileStoreService ServicePackage activation.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", FSSActivationTimeout, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);

        TEST_CONFIG_ENTRY(bool, L"Hosting", InteractiveRunAsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

        //-----------------  deactivation settings

        //Interval in secs - the scan for deactivation is performed
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeactivationScanInterval, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Grace interval after which the process can be marked for deactivation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeactivationGraceInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        //On every continuous failure system will generate a randomized time between 0 to DeactivationFailedRetryIntervalRange. Retry backoff interval will be Random(0, DeactivationFailedRetryIntervalRange) + failureCount*DeactivationGraceInterval.
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeactivationFailedRetryIntervalRange, Common::TimeSpan::FromSeconds(90), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Max retry interval for Deactivation. On every continuous failure if the retry interval is more than the backoff interval, we set the backoff interval as DeactivationMaxRetryInterval.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeactivationMaxRetryInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // -------------- block list settings

        //Time interval after which the service type can be disabled
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ServiceTypeDisableGraceInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        //When an activation or deployment is failed on a node, the system can try a different node to increase the availability. However many errors are recoverable and the failures can be resolved using simple retry. This knob controls the threshold for the failure count after which FM is notified to disable the service type on that node and try a different node for placement.
        PUBLIC_CONFIG_ENTRY(int, L"Hosting", ServiceTypeDisableFailureThreshold, 1, Common::ConfigEntryUpgradePolicy::Dynamic);
        //Recovers a disabled service type for adhoc applications after the specified time interval
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DisabledServiceTypeRecoveryInterval, Common::TimeSpan::MaxValue, Common::ConfigEntryUpgradePolicy::Dynamic);

        // -------------- Environment specific settings
        // The periodic interval at which Hosting scans for new certificates to be used for FileStoreService NTLM configuration.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", NTLMSecurityUsersByX509CommonNamesRefreshInterval, Common::TimeSpan::FromMinutes(3), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The timeout for configuring NTLM users using certificate common names. The NTLM users are needed for FileStoreService shares.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", NTLMSecurityUsersByX509CommonNamesRefreshTimeout, Common::TimeSpan::FromMinutes(4), Common::ConfigEntryUpgradePolicy::Dynamic);

        // -------------- Cache Cleanup settings
        // Time interval for cleaning up unused content in ApplicationInstance, ImageCache and Shared folders
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", CacheCleanupScanInterval, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Static);
        //Setting to enable or disable pruning of container images when application type is unregistered
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", PruneContainerImages, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        //list of container image names that we should skip deleting. List is | delimited. If no tag is specified images with all tags will be skipped
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerImagesToSkip, L"microsoft/windowsservercore|microsoft/nanoserver", Common::ConfigEntryUpgradePolicy::Static);

        // Backoff internval on failure during Cache Cleanup
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", CacheCleanupBackoffInternval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Maximum number of continous failures before giving up on CacheCleanup
        INTERNAL_CONFIG_ENTRY(uint, L"Hosting", CacheCleanupMaxContinuousFailures, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

        // log collector settings
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", AzureLogCollectorConfigurationTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // crash dump configuration settings
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", CrashDumpConfigurationTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // dllhost settings
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", DllHostExePath, L"FWP.exe", Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", FabricUpgradeTimeout, Common::TimeSpan::FromSeconds(1800), Common::ConfigEntryUpgradePolicy::Dynamic);

        TEST_CONFIG_ENTRY(std::wstring, L"Hosting", FabricTypeHostPath, L"FabricTypeHost.exe", Common::ConfigEntryUpgradePolicy::Static);

        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", FabricContainerAppsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerProviderServiceName, L"docker", Common::ConfigEntryUpgradePolicy::Dynamic);

        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerProviderProcessName, L"dockerd", Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerHostAddress, L"", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerHostPort, L"2375", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerImageDownloadTimeout, Common::TimeSpan::FromSeconds(1200), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ContainerImageDownloadRetryCount, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerImageDownloadBackoff, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerDeactivationTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerAppDeploymentRootFolder, L"C:\\SFApplications", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerTerminationTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerPackageRootFolder, L"C:\\SFPackageRoot", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerFabricBinRootFolder, L"C:\\SFFabricBin", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DockerRequestTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ContainerTerminationMaxRetryCount, 5, Common::ConfigEntryUpgradePolicy::Dynamic);
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerDeactivationRetryDelayInMilliseconds, Common::TimeSpan::FromMilliseconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", SkipDockerProcessManagement, false, Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", MaxContainerOperations, 100, Common::ConfigEntryUpgradePolicy::Static);
        DEPRECATED_CONFIG_ENTRY(int, L"Hosting", ContainerDeactivationRetryDelayInSec, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

        //Config to determine the time to trace number of active Containers and Exe
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", InitialStatisticsInterval, Common::TimeSpan::FromMinutes(20), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", StatisticsInterval, Common::TimeSpan::FromMinutes(300), Common::ConfigEntryUpgradePolicy::Static);


#if defined(PLATFORM_UNIX)
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerGroupRootImageName, L"ubuntu:16.04", Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerGroupEntrypoint, L"", Common::ConfigEntryUpgradePolicy::Static);
        //The primary directory of external executable commands on the node.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", LinuxExternalExecutablePath, L"/usr/bin/", Common::ConfigEntryUpgradePolicy::Static);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerGroupRootImageName, L"microsoft/nanoserver:latest", Common::ConfigEntryUpgradePolicy::Static);
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerGroupEntrypoint, L"powershell.exe", Common::ConfigEntryUpgradePolicy::Static);
#endif
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerDeactivationQueueTimeount, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", ContainerImageCachingEnabled, false, Common::ConfigEntryUpgradePolicy::Static);

        //Allow containers to be run as privileged. Disallowed by default.
        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", EnablePrivilegedContainers, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerFabricLogRootFolder, L"C:\\SFFabricLog", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerNodeWorkFolder, L"C:\\SFNodeFolder", Common::ConfigEntryUpgradePolicy::Static);

        // Setting this to true will include guest service type host in deployed code package query
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", IncludeGuestServiceTypeHostInCodePackageQuery, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Setting this to false will host the guest type in old way by bringing up FabricTypeHost.exe
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", HostGuestServiceTypeInProc, true, Common::ConfigEntryUpgradePolicy::Static);

        // -------------- diagnostics settings

        //Max file size for application ETW traces
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ApplicationEtwTraceFileSizeInMB, 128, Common::ConfigEntryUpgradePolicy::Static);

        //Verbosity for application ETW traces. Default is 4 (info).
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ApplicationEtwTraceLevel, 4, Common::ConfigEntryUpgradePolicy::Static);

        // Interval at which we notify the DCA about active service packages
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DcaNotificationIntervalInSeconds, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);

        //Fabric to FabricHost communication settings
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", FabricHostCommunicationTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

        //--------------- ipc health reporting settings
        //Enables health reporting from within the applicationhost
        INTERNAL_CONFIG_ENTRY(bool, L"Hosting", IsApplicationHostHealthReportingEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", DeployFabricSystemPackageRetryBackoff, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        //--------------- test setting to fail upgrade application
        TEST_CONFIG_ENTRY(bool, L"Hosting", AbortSwitchServicePackage, false, Common::ConfigEntryUpgradePolicy::Static);

        TEST_CONFIG_ENTRY(bool, L"Hosting", AbortSwitchApplicationPackage, false, Common::ConfigEntryUpgradePolicy::Static);

        //The timeout for application upgrade. If the timeout is less than the "ActivationTimeout" deployer will fail.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ApplicationUpgradeTimeout, Common::TimeSpan::FromSeconds(360), Common::ConfigEntryUpgradePolicy::Dynamic);

        PUBLIC_CONFIG_ENTRY(bool, L"Hosting", EnableDockerHealthCheckIntegration, true, Common::ConfigEntryUpgradePolicy::Static);

        // Test mode for LocalResourceManager (will not check available resources, and will not govern anything actually).
        // This is used to test LRM <-> FM <-> CRM flows in FabricTest.
        TEST_CONFIG_ENTRY(bool, L"Hosting", LocalResourceManagerTestMode, false, Common::ConfigEntryUpgradePolicy::Static);

        // Max number of continuous failure for ContainerEventManager trying to register for docker events after which it will assert.
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ContainerEventManagerMaxContinuousFailure, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ContainerEventManagerFailureBackoffInSec, 5, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Used by DockerProcessManager trying to start docker service after which it will assert.
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", DockerProcessManagerMaxRetryCount, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", DockerProcessManagerRetryIntervalInSec, 2, Common::ConfigEntryUpgradePolicy::Dynamic);

#if !defined(PLATFORM_UNIX)
        //
        // Service Fabric (SF) manages docker daemon (except windows client machines like Win10). This configuration allows
        // user to specify endpoint on which docker daemon should listen on when started by SF. By default, SF will start
        // docker daemon to listen both on http and named pipe protocols.
        //
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerServiceAddressArgument, L"", Common::ConfigEntryUpgradePolicy::Static);

        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerServiceNamedPipeOrUnixSocketAddress, L"npipe://./pipe/docker_engine", Common::ConfigEntryUpgradePolicy::Static);
#else
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Hosting", ContainerServiceNamedPipeOrUnixSocketAddress, L"unix:///var/run/docker.sock", Common::ConfigEntryUpgradePolicy::Static);
#endif

        // Used by ContainerActivator for windows to retry sending the ipc message when FabricCAS is temporarily not available.
        INTERNAL_CONFIG_ENTRY(int, L"Hosting", ContainerActivatorMessageRetryDelayInSec, 1, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Currently when ContainerActivator opens, it starts container service process. When FabricTests runs in parallel on one machine
        // it may cause multiple container service process to start in parallel. For FabricTest this config is set to true by default.
        // Those FabricTest which uses container need to set it to false and run sequentially.
        TEST_CONFIG_ENTRY(bool, L"Hosting", DisableContainerServiceStartOnContainerActivatorOpen, false, Common::ConfigEntryUpgradePolicy::Static);

        // Amount of time we wait for a container stats response
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Hosting", ContainerStatsTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

    public:
        std::wstring GetContainerApplicationFolder(std::wstring const& applicationId)
        {
            return Common::Path::Combine(this->ContainerAppDeploymentRootFolder, applicationId);
        }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_HOSTING_SETTINGS & settings) const;
    };
}
