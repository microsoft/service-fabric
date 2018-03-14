// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ServicePackageOperation 
    {
    public:
        ServicePackageOperation();
        ServicePackageOperation(
            std::wstring const & packageName, 
            ServiceModel::RolloutVersion const & packageRolloutVersion,
            int64 const versionInstanceId,
            ServiceModel::ServicePackageDescription const & packageDescription,
            ServiceModel::UpgradeType::Enum const upgradeType = ServiceModel::UpgradeType::Invalid);
        ServicePackageOperation(ServicePackageOperation const & other);
        ServicePackageOperation(ServicePackageOperation && other);

        __declspec(property(get=get_PackageName, put=put_PackageName)) std::wstring const & PackageName;
        std::wstring const & get_PackageName() const { return packageName_; }
        void put_PackageName(std::wstring const & value) { packageName_ = value; }

        __declspec(property(get=get_PackageDesc, put=put_PackageDesc)) ServiceModel::ServicePackageDescription const & PackageDescription;
        ServiceModel::ServicePackageDescription const & get_PackageDesc() const { return packageDescription_; }
        void put_PackageDesc(ServiceModel::ServicePackageDescription const & value) { packageDescription_ = value; }

        __declspec(property(get=get_PackageRolloutVersion, put=put_PackageRolloutVersion)) ServiceModel::RolloutVersion const & PackageRolloutVersion;
        ServiceModel::RolloutVersion const & get_PackageRolloutVersion() const { return packageRolloutVersion_; }
        void put_PackageRolloutVersion(ServiceModel::RolloutVersion const & value) { packageRolloutVersion_ = value; }

        __declspec(property(get=get_VersionInstanceId, put=put_VersionInstanceId)) int64 const VersionInstanceId;
        int64 const get_VersionInstanceId() const { return versionInstanceId_; }
        void put_VersionInstanceId(int64 const value) { versionInstanceId_ = value; }

        __declspec(property(get=get_UpgradeType, put=put_UpgradeType)) ServiceModel::UpgradeType::Enum const UpgradeType;
        ServiceModel::UpgradeType::Enum const get_UpgradeType() const { return upgradeType_; }
        void put_UpgradeType(ServiceModel::UpgradeType::Enum const value) { upgradeType_ = value; }

        ServicePackageOperation const & operator = (ServicePackageOperation const & other);
        ServicePackageOperation const & operator = (ServicePackageOperation && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        std::wstring packageName_;
        ServiceModel::ServicePackageDescription packageDescription_;
        ServiceModel::RolloutVersion packageRolloutVersion_;
        int64 versionInstanceId_;
        ServiceModel::UpgradeType::Enum upgradeType_;
    };
}
