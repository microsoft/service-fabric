// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !defined(PLATFORM_UNIX)
#include <hash_set>
#include <Wincrypt.h>
#include <Http.h>
#include <sddl.h>
#include <fwpvi.h>
#include <fwpmu.h>
#include <fwpmtypes.h>
#include <Aclapi.h>
#include <Iphlpapi.h>
#include <InitGuid.h>
#include <netfw.h>
#include <wrl/client.h>
#else
#include <ext/hash_set>
#endif

//
// Header Files required by internal Runtime header files
//
#include "Management/Common/ManagementCommon.h"
#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Management/ImageStore/ImageStore.h"
#include "Management/ImageStore/ImageCacheConstants.h"
#include "Management/DnsService/config/DnsServiceConfig.h"
#include "Management/FileStoreService/FileStoreService.h"
#include "Management/BackupRestoreService/BackupRestoreServiceConfig.h"
#include "Management/ResourceMonitor/config/ResourceMonitorServiceConfig.h"
#include "Management/CentralSecretService/CentralSecretServiceConfig.h"

#if !defined(PLATFORM_UNIX)
#include "Management/HttpTransport/HttpTransport.Client.h"
#include "Management/HttpTransport/HttpTransport.Server.h"
#include "Management/HttpTransport/HttpClient.h"
#endif

#include "ServiceModel/management/ResourceMonitor/public.h"

//
// Internal Hosting header files
//

#include "Hosting2/HostingHelpers.h"
#include "Hosting2/ContainerInfoReply.h"
#include "Hosting2/HostedServiceParameters.h"
#include "Hosting2/Hosting.h"
#include "Hosting2/Hosting.Protocol.Internal.h"
#include "Hosting2/OperationStatus.h"
#include "Hosting2/OperationStatusMap.h"
#include "Hosting2/Constants.h"
#include "Hosting2/ServiceTypeStatus.h"
#include "Hosting2/ServiceTypeState.h"
#include "Hosting2/ManageOverlayNetworkAction.h"
#include "Hosting2/FabricRuntimeManager.h"
#include "Hosting2/RuntimeRegistrationTable.h"
#include "Hosting2/RuntimeRegistration.h"
#include "Hosting2/ServiceTypeStateManager.h"
#include "Hosting2/IProcessActivationContext.h"
#include "Hosting2/ContainerEventConfig.h"

#if !defined(PLATFORM_UNIX)
#include "Hosting2/ProcessConsoleRedirector.h"
#include "Hosting2/ProcessActivationContext.h"
#else
#include "Hosting2/ProcessActivationContext.Linux.h"
#include "Hosting2/ProcessConsoleRedirector.Linux.h"
#endif

#if !defined(PLATFORM_UNIX)
#include "Hosting2/ProcessActivator.h"
#include "Hosting2/ContainerApiConfig.h"
#else
#include "Hosting2/ProcessActivator.Linux.h"
#endif

#include "Management/NetworkInventoryManager/common/NIMNetworkErrorCodeResponseMessage.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkAllocationEntry.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkAllocationRequestMessage.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkAllocationResponseMessage.h"
#include "Management/NetworkInventoryManager/common/NIMPublishNetworkTablesRequestMessage.h"
#include "Management/NetworkInventoryManager/common/NIMPublishNetworkTablesRequestMessageBody.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkCreateRequestMessage.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkCreateResponseMessage.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkRemoveRequestMessage.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkEnumerateRequestMessage.h"
#include "Management/NetworkInventoryManager/common/NIMNetworkEnumerateResponseMessage.h"
#include "Hosting2/RepositoryAuthConfig.h"
#include "Hosting2/ClearContainerHelper.h"
#include "Hosting2/ContainerHostConfig.h"
#include "Hosting2/VolumeMapper.h"
#include "Hosting2/ContainerConfig.h"
#include "Hosting2/ContainerStats.h"
#include "Hosting2/IContainerActivator.h"
#include "Hosting2/ContainerActivator.h"
#include "Hosting2/ContainerEventTracker.h"
#include "Hosting2/NodeAvailableImages.h" // For deserialization json response from docker about available images
#include "Hosting2/ContainerNetworkOperations.h"
#include "Hosting2/CodePackageActivationId.h"
#include "Hosting2/CodePackageRuntimeInformation.h"
#include "Hosting2/ActivateProcessRequest.h"
#include "Hosting2/FabricUpgradeRequest.h"
#include "Hosting2/ActivatorRequestor.h"
#include "Hosting2/ApplicationHostManager.h"
#include "Hosting2/GuestServiceTypeHostManager.h"
#include "Hosting2/ApplicationHostRegistrationTable.h"
#include "Hosting2/ApplicationHostRegistrationStatus.h"
#include "Hosting2/ApplicationHostRegistration.h"
#include "Hosting2/ApplicationHostIsolationContext.h"
#include "Hosting2/ApplicationHostTerminatedEventArgs.h"
#include "Hosting2/ContainerHealthCheckStatusChangedEventArgs.h"
#include "Hosting2/ApplicationHostActivationTable.h"
#include "Hosting2/ApplicationMap.h"
#include "Hosting2/ISecurityPrincipal.h"
#include "Hosting2/SecurityUser.h"
#include "Hosting2/SecurityGroup.h"
#include "Hosting2/LocalUser.h"
#include "Hosting2/DomainUser.h"
#include "Hosting2/ServiceAccount.h"
#include "Hosting2/TokenSecurityAccount.h"
#include "Hosting2/BuiltinServiceAccount.h"
#include "Hosting2/SecurityPrincipalInformation.h"
#include "Hosting2/CertificateAccessDescription.h"
#include "Hosting2/EndpointCertificateBinding.h"
#include "Hosting2/IFabricActivatorClient.h"
#include "Hosting2/FabricActivatorClient.h"
#include "Hosting2/NodeRestartManagerClient.h"
#include "Hosting2/ApplicationPrincipals.h"
#include "Hosting2/PrincipalsProviderContext.h"
#include "Hosting2/PrincipalsProvider.h"
#include "Hosting2/PortAclRef.h"
#include "Hosting2/portcertref.h"
#include "Hosting2/PortAclMap.h"
#include "Hosting2/NodeEnvironmentManager.h"
#include "Hosting2/HttpEndpointSecurityProvider.h"
#if !defined(PLATFORM_UNIX)
#include "Hosting2/FirewallSecurityProviderHelper.h"
#endif
#include "Hosting2/FirewallSecurityProvider.h"
#include "Hosting2/SecurityIdentityMap.h"
#include "Hosting2/SecurityPrincipalsProvider.h"
#include "Hosting2/ApplicationEnvironmentContext.h"
#include "Hosting2/EnvironmentResource.h"
#include "Hosting2/EndpointResource.h"
#include "Hosting2/ServicePackageEnvironmentContext.h"
#include "Hosting2/ILogCollectionProvider.h"
#include "Hosting2/LogCollectionProviderFactory.h"
#include "Hosting2/EndpointProvider.h"
#include "Hosting2/EtwSessionProvider.h"
#include "Hosting2/CrashDumpConfigurationManager.h"
#include "Hosting2/CrashDumpProvider.h"
#include "Hosting2/EnvironmentManager.h"
#include "Hosting2/ServicePackageOperation.h"
#include "Hosting2/ServicePackageInstanceOperation.h"
#include "Hosting2/ServicePackageContext.h"
#include "Hosting2/Application.h"
#include "Hosting2/VersionedApplication.h"
#include "Hosting2/ServicePackageMap.h"
#include "Hosting2/ServicePackage.h"
#include "Hosting2/VersionedServicePackage.h"
#include "Hosting2/CodePackage.h"
#include "Hosting2/CodePackageInstance.h"
#include "Hosting2/ApplicationHostProxy.h"
#include "Hosting2/SingleCodePackageApplicationHostProxy.h"
#include "Hosting2/MultiCodePackageApplicationHostProxy.h"
#include "Hosting2/InProcessApplicationHostProxy.h"
#include "Hosting2/PendingOperationMap.h"
#include "Hosting2/ApplicationManager.h"
#include "Hosting2/Activator.h"
#include "Hosting2/Deactivator.h"
#include "Hosting2/ImageCacheManager.h"
#include "Hosting2/DownloadManager.h"
#include "Hosting2/DeletionManager.h"
#include "Hosting2/FabricUpgradeImpl.h"
#include "Hosting2/FabricUpgradeManager.h"
#include "Hosting2/HostingQueryManager.h"
#include "Hosting2/HostingTraces.h"
#include "Hosting2/EventContainer.h"
#include "Hosting2/EventDispatcher.h"
#include "Hosting2/HostingHealthManager.h"
#include "Hosting2/ServiceTypeRegistrationHelper.h"
#include "Hosting2/ConfigureEndpointBindingAndFirewallPolicyRequest.h"
#include "Hosting2/AssignIpAddressesReply.h"
#include "Hosting2/AssignIpAddressesRequest.h"
#include "Hosting2/UpdateOverlayNetworkRoutesRequest.h"
#include "Hosting2/UpdateOverlayNetworkRoutesReply.h"
#include "Hosting2/ManageOverlayNetworkResourcesRequest.h"
#include "Hosting2/ManageOverlayNetworkResourcesReply.h"
#include "Hosting2/GetNetworkDeployedPackagesRequest.h"
#include "Hosting2/GetNetworkDeployedPackagesReply.h"
#include "Hosting2/GetDeployedNetworksRequest.h"
#include "Hosting2/GetDeployedNetworksReply.h"
#include "Hosting2/ConfigureContainerCertificateExportRequest.h"
#include "Hosting2/CleanupContainerCertificateExportRequest.h"
#include "Hosting2/DockerProcessManager.h"
#include "Hosting2/TraceSessionManager.h"
#include "Hosting2/ConfigureNodeForDnsService.h"
#include "Hosting2/DnsServiceEnvironmentManager.h"
#include "Hosting2/LocalResourceManager.h"
#include "Hosting2/CertificateAclingManager.h"
#include "Hosting2/LocalSecretServiceManager.h"
#include "Hosting2/ContainerHelper.h"
#include "Hosting2/SetupConfig.h"
#include "Hosting2/NetworkInventoryAgent.h"
# include "Hosting2/SetupConfig.h"
#include "Hosting2/ContainerConfigHelper.h"
#include "Hosting2/SetupContainerGroupRequest.h"
#include "Hosting2/DockerUtilities.h"
#include "FabricContainerActivatorService_.h"
#include "Hosting2/ContainerActivatorServiceConfig.h"
#include "Hosting2/ContainerActivatorServiceAgent.h"
#include "Hosting2/ComHostingSettingsResult.h"
#include "Hosting2/ContainerLogDriverConfig.h"
#include "Hosting2/HostingHelper.h"
#include "Hosting2/DockerProcessTerminatedNotification.h"

//
// Header files required by flat network implementation
//
#include "Hosting2/ReservationManager.h"
#include "Hosting2/FlatIPConfiguration.h"
#include "Hosting2/IPAddressProvider.h"
#include "Hosting2/IPHelper.h"
#include "Hosting2/IIPAM.h"
#include "Hosting2/IPAM.h"

#if !defined(PLATFORM_UNIX)
#include "Hosting2/IPAMWindows.h"
#else
#include "Hosting2/IPAMLinux.h"
#endif

#include "FabricContainerActivatorService_.h"
#include "Hosting2/ContainerActivatorServiceConfig.h"
#include "Hosting2/ContainerActivatorServiceAgent.h"
#include "Hosting2/ComHostingSettingsResult.h"

// 
// Header files required by nat network implementation
//
#include "Hosting2/NatIpAddressProvider.h"
#include "Hosting2/INatIPAM.h"
#include "Hosting2/NatIPAM.h"
#include "Hosting2/NetworkIPAMConfig.h"
#include "Hosting2/NetworkIPAM.h"
#include "Hosting2/NetworkConfig.h"

// 
// Header files required by isolated network implementation
//
#include "MACHelper.h"
#include "Hosting2/OverlayNetworkDefinition.h"
#include "Hosting2/OverlayNetworkContainerParameters.h"
#include "Hosting2/OverlayNetworkRoute.h"
#include "Hosting2/OverlayNetworkRoutingInformation.h"
#include "Hosting2/INetworkPlugin.h"
#include "Hosting2/OverlayNetworkPlugin.h"
#include "Hosting2/OverlayNetworkDriver.h"
#include "Hosting2/OverlayNetworkManager.h"
#include "Hosting2/OverlayNetworkResource.h"
#include "Hosting2/OverlayNetworkResourceProvider.h"
#include "Hosting2/OverlayNetworkReservationManager.h"
#include "IOverlayIPAM.h"
#include "OverlayIPAM.h"
#include "Hosting2/INetworkPluginProcessManager.h"
#include "Hosting2/NetworkPluginProcessManager.h"
#include "Hosting2/OverlayNetworkPluginOperations.h"