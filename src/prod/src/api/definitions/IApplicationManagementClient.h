// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IApplicationManagementClient
    {
    public:
        virtual ~IApplicationManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginProvisionApplicationType(
            Management::ClusterManager::ProvisionApplicationTypeDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndProvisionApplicationType(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCreateApplication(
            ServiceModel::ApplicationDescriptionWrapper const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateApplication(
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::AsyncOperationSPtr BeginUpdateApplication(
            ServiceModel::ApplicationUpdateDescriptionWrapper const &updateDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr const&) = 0;
        virtual Common::ErrorCode EndUpdateApplication(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginUpgradeApplication(
            ServiceModel::ApplicationUpgradeDescriptionWrapper const &upgradeDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUpgradeApplication(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationUpgradeProgress(
            Common::NamingUri const &applicationName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetApplicationUpgradeProgress(
            Common::AsyncOperationSPtr const &,
            IApplicationUpgradeProgressResultPtr &upgradeDescription) = 0;

        virtual Common::AsyncOperationSPtr BeginMoveNextApplicationUpgradeDomain(
            IApplicationUpgradeProgressResultPtr const &progress,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndMoveNextApplicationUpgradeDomain(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginMoveNextApplicationUpgradeDomain2(
            Common::NamingUri const &applicationName,
            std::wstring const &nextUpgradeDomain,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndMoveNextApplicationUpgradeDomain2(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteApplication(
            ServiceModel::DeleteApplicationDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteApplication(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginUnprovisionApplicationType(
            Management::ClusterManager::UnprovisionApplicationTypeDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUnprovisionApplicationType(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetApplicationManifest(
            std::wstring const& applicationTypeName,
            std::wstring const& applicationTypeVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetApplicationManifest(
            Common::AsyncOperationSPtr const &,
            __inout std::wstring &clusterManifest) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateApplicationUpgrade(
            Management::ClusterManager::ApplicationUpgradeUpdateDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUpdateApplicationUpgrade(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRollbackApplicationUpgrade(
            Common::NamingUri const & applicationName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRollbackApplicationUpgrade(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRestartDeployedCodePackage(
            std::wstring const & nodeName,
            Common::NamingUri const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & codePackageName,
            std::wstring const & instanceId,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndRestartDeployedCodePackage(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginDeployServicePackageToNode(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::wstring const & serviceManifestName,
            std::wstring const & sharingPolicies,
            std::wstring const & nodeName,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeployServicePackageToNode(
            Common::AsyncOperationSPtr const &) = 0;
            
        virtual Common::AsyncOperationSPtr BeginGetContainerInfo(
                std::wstring const & nodeName,
                Common::NamingUri const & applicationName,
                std::wstring const & serviceManifestName,
                std::wstring const & codePackageName,
                std::wstring const & containerInfoType,
                ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const &,
            __out wstring & result) = 0;

        virtual Common::AsyncOperationSPtr BeginInvokeContainerApi(
            std::wstring const & nodeName,
            Common::NamingUri const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & codePackageName,
            FABRIC_INSTANCE_ID codePackageInstance,
            ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndInvokeContainerApi(
            Common::AsyncOperationSPtr const &,
            __out wstring &containerInfo) = 0;
    };
}
