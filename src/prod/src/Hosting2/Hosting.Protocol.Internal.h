// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#include "ServiceModel/ServiceModel.h"
#include "Hosting2/FabricRuntimeContext.h"
#include "Hosting2/CodePackageActivationId.h"

//
// Header file defining the protocol message contents
//
#include "Hosting2/StartRegisterApplicationHostRequest.h"
#include "Hosting2/StartRegisterApplicationHostReply.h"
#include "Hosting2/FinishRegisterApplicationHostRequest.h"
#include "Hosting2/FinishRegisterApplicationHostReply.h"
#include "Hosting2/UnregisterApplicationHostRequest.h"
#include "Hosting2/UnregisterApplicationHostReply.h"
#include "Hosting2/ApplicationHostCodePackageOperationRequest.h"
#include "Hosting2/ApplicationHostCodePackageOperationReply.h"
#include "Hosting2/CodePackageEventNotificationRequest.h"
#include "Hosting2/CodePackageEventNotificationReply.h"
#include "Hosting2/RegisterFabricRuntimeRequest.h"
#include "Hosting2/RegisterFabricRuntimeReply.h"
#include "Hosting2/UnregisterFabricRuntimeRequest.h"
#include "Hosting2/UnregisterFabricRuntimeReply.h"
#include "Hosting2/RegisterServiceTypeRequest.h"
#include "Hosting2/RegisterServiceTypeReply.h"
#include "Hosting2/InvalidLeaseReplyBody.h"
#include "Hosting2/ActivateCodePackageRequest.h"
#include "Hosting2/ActivateCodePackageReply.h"
#include "Hosting2/DeactivateCodePackageRequest.h"
#include "Hosting2/DeactivateCodePackageReply.h"
#include "Hosting2/CodePackageTerminationNotificationRequest.h"
#include "Hosting2/GetFabricProcessSidReply.h"
#include "Hosting2/UpdateCodePackageContextRequest.h"
#include "Hosting2/UpdateCodePackageContextReply.h"
#include "Hosting2/ActivateCodePackageRequest.h"
#include "Hosting2/RegisterFabricActivatorClientReply.h"
#include "Hosting2/RegisterFabricActivatorClientRequest.h"
#include "Hosting2/RegisterContainerActivatorServiceReply.h"
#include "Hosting2/RegisterContainerActivatorServiceRequest.h"
#include "Hosting2/CreateSymbolicLinkRequest.h"
#include "Hosting2/ConfigureSharedFolderACLRequest.h"
#include "Hosting2/ConfigureSMBShareRequest.h"
#include "Hosting2/ActivateProcessReply.h"
#include "Hosting2/DeactivateProcessRequest.h"
#include "Hosting2/DeactivateProcessReply.h"
#include "Hosting2/ConfigureSecurityPrincipalRequest.h"
#include "Hosting2/ConfigureSecurityPrincipalReply.h"
#include "Hosting2/ConfigureCertificateACLsRequest.h"
#include "Hosting2/ConfigureEndpointSecurityRequest.h"
#include "Hosting2/FabricHostOperationReply.h"
#include "Hosting2/CleanupSecurityPrincipalRequest.h"
#include "Hosting2/ServiceTerminatedNotification.h"
#include "Hosting2/ContainerHealthCheckStatusChangeNotification.h"
#include "Hosting2/ConfigureCrashDumpRequest.h"
#include "Hosting2/ConfigureCrashDumpReply.h"
#include "Hosting2/ConfigureContainerCertificateExportReply.h"
#include "Hosting2/CleanupApplicationPrincipalsReply.h"
#include "Hosting2/GetContainerInfoReply.h"
#include "Hosting2/GetContainerInfoRequest.h"
#include "Hosting2/GetImagesReply.h"
#if defined(PLATFORM_UNIX)
#include "Hosting2/DeleteFolderRequest.h"
#include "Hosting2/DeleteApplicationFoldersReply.h"
#endif
#include "Hosting2/ActivateHostedServiceRequest.h"
#include "Hosting2/DeactivateHostedServiceRequest.h"
#include "Hosting2/DisableNodeRequest.h"
#include "Hosting2/NodeDisabledNotification.h"
#include "Hosting2/EnableNodeRequest.h"
#include "Hosting2/NodeEnabledNotification.h"
#include "Hosting2/DownloadContainerImagesRequest.h"
#include "Hosting2/DeleteContainerImagesRequest.h"
#include "Hosting2/ConfigureNodeForDnsServiceRequest.h"
#include "Hosting2/AssignIpAddressesReply.h"
#include "Hosting2/AssignIpAddressesRequest.h"
#include "Hosting2/ActivateContainerRequest.h"
#include "Hosting2/ActivateContainerReply.h"
#include "Hosting2/DeactivateContainerRequest.h"
#include "Hosting2/ContainerOperationReply.h"
#include "Hosting2/InvokeContainerApiRequest.h"
#include "Hosting2/InvokeContainerApiReply.h"

//
// Actor and Action definitions
//

namespace Hosting2
{
    namespace Protocol
    {
        namespace Actions
        {
            std::wstring const StartRegisterApplicationHostRequest = L"StartRegisterApplicationHostRequest";
            std::wstring const FinishRegisterApplicationHostRequest = L"FinishRegisterApplicationHostRequest";
            std::wstring const RegisterFabricRuntimeRequest = L"RegisterFabricRuntimeRequest";
            std::wstring const RegisterServiceTypeRequest = L"RegisterServiceTypeRequest";
            std::wstring const RegisterServiceTypeReply = L"RegisterServiceTypeReply";
            std::wstring const UnregisterFabricRuntimeRequest = L"UnregisterFabricRuntimeRequest";
            std::wstring const UnregisterApplicationHostRequest = L"UnregisterApplicationHostRequest";
            std::wstring const InvalidateLeaseRequest = L"InvalidateLeaseRequest";
            std::wstring const ActivateCodePackageRequest = L"ActivateCodePackageRequest";
            std::wstring const DeactivateCodePackageRequest = L"DeactivateCodePackageRequest";
            std::wstring const CodePackageTerminationNotificationRequest = L"CodePackageTerminationNotificationRequest";
            std::wstring const GetFabricProcessSidRequest = L"GetFabricProcessSidRequest";
            std::wstring const UpdateCodePackageContextRequest = L"UpdateCodePackageContextRequest";
            std::wstring const CodePackageEventNotification = L"CodePackageEventNotification";

            std::wstring const ConfigureSecurityPrincipalRequest = L"ConfigureSecurityPrincipalRequest";
            std::wstring const CleanupSecurityPrincipalRequest = L"CleanupSecurityPrincipalRequest";
            std::wstring const ConfigureCrashDumpRequest = L"ConfigureCrashDumpRequest";
            std::wstring const SetupPortSecurity = L"SetupPortSecurity";
            std::wstring const CleanupPortSecurity = L"CleanupPortSecurity";
            std::wstring const SetupResourceACLs = L"SetupResourceACLs";
            std::wstring const ActivateProcessRequest = L"CreateProcessRequest";
            std::wstring const InvokeContainerApiRequest = L"InvokeContainerApiRequest";
            std::wstring const ActivateContainerRequest = L"ActivateContainerRequest";
            std::wstring const DeactivateProcessRequest = L"DeactivateProcessRequest";
            std::wstring const DeactivateContainerRequest = L"DeactivateContainerRequest";
            std::wstring const TerminateProcessRequest = L"TerminateProcessRequest";
            std::wstring const ActivateHostedServiceRequest = L"ActivateHostedServiceRequest";
            std::wstring const DeactivateHostedServiceRequest = L"DeactivateHostedServiceRequest";
            std::wstring const RemoveSecurityPrincipalConfigurationRequest = L"RemoveSecurityPrincipalRequest";
            std::wstring const ServiceTerminatedNotificationRequest =L"ServiceTerminatedNotificationRequest";
            std::wstring const ContainerEventNotificationRequest = L"ContainerEventNotificationRequest";
            std::wstring const ContainerHealthCheckStatusChangeNotificationRequest = L"ContainerHealthCheckStatusChangeNotificationRequest";
            std::wstring const RegisterFabricActivatorClient = L"RegisterFabricActivatorClient";
            std::wstring const RegisterContainerActivatorService = L"RegisterContainerActivatorService";
            std::wstring const UnregisterFabricActivatorClient = L"UnregisterFabricActivatorClient";
            std::wstring const FabricUpgradeRequest = L"FabricUpgradeRequest";
            std::wstring const CreateSymbolicLinkRequest = L"CreateSymbolicLink";
            std::wstring const ConfigureSharedFolderACL = L"ConfigureSharedFolderACL";
            std::wstring const ConfigureSMBShare = L"ConfigureSMBShare";
            std::wstring const CleanupApplicationLocalGroup = L"CleanupApplicationLocalGroup";
#if defined(PLATFORM_UNIX)
            std::wstring const DeleteApplicationFolder = L"DeleteApplicationFolder";
#endif
            std::wstring const ConfigureContainerCertificateExport = L"ConfigureContainerCertificateExport";
            std::wstring const CleanupContainerCertificateExport = L"CleanupContainerCertificateExport";
            std::wstring const DisableNodeRequest = L"DisableNodeRequest";
            std::wstring const EnableNodeRequest = L"EnableNodeRequest";
            std::wstring const NodeDisabledNotification = L"NodeDisabledNotification";
            std::wstring const NodeEnabledNotification = L"NodeEnabledNotification";
            std::wstring const DownloadContainerImages = L"DownloadContainerImages";
            std::wstring const ContainerUpdateRoutes = L"ContainerUpdateRoutes";
            std::wstring const DeleteContainerImages = L"DeleteContainerImages";
            std::wstring const GetContainerInfo = L"GetContainerInfo";
            std::wstring const ConfigureNodeForDnsService = L"ConfigureNodeForDnsService";
            std::wstring const AssignIpAddresses = L"AssignIpAddresses";
            std::wstring const ManageOverlayNetworkResources = L"ManageOverlayNetworkResources";
            std::wstring const UpdateOverlayNetworkRoutes = L"UpdateOverlayNetworkRoutes";
            std::wstring const GetOverlayNetworkDefinition = L"GetOverlayNetworkDefinition";
            std::wstring const DeleteOverlayNetworkDefinition = L"DeleteOverlayNetworkDefinition";
            std::wstring const PublishNetworkTablesRequest = L"PublishNetworkTablesRequest";
            std::wstring const SetupContainerGroup = L"SetupContainerGroup";
            std::wstring const ConfigureEndpointCertificatesAndFirewallPolicy = L"ConfigureEndpointCertificatesAndFirewallPolicy";
            std::wstring const GetImages = L"GetImages"; // Header for GetImages request from FabricActivatorClient
            std::wstring const DockerProcessTerminatedNotificationRequest = L"DockerProcessTerminatedNotificationRequest";
            std::wstring const ApplicationHostCodePackageOperationRequest = L"ApplicationHostCodePackageOperationRequest";
            std::wstring const GetNetworkDeployedCodePackages = L"GetNetworkDeployedCodePackages";
            std::wstring const GetDeployedNetworks = L"GetDeployedNetworks";
        }
    }
}