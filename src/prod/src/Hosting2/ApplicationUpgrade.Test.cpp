// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Hosting2/FabricNodeHost.Test.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace Management;
using namespace ImageModel;

const StringLiteral TraceType("ApplicationUpgradeTest");

class DummyFabricActivatorClient :
    public IFabricActivatorClient
{
public:

    DummyFabricActivatorClient()
        : abortOpenServicePackage(false)
        , abortCloseApplicationPackage(false)
        , abortOpenApplicationPackage(false)
    {
    }

    AsyncOperationSPtr BeginActivateProcess(
        wstring const & applicationId,
        wstring const & appServiceId,
        ProcessDescriptionUPtr const & processDescription,
        wstring const & runasUserId,
        bool isContainerHost,
        ContainerDescriptionUPtr && containerDescription,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(applicationId);
        UNREFERENCED_PARAMETER(appServiceId);
        UNREFERENCED_PARAMETER(processDescription);
        UNREFERENCED_PARAMETER(runasUserId);
        UNREFERENCED_PARAMETER(isContainerHost);
        UNREFERENCED_PARAMETER(containerDescription);
        UNREFERENCED_PARAMETER(timeout);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndActivateProcess(AsyncOperationSPtr const & operation, __out DWORD & processId)
    {
        UNREFERENCED_PARAMETER(processId);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginDeactivateProcess(
        wstring const & appServiceId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(appServiceId);
        UNREFERENCED_PARAMETER(timeout);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndDeactivateProcess(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginActivateHostedService(
        HostedServiceParameters const & params,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(params);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndActivateHostedService(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginDeactivateHostedService(
        wstring const & serviceName,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(serviceName);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndDeactivateHostedService(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureSecurityPrincipalsForNode(
        wstring const & applicationId,
        ULONG applicationPackageCounter,
        PrincipalsDescription const & principalsDescription,
        int allowedUserCreationFailureCount,
        bool updateExisting,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(applicationId);
        UNREFERENCED_PARAMETER(applicationPackageCounter);
        UNREFERENCED_PARAMETER(principalsDescription);
        UNREFERENCED_PARAMETER(allowedUserCreationFailureCount);
        UNREFERENCED_PARAMETER(updateExisting);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureSecurityPrincipalsForNode(
        Common::AsyncOperationSPtr const & operation,
        __out PrincipalsProviderContextUPtr & principalsContext)
    {
        UNREFERENCED_PARAMETER(principalsContext);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginCleanupApplicationSecurityGroup(
        vector<std::wstring> const & applicationIds,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(applicationIds);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndCleanupApplicationSecurityGroup(
        AsyncOperationSPtr const & operation,
        __out vector<std::wstring> & deletedAppPrincipals)
    {
        UNREFERENCED_PARAMETER(deletedAppPrincipals);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr DummyFabricActivatorClient::BeginAssignIpAddresses(
        wstring const & servicePackageInstanceId,
        vector<wstring> const & codePackages,
        bool cleanup,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(servicePackageInstanceId);
        UNREFERENCED_PARAMETER(codePackages);
        UNREFERENCED_PARAMETER(cleanup);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, parent);
    }

    ErrorCode DummyFabricActivatorClient::EndAssignIpAddresses(
        AsyncOperationSPtr const & operation,
        __out std::vector<std::wstring> & assignedIps)
    {
        UNREFERENCED_PARAMETER(assignedIps);
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginManageOverlayNetworkResources(
        std::wstring const & nodeName,
        std::wstring const & nodeIpAddress,
        std::wstring const & servicePackageId,
        std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
        ManageOverlayNetworkAction::Enum action,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(nodeName);
        UNREFERENCED_PARAMETER(nodeIpAddress);
        UNREFERENCED_PARAMETER(servicePackageId);
        UNREFERENCED_PARAMETER(codePackageNetworkNames);
        UNREFERENCED_PARAMETER(action);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, parent);
    }

    Common::ErrorCode EndManageOverlayNetworkResources(
        Common::AsyncOperationSPtr const & operation,
        __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources)
    {
        UNREFERENCED_PARAMETER(operation);
        UNREFERENCED_PARAMETER(assignedNetworkResources);
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginUpdateRoutes(
        Management::NetworkInventoryManager::PublishNetworkTablesRequestMessage const & networkTables,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(networkTables);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::Success, callback, parent);
    }

    Common::ErrorCode EndUpdateRoutes(Common::AsyncOperationSPtr const & operation)
    {
        UNREFERENCED_PARAMETER(operation);
        return CompletedAsyncOperation::End(operation);
    }

#if defined(PLATFORM_UNIX)
    AsyncOperationSPtr BeginDeleteApplicationFolder(
        vector<std::wstring> const & appFolders,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(appFolders);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndDeleteApplicationFolder(
        AsyncOperationSPtr const & operation,
        __out vector<std::wstring> & deletedAppFolders)
    {
        UNREFERENCED_PARAMETER(deletedAppFolders);
        return CompletedAsyncOperation::End(operation);
    }

#endif

    AsyncOperationSPtr BeginConfigureSMBShare(
        vector<std::wstring> const & sidPrincipals,
        DWORD accessMask,
        wstring const & localFullPath,
        wstring const & shareName,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(sidPrincipals);
        UNREFERENCED_PARAMETER(accessMask);
        UNREFERENCED_PARAMETER(localFullPath);
        UNREFERENCED_PARAMETER(shareName);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureSMBShare(Common::AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureCrashDumps(
        bool enable,
        wstring const & servicePackageId,
        vector<std::wstring> const & exeNames,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(enable);
        UNREFERENCED_PARAMETER(servicePackageId);
        UNREFERENCED_PARAMETER(exeNames);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureCrashDumps(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginFabricUpgrade(
        bool const useFabricInstallerService,
        wstring const & programToRun,
        wstring const & arguments,
        wstring const & downloadedFabricPackageLocation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(useFabricInstallerService);
        UNREFERENCED_PARAMETER(programToRun);
        UNREFERENCED_PARAMETER(arguments);
        UNREFERENCED_PARAMETER(downloadedFabricPackageLocation);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndFabricUpgrade(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginCleanupSecurityPrincipals(
        wstring const & applicationId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(applicationId);
        UNREFERENCED_PARAMETER(timeout);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            abortCloseApplicationPackage ? ErrorCodeValue::OperationCanceled : ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndCleanupSecurityPrincipals(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureResourceACLs(
        vector<wstring> const & sids,
        DWORD accessMask,
        vector<CertificateAccessDescription> const & certificateAccessDescriptions,
        vector<wstring> const & applicationFolders,
        ULONG applicationCounter,
        wstring const & nodeId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(sids);
        UNREFERENCED_PARAMETER(accessMask);
        UNREFERENCED_PARAMETER(certificateAccessDescriptions);
        UNREFERENCED_PARAMETER(applicationFolders);
        UNREFERENCED_PARAMETER(applicationCounter);
        UNREFERENCED_PARAMETER(nodeId);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureResourceACLs(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureSecurityPrincipals(
        wstring const & applicationId,
        ULONG applicationPackageCounter,
        PrincipalsDescription const & principalsDescription,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(applicationId);
        UNREFERENCED_PARAMETER(applicationPackageCounter);
        UNREFERENCED_PARAMETER(principalsDescription);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            abortOpenApplicationPackage ? ErrorCodeValue::OperationCanceled : ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureSecurityPrincipals(
        AsyncOperationSPtr const & operation,
        __out PrincipalsProviderContextUPtr & principalsContext)
    {
        principalsContext = make_unique<PrincipalsProviderContext>();
        std::vector<SecurityPrincipalInformation> principalsInfo;
        principalsInfo.push_back(SecurityPrincipalInformation(L"TestUser", L"TestPrincipalId", L"TestSid", false));
        principalsContext->AddSecurityPrincipals(principalsInfo);
        return CompletedAsyncOperation::End(operation);
    }


    //ServicePackageOperations
    AsyncOperationSPtr BeginConfigureEndpointSecurity(
        wstring const & principalSid,
        UINT port,
        bool isHttps,
        bool cleanupAcls,
        wstring const & prefix,
        wstring const & servicePackageId,
        bool isSharedPort,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(principalSid);
        UNREFERENCED_PARAMETER(port);
        UNREFERENCED_PARAMETER(isHttps);
        UNREFERENCED_PARAMETER(cleanupAcls);
        UNREFERENCED_PARAMETER(prefix);
        UNREFERENCED_PARAMETER(servicePackageId);
        UNREFERENCED_PARAMETER(isSharedPort);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::OperationCanceled,
            callback,
            parent);
    }

    ErrorCode EndConfigureEndpointSecurity(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureContainerCertificateExport(
        std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & CertificateRef,
        wstring workDirectoryPath,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(CertificateRef);
        UNREFERENCED_PARAMETER(workDirectoryPath);
        UNREFERENCED_PARAMETER(timeout);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::OperationCanceled,
            callback,
            parent);
    }

    ErrorCode EndConfigureContainerCertificateExport(
       Common::AsyncOperationSPtr const & operation,
        __out std::map<std::wstring, std::wstring> & certificatePaths,
        __out std::map<std::wstring, std::wstring> & certificatePasswordPaths)
    {
        UNREFERENCED_PARAMETER(certificatePaths);
        UNREFERENCED_PARAMETER(certificatePasswordPaths);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginCleanupContainerCertificateExport(
        std::map<std::wstring, std::wstring> const & certificatePaths,
        std::map<std::wstring, std::wstring> const & certificatePasswordPaths,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(certificatePaths);
        UNREFERENCED_PARAMETER(certificatePasswordPaths);
        UNREFERENCED_PARAMETER(timeout);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::OperationCanceled,
            callback,
            parent);
    }

    ErrorCode EndCleanupContainerCertificateExport(
        Common::AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginCreateSymbolicLink(
        vector<ArrayPair<std::wstring, std::wstring>> const & symbolicLinks,
        DWORD flags,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(symbolicLinks);
        UNREFERENCED_PARAMETER(flags);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndCreateSymbolicLink(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginDownloadContainerImages(
        vector<ContainerImageDescription> const & images,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(images);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndDownloadContainerImages(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginDeleteContainerImages(
        vector<wstring> const & images,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(images);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndDeleteContainerImages(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginSetupContainerGroup(
        std::wstring const & containerGroupId,
        ServiceModel::NetworkType::Enum networkType,
        std::wstring const & openNetworkAssignedIp,
        std::map<std::wstring, std::wstring> const & overlayNetworkResources,
        std::vector<std::wstring> const & dnsServers,
        std::wstring const & appfolder,
        std::wstring const & appId,
        std::wstring const & appName,
        std::wstring const & partitionId,
        std::wstring const & servicePackageActivationId,
        ServiceModel::ServicePackageResourceGovernanceDescription const & spRg,
#if defined(PLATFORM_UNIX)
        ContainerPodDescription const & podDesc,
#endif
        bool isCleanup,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(containerGroupId);
        UNREFERENCED_PARAMETER(networkType);
        UNREFERENCED_PARAMETER(openNetworkAssignedIp);
        UNREFERENCED_PARAMETER(overlayNetworkResources);
        UNREFERENCED_PARAMETER(dnsServers);
        UNREFERENCED_PARAMETER(appfolder);
        UNREFERENCED_PARAMETER(appId);
        UNREFERENCED_PARAMETER(appName);
        UNREFERENCED_PARAMETER(partitionId);
        UNREFERENCED_PARAMETER(servicePackageActivationId);
        UNREFERENCED_PARAMETER(isCleanup);
        UNREFERENCED_PARAMETER(spRg);
#if defined(PLATFORM_UNIX)
        UNREFERENCED_PARAMETER(podDesc);
#endif

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndSetupContainerGroup(
        AsyncOperationSPtr const & operation,
        __out wstring & data)
    {
        UNREFERENCED_PARAMETER(data);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginGetContainerInfo(
        std::wstring const & ,
        bool ,
        wstring const & containerInfoType,
        wstring const & containerInfoArgs,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(containerInfoType);
        UNREFERENCED_PARAMETER(containerInfoArgs);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndGetContainerInfo(
        AsyncOperationSPtr const & operation,
        __out wstring & containerInfo)
    {
        UNREFERENCED_PARAMETER(containerInfo);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginGetImages(
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndGetImages(
        Common::AsyncOperationSPtr const & operation,
        __out std::vector<wstring> & images)
    {
        UNREFERENCED_PARAMETER(images);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureSharedFolderPermissions(
        vector<std::wstring> const & sharedFolders,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(sharedFolders);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureSharedFolderPermissions(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginTerminateProcess(
        wstring const & appServiceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(appServiceId);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndTerminateProcess(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginConfigureEndpointCertificateAndFirewallPolicy(
        wstring const & servicePackageId,
        vector<EndpointCertificateBinding> const & endpointCertBindings,
        bool cleanupEndpointCert,
        bool cleanupFirewallPolicy,
        std::vector<LONG> const & firewallPorts,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(servicePackageId);
        UNREFERENCED_PARAMETER(endpointCertBindings);
        UNREFERENCED_PARAMETER(cleanupEndpointCert);
        UNREFERENCED_PARAMETER(cleanupFirewallPolicy);
        UNREFERENCED_PARAMETER(firewallPorts);
        UNREFERENCED_PARAMETER(timeout);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            abortOpenServicePackage ? ErrorCodeValue::OperationCanceled : ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndConfigureEndpointCertificateAndFirewallPolicy(
        AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    ErrorCode ConfigureEndpointSecurity(
        wstring const & principalSid,
        UINT port,
        bool isHttps,
        bool cleanupAcls,
        wstring const & servicePackageId,
        bool isSharedPort)
    {
        UNREFERENCED_PARAMETER(principalSid);
        UNREFERENCED_PARAMETER(port);
        UNREFERENCED_PARAMETER(isHttps);
        UNREFERENCED_PARAMETER(cleanupAcls);
        UNREFERENCED_PARAMETER(servicePackageId);
        UNREFERENCED_PARAMETER(isSharedPort);
        return ErrorCode(ErrorCodeValue::OperationCanceled);
    }

    ErrorCode ConfigureEndpointBindingAndFirewallPolicy(
        wstring const & servicePackageId,
        vector<Hosting2::EndpointCertificateBinding> const & endpointCertBindings,
        bool isCleanup,
        bool cleanupFirewallPolicy,
        std::vector<LONG> const & firewallPorts)
    {
        UNREFERENCED_PARAMETER(servicePackageId);
        UNREFERENCED_PARAMETER(endpointCertBindings);
        UNREFERENCED_PARAMETER(isCleanup);
        UNREFERENCED_PARAMETER(cleanupFirewallPolicy);
        UNREFERENCED_PARAMETER(firewallPorts);
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CleanupAssignedIPs(wstring const & servicePackageId)
    {
        UNREFERENCED_PARAMETER(servicePackageId);
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CleanupAssignedOverlayNetworkResources(
        std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
        std::wstring const & nodeName,
        std::wstring const & nodeIpAddress,
        std::wstring const & servicePackageId)
    {
        UNREFERENCED_PARAMETER(codePackageNetworkNames);
        UNREFERENCED_PARAMETER(nodeName);
        UNREFERENCED_PARAMETER(nodeIpAddress);
        UNREFERENCED_PARAMETER(servicePackageId);
        return ErrorCode(ErrorCodeValue::Success);
    }

    void AbortApplicationEnvironment(std::wstring const & applicationId)
    {
        UNREFERENCED_PARAMETER(applicationId);
        //UnImplemented
    }

    void AbortProcess(std::wstring const & appServiceId)
    {
        UNREFERENCED_PARAMETER(appServiceId);
        //UnImplemented
    }

    Common::HHandler AddProcessTerminationHandler(Common::EventHandler const & eventhandler)
    {
        return event_.Add(eventhandler);
    }

    void RemoveProcessTerminationHandler(Common::HHandler const & handler)
    {
        event_.Remove(handler);
    }

    Common::HHandler AddContainerHealthCheckStatusChangeHandler(Common::EventHandler const & eventhandler)
    {
        return healthEvent_.Add(eventhandler);
    }

    void RemoveContainerHealthCheckStatusChangeHandler(Common::HHandler const & handler)
    {
        healthEvent_.Remove(handler);
    }

    Common::HHandler AddRootContainerTerminationHandler(Common::EventHandler const & eventhandler)
    {
        return event_.Add(eventhandler);
    }

    void RemoveRootContainerTerminationHandler(Common::HHandler const & handler)
    {
        event_.Remove(handler);
    }

    bool IsIpcClientInitialized()
    {
        return false;
    }

    Common::AsyncOperationSPtr BeginConfigureNodeForDnsService(
        bool isDnsServiceEnabled,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(isDnsServiceEnabled);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    Common::ErrorCode EndConfigureNodeForDnsService(
        Common::AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginGetNetworkDeployedPackages(
        std::vector<std::wstring> const & servicePackageIds,
        std::wstring const & codePackageName,
        std::wstring const & networkName,
        std::wstring const & nodeId,
        std::map<std::wstring, std::wstring> const & codePackageInstanceAppHostMap,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(servicePackageIds);
        UNREFERENCED_PARAMETER(codePackageName);
        UNREFERENCED_PARAMETER(networkName);
        UNREFERENCED_PARAMETER(nodeId);
        UNREFERENCED_PARAMETER(codePackageInstanceAppHostMap);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    Common::ErrorCode EndGetNetworkDeployedPackages(
        Common::AsyncOperationSPtr const & operation,
        __out std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> & networkReservedCodePackages,
        __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & codePackageInstanceIdentifierContainerInfoMap)
    {
        UNREFERENCED_PARAMETER(networkReservedCodePackages);
        UNREFERENCED_PARAMETER(codePackageInstanceIdentifierContainerInfoMap);
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginGetDeployedNetworks(
        ServiceModel::NetworkType::Enum networkType,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(networkType);
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    Common::ErrorCode EndGetDeployedNetworks(
        Common::AsyncOperationSPtr const & operation,
        __out std::vector<std::wstring> & networkNames)
    {
        UNREFERENCED_PARAMETER(networkNames);
        return CompletedAsyncOperation::End(operation);
    }

protected:
    AsyncOperationSPtr OnBeginOpen(
        TimeSpan timeout,
        AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode OnEndOpen(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr OnBeginClose(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(timeout);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode OnEndClose(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    void OnAbort()
    {
        //empty
    }

public:
    __declspec(property(put = set_OpenSvcPkg)) bool AbortOpenServicePackage;
    inline void set_OpenSvcPkg(bool value) { abortOpenServicePackage = value; }

    __declspec(property(put = set_CloseApplnPkg)) bool AbortCloseApplicationPackage;
    inline void set_CloseApplnPkg(bool value) { abortCloseApplicationPackage = value; }

    __declspec(property(put = set_OpenApplnPkg)) bool AbortOpenApplicationPackage;
    inline void set_OpenApplnPkg(bool value) { abortOpenApplicationPackage = value; }

public:
    Common::Event event_;
    Common::Event healthEvent_;
    bool abortOpenServicePackage;
    bool abortCloseApplicationPackage;
    bool abortOpenApplicationPackage;
};


class ApplicationUpgradeTest
{
protected:
    ApplicationUpgradeTest() : fabricNodeHost_(make_shared<TestFabricNodeHost>()) { BOOST_REQUIRE(Setup()); }
    TEST_METHOD_SETUP(Setup);
    ~ApplicationUpgradeTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_METHOD_CLEANUP(Cleanup);

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
    unique_ptr<RunLayoutSpecification> runLayout_;
    unique_ptr<StoreLayoutSpecification> imageStore_;

    static GlobalWString AppUpgradeAppTypeName;
    static GlobalWString AppUpgradeAppId;
    static GlobalWString AppUpgradeServicePackageName;
    static GlobalWString AppUpgradeAppName;

    bool DownloadUpgradeApplicationPackage(ApplicationIdentifier const & appId, RolloutVersion const & rollOut);
    void BeginDownloadAndActivate(ApplicationIdentifier const & appId);
    vector<ServicePackageUpgradeSpecification> CreatePackageUpgradeSpecification(
        std::wstring const & packageName,
        ServiceModel::RolloutVersion const & version);
};

GlobalWString ApplicationUpgradeTest::AppUpgradeAppName = make_global<wstring>(L"AppUpgradeApp");
GlobalWString ApplicationUpgradeTest::AppUpgradeAppTypeName = make_global<wstring>(L"AppUpgradeApp");
GlobalWString ApplicationUpgradeTest::AppUpgradeAppId = make_global<wstring>(L"AppUpgradeApp_App0");
GlobalWString ApplicationUpgradeTest::AppUpgradeServicePackageName = make_global<wstring>(L"AppUpgradeServicePackage");


bool ApplicationUpgradeTest::Setup()
{
    HostingConfig::GetConfig().EndpointProviderEnabled = true;
    HostingConfig::GetConfig().RunAsPolicyEnabled = true;

    if (!fabricNodeHost_->Open())
        return false;

    runLayout_ = make_unique<RunLayoutSpecification>(fabricNodeHost_->GetHosting().DeploymentFolder);

    fabricNodeHost_->GetHosting().Test_SetFabricActivatorClient(make_shared<DummyFabricActivatorClient>());

    imageStore_ = make_unique<StoreLayoutSpecification>(fabricNodeHost_->GetImageStoreRoot());

    return true;
}

bool ApplicationUpgradeTest::DownloadUpgradeApplicationPackage(ApplicationIdentifier const & appId, RolloutVersion const & rolloutUpgrade)
{
    ErrorCode error;

    error = File::Copy(
        this->imageStore_->GetApplicationPackageFile(
        *AppUpgradeAppTypeName,
        appId.ToString(),
        rolloutUpgrade.ToString()),
        this->runLayout_->GetApplicationPackageFile(
        appId.ToString(),
        rolloutUpgrade.ToString()),
        true);

    if (!error.IsSuccess())
    {

        Trace.WriteError(
            TraceType,
            "Unable to copy ApplicationPackage.{0}.xml App.{1}.xml to AppInstanceFolder with ErrorCode={2}",
            rolloutUpgrade.ToString(),
            rolloutUpgrade.ToString(),
            error);

        return false;
    }

    error = File::Copy(
        this->imageStore_->GetServicePackageFile(
        *AppUpgradeAppTypeName,
        appId.ToString(),
        *AppUpgradeServicePackageName,
        rolloutUpgrade.ToString()),
        this->runLayout_->GetServicePackageFile(
        appId.ToString(),
        *AppUpgradeServicePackageName,
        rolloutUpgrade.ToString()),
        true);

    if (!error.IsSuccess())
    {
        Trace.WriteError(
            TraceType,
            "Unable to copy AppUpgradeServicePackage.Package.{0}.xml to AppInstanceFolder with ErrorCode={1}",
            rolloutUpgrade.ToString(),
            error);

        return false;
    }

    error = File::Copy(
        this->imageStore_->GetServiceManifestFile(
        *AppUpgradeAppTypeName,
        *AppUpgradeServicePackageName,
        rolloutUpgrade.ToString()),
        this->runLayout_->GetServiceManifestFile(appId.ToString(),
        *AppUpgradeServicePackageName,
        rolloutUpgrade.ToString()),
        true);

    if (!error.IsSuccess())
    {
        Trace.WriteError(
            TraceType,
            "Unable to copy AppUpgradeServicePackage.Manifest.{0}.xml to AppInstanceFolder with ErrorCode={1}",
            rolloutUpgrade.ToString(),
            error);

        return false;
    }

    return true;
}

bool ApplicationUpgradeTest::Cleanup()
{
    auto retval = fabricNodeHost_->Close();
    if (!retval)
    {
        Sleep(5000);
    }
    return retval;
}

void ApplicationUpgradeTest::BeginDownloadAndActivate(ApplicationIdentifier const & appId)
{
    //Downloads And Activates the Application 1.0

    wstring serviceTypeName = L"AppUpgradeServiceType";
    ServicePackageIdentifier servicePackageIdentifier(appId, *AppUpgradeServicePackageName);

    ServiceTypeIdentifier serviceTypeId = ServiceTypeIdentifier(servicePackageIdentifier, serviceTypeName);
    ServicePackageVersionInstance servicePackageVersionInstance;
    VersionedServiceTypeIdentifier versionedServiceTypeId = VersionedServiceTypeIdentifier(serviceTypeId, servicePackageVersionInstance);

    {
        ManualResetEvent downloadEvent;

        fabricNodeHost_->GetHosting().ApplicationManagerObj->BeginDownloadAndActivate(
            1,
            versionedServiceTypeId,
            ServicePackageActivationContext(),
            L"",
            *AppUpgradeAppName,
            [this, &downloadEvent](AsyncOperationSPtr const & operation)
            {
                auto error = fabricNodeHost_->GetHosting().ApplicationManagerObj->EndDownloadAndActivate(operation);
                VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success), L"Download and activation Succeded");
                downloadEvent.Set();
            },
            AsyncOperationSPtr());

        VERIFY_IS_TRUE(downloadEvent.WaitOne(10000), L"Download and Activate completed before timeout.");
    }

}

vector<ServicePackageUpgradeSpecification> ApplicationUpgradeTest::CreatePackageUpgradeSpecification(
    std::wstring const & packageName,
    ServiceModel::RolloutVersion const & version)
{
    vector<ServiceModel::ServicePackageUpgradeSpecification> servicePackageUpgradeSpecs;
    servicePackageUpgradeSpecs.push_back(ServiceModel::ServicePackageUpgradeSpecification(
        packageName,
        version,
        vector<wstring>()));
    return servicePackageUpgradeSpecs;
}

BOOST_FIXTURE_TEST_SUITE(ApplicationUpgradeTestSuite, ApplicationUpgradeTest)

BOOST_AUTO_TEST_CASE(TestOpenServicePackage)
{
    // Do switch for Application Package and Close-Open for service package
    // Close ServicePackage completes with success
    // and Open Service Package fails with OperationCancelled

    ApplicationIdentifier appId = ApplicationIdentifier(*AppUpgradeAppTypeName, 0);
    uint64 applicationUpgradeInstanceId = 2;

    //Download and activate application
    BeginDownloadAndActivate(appId);

    ManualResetEvent upgradeEvent;
    RolloutVersion rolloutUpgrade = RolloutVersion(2, 0);
    ApplicationVersion appUpgradeVer = ApplicationVersion(rolloutUpgrade);
    bool isDownloaded = DownloadUpgradeApplicationPackage(appId, rolloutUpgrade);

    VERIFY_IS_TRUE(isDownloaded, L"Download Upgrade Application Package 2.0 Complete");

    // will create a completed Async Operation with OperationCancelled
    static_cast<DummyFabricActivatorClient*>(fabricNodeHost_->GetHosting().FabricActivatorClientObj.get())->AbortOpenServicePackage = true;

    ApplicationUpgradeSpecification appUpgradeSpecs(
        appId,
        appUpgradeVer,
        *AppUpgradeAppName,
        applicationUpgradeInstanceId,
        UpgradeType::Rolling_ForceRestart,
        false,
        false,
        CreatePackageUpgradeSpecification(*AppUpgradeServicePackageName, rolloutUpgrade),
        vector<ServiceModel::ServiceTypeRemovalSpecification>(),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());

    fabricNodeHost_->GetHosting().BeginUpgradeApplication(
        appUpgradeSpecs,
        [this, &upgradeEvent](AsyncOperationSPtr const & operation)
    {
        auto error = fabricNodeHost_->GetHosting().EndUpgradeApplication(operation);

        Trace.WriteNoise(TraceType, "Test Open Service Package completed with error: {0} expected error: {1} ",
            error, "OperationCanceled");

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::OperationCanceled), L"Verification Failed");
        upgradeEvent.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(upgradeEvent.WaitOne(10000), L"TestOpenServicePackage completed before timeout.");
    static_cast<DummyFabricActivatorClient*>(fabricNodeHost_->GetHosting().FabricActivatorClientObj.get())->AbortOpenServicePackage = false;
}


BOOST_AUTO_TEST_CASE(TestCloseApplicationPackage)
{
    // Close Application Package fails with OperationCancelled and aborts with Success.
    // Open Application Package succeeds. To close-open ApplicationPackage ContentChecksum is changed
    // for ApplicationPackage3.0.xml

    ManualResetEvent upgradeEvent;

    RolloutVersion rolloutUpgrade = RolloutVersion(3, 0);
    ApplicationVersion appUpgradeVer = ApplicationVersion(rolloutUpgrade);
    ApplicationIdentifier appId = ApplicationIdentifier(*AppUpgradeAppTypeName, 0);
    uint64 applicationUpgradeInstanceId = 3;

    //Download and activate application
    BeginDownloadAndActivate(appId);

    bool isDownloaded = DownloadUpgradeApplicationPackage(appId, rolloutUpgrade);

    VERIFY_IS_TRUE(isDownloaded, L"Download Upgrade Application Package 3.0 Complete");

    // Will cancel the parent UpgradeAsync Operation
    static_cast<DummyFabricActivatorClient*>(fabricNodeHost_->GetHosting().FabricActivatorClientObj.get())->AbortCloseApplicationPackage = true;

    AsyncOperationSPtr operation = make_shared<ComponentRoot>()->CreateAsyncOperationRoot();

    ApplicationUpgradeSpecification appUpgradeSpecs(
        appId,
        appUpgradeVer,
        *AppUpgradeAppName,
        applicationUpgradeInstanceId,
        UpgradeType::Rolling_ForceRestart,
        false,
        false,
        CreatePackageUpgradeSpecification(*AppUpgradeServicePackageName, rolloutUpgrade),
        vector<ServiceModel::ServiceTypeRemovalSpecification>(),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());

    fabricNodeHost_->GetHosting().BeginUpgradeApplication(
        appUpgradeSpecs,
        [this, &upgradeEvent](AsyncOperationSPtr const & operation)
    {
        auto error = fabricNodeHost_->GetHosting().EndUpgradeApplication(operation);

        Trace.WriteNoise(TraceType, "Test Close Application Package completed with error: {0} expected error: {1} ",
            error, "Success");

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success), L"Verification Succeeded");
        upgradeEvent.Set();
    },
        operation);

    VERIFY_IS_TRUE(upgradeEvent.WaitOne(10000), L"TestCloseApplicationPackage completed before timeout.");

    static_cast<DummyFabricActivatorClient*>(fabricNodeHost_->GetHosting().FabricActivatorClientObj.get())->AbortCloseApplicationPackage = false;
}


BOOST_AUTO_TEST_CASE(TestOpenApplicationPackage)
{
    // Close Application Package succeeds.
    // Open Application Package fails with OperationCancelled. To close-open ApplicationPackage ContentChecksum is changed
    // for ApplicationPackage3.0.xml to perform close-open

    ManualResetEvent upgradeEvent;
    RolloutVersion rolloutUpgrade = RolloutVersion(3, 0);
    ApplicationVersion appUpgradeVer = ApplicationVersion(rolloutUpgrade);
    ApplicationIdentifier appId = ApplicationIdentifier(*AppUpgradeAppTypeName, 0);
    uint64 applicationUpgradeInstanceId = 3;

    //Download and activate application
    BeginDownloadAndActivate(appId);

    bool isDownloaded = DownloadUpgradeApplicationPackage(appId, rolloutUpgrade);

    VERIFY_IS_TRUE(isDownloaded, L"Download Upgrade Application Package 3.0 Complete");

    // will create a completed Async Operation with OperationCancelled
    static_cast<DummyFabricActivatorClient*>(fabricNodeHost_->GetHosting().FabricActivatorClientObj.get())->AbortOpenApplicationPackage = true;

    ApplicationUpgradeSpecification appUpgradeSpecs(
        appId,
        appUpgradeVer,
        *AppUpgradeAppName,
        applicationUpgradeInstanceId,
        UpgradeType::Rolling_ForceRestart,
        false,
        false,
        CreatePackageUpgradeSpecification(*AppUpgradeServicePackageName, rolloutUpgrade),
        vector<ServiceModel::ServiceTypeRemovalSpecification>(),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());

    fabricNodeHost_->GetHosting().BeginUpgradeApplication(
        appUpgradeSpecs,
        [this, &upgradeEvent](AsyncOperationSPtr const & operation)
    {
        auto error = fabricNodeHost_->GetHosting().EndUpgradeApplication(operation);

        Trace.WriteNoise(TraceType, "Test Open Application Package completed with error: {0} expected error: {1} ",
            error, "Success");

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::OperationCanceled), L"Verification Succeeded");
        upgradeEvent.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(upgradeEvent.WaitOne(10000), L"TestOpenApplicationPackage completed before timeout.");
    static_cast<DummyFabricActivatorClient*>(fabricNodeHost_->GetHosting().FabricActivatorClientObj.get())->AbortOpenApplicationPackage = false;
}

BOOST_AUTO_TEST_CASE(TestCancelSwitchApplicationPackage)
{
    // Cancel Switch Application Package operation after transition to state "Upgrading".
    // Close-Open happens as switch fails. Close fails to transition to "Closing" state from "Upgrading" state and aborts with Succeess
    // Open Application Package transition to fails on seeing the parent aborted.

    ManualResetEvent upgradeEvent;

    //Fail the Switch Application Package with Abort
    HostingConfig::GetConfig().AbortSwitchApplicationPackage = true;

    RolloutVersion rolloutUpgrade = RolloutVersion(2, 0);
    ApplicationVersion appUpgradeVer = ApplicationVersion(rolloutUpgrade);
    ApplicationIdentifier appId = ApplicationIdentifier(*AppUpgradeAppTypeName, 0);
    uint64 applicationUpgradeInstanceId = 2;

    //Download and activate application
    BeginDownloadAndActivate(appId);

    bool isDownloaded = DownloadUpgradeApplicationPackage(appId, rolloutUpgrade);

    VERIFY_IS_TRUE(isDownloaded, L"Download Upgrade Application Package 2.0 Complete");

    ApplicationUpgradeSpecification appUpgradeSpecs(
        appId,
        appUpgradeVer,
        *AppUpgradeAppName,
        applicationUpgradeInstanceId,
        //Upgrade Type to Notification only so that switch happens at service package
        UpgradeType::Rolling_NotificationOnly,
        false,
        false,
        CreatePackageUpgradeSpecification(*AppUpgradeServicePackageName, rolloutUpgrade),
        vector<ServiceModel::ServiceTypeRemovalSpecification>(),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());

    fabricNodeHost_->GetHosting().BeginUpgradeApplication(
        appUpgradeSpecs,
        [this, &upgradeEvent](AsyncOperationSPtr const & operation)
    {
        auto error = fabricNodeHost_->GetHosting().EndUpgradeApplication(operation);

        Trace.WriteNoise(TraceType, "Test Cancel Switch Application Package completed with error: {0} expected error: {1} ",
            error, "OperationCanceled");

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::OperationCanceled), L"Verification Succeeded");
        upgradeEvent.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(upgradeEvent.WaitOne(10000), L"CancelSwitchApplicationPackage completed before timeout.");
    HostingConfig::GetConfig().AbortSwitchApplicationPackage = false;
}

BOOST_AUTO_TEST_CASE(TestCancelSwitchServicePackage)
{
    // Cancel Switch Service Package operation after transition to state "Switching".
    // Close-Open happens as switch fails. Close fails to transition to "Closing" state from "Switching" state and aborts with Succeess
    // Open Service Package transition to fails on seeing the parent aborted.
    ManualResetEvent upgradeEvent;

    //Fail the Switch Service Package with Abort
    HostingConfig::GetConfig().AbortSwitchServicePackage = true;

    RolloutVersion rolloutUpgrade = RolloutVersion(2, 0);
    ApplicationVersion appUpgradeVer = ApplicationVersion(rolloutUpgrade);
    ApplicationIdentifier appId = ApplicationIdentifier(*AppUpgradeAppTypeName, 0);
    uint64 applicationUpgradeInstanceId = 2;

    //Download and activate application
    BeginDownloadAndActivate(appId);

    bool isDownloaded = DownloadUpgradeApplicationPackage(appId, rolloutUpgrade);

    VERIFY_IS_TRUE(isDownloaded, L"Download Upgrade Application Package 2.0 Complete");

    ApplicationUpgradeSpecification appUpgradeSpecs(
        appId,
        appUpgradeVer,
        *AppUpgradeAppName,
        applicationUpgradeInstanceId,
        //Upgrade Type to Notification only so that switch happens at service package
        UpgradeType::Rolling_NotificationOnly,
        false,
        false,
        CreatePackageUpgradeSpecification(*AppUpgradeServicePackageName, rolloutUpgrade),
        vector<ServiceModel::ServiceTypeRemovalSpecification>(),
        map<ServicePackageIdentifier, ServicePackageResourceGovernanceDescription>());

    fabricNodeHost_->GetHosting().BeginUpgradeApplication(
        appUpgradeSpecs,
        [this, &upgradeEvent](AsyncOperationSPtr const & operation)
    {
        auto error = fabricNodeHost_->GetHosting().EndUpgradeApplication(operation);

        Trace.WriteNoise(TraceType, "Test Cancel Switch Service Package completed with error: {0} expected error: {1} ",
            error, "OperationCanceled");

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::OperationCanceled), L"Verification Succeeded");
        upgradeEvent.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(upgradeEvent.WaitOne(10000), L"CancelSwitchServicePackage completed before timeout.");
    HostingConfig::GetConfig().AbortSwitchServicePackage = false;
}

BOOST_AUTO_TEST_SUITE_END()


