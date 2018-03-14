// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ServicePackageInstanceOperation 
    {
    public:
        ServicePackageInstanceOperation();
        ServicePackageInstanceOperation(
            std::wstring const & packageName,
            ServiceModel::ServicePackageActivationContext activationContext,
			std::wstring const & servicePackagePublicActivationId,
            ServiceModel::RolloutVersion const & packageRolloutVersion,
            int64 const versionInstanceId,
            ServiceModel::ServicePackageDescription const & packageDescription,
            ServiceModel::UpgradeType::Enum const upgradeType = ServiceModel::UpgradeType::Invalid);

        ServicePackageInstanceOperation(
            ServicePackageOperation const & packageOperation,
            ServiceModel::ServicePackageActivationContext const & activationContext,
			std::wstring const & servicePackagePublicActivationId);
        
        ServicePackageInstanceOperation(ServicePackageInstanceOperation const & other);
        ServicePackageInstanceOperation(ServicePackageInstanceOperation && other);

        __declspec(property(get = get_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName() const { return packageOperation_.PackageName; }

        __declspec(property(get = get_ActivationContext)) ServiceModel::ServicePackageActivationContext const & ActivationContext;
        ServiceModel::ServicePackageActivationContext const & get_ActivationContext() const { return activationContext_; }

		__declspec(property(get = get_PublicActivationId)) std::wstring const & PublicActivationId;
		std::wstring const & get_PublicActivationId() const { return servicePackagePublicActivationId_; }

        __declspec(property(get = get_OperationId)) std::wstring const & OperationId;
        std::wstring const & get_OperationId() const { return operationId_; }

        __declspec(property(get=get_PackageDesc)) ServiceModel::ServicePackageDescription const & PackageDescription;
        ServiceModel::ServicePackageDescription const & get_PackageDesc() const { return packageOperation_.PackageDescription; }

        __declspec(property(get=get_PackageRolloutVersion)) ServiceModel::RolloutVersion const & PackageRolloutVersion;
        ServiceModel::RolloutVersion const & get_PackageRolloutVersion() const { return packageOperation_.PackageRolloutVersion; }

        __declspec(property(get=get_VersionInstanceId)) int64 const VersionInstanceId;
        int64 const get_VersionInstanceId() const { return packageOperation_.VersionInstanceId; }

        __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum const UpgradeType;
        ServiceModel::UpgradeType::Enum const get_UpgradeType() const { return packageOperation_.UpgradeType; }

        ServicePackageInstanceOperation const & operator = (ServicePackageInstanceOperation const & other);
        ServicePackageInstanceOperation const & operator = (ServicePackageInstanceOperation && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        ServiceModel::ServicePackageActivationContext activationContext_;
		std::wstring servicePackagePublicActivationId_;
        ServicePackageOperation packageOperation_;
        std::wstring operationId_;
    };
}
