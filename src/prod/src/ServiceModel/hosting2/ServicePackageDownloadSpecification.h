// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ServicePackageDownloadSpecification : public Serialization::FabricSerializable
    {
    public:
        ServicePackageDownloadSpecification();
        ServicePackageDownloadSpecification(
            std::wstring const & packageName, 
            ServiceModel::RolloutVersion const & packageRolloutVersion);
        ServicePackageDownloadSpecification(ServicePackageDownloadSpecification const & other);
        ServicePackageDownloadSpecification(ServicePackageDownloadSpecification && other);

        __declspec(property(get=get_ServicePackageName, put=put_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName() const { return packageName_; }
        void put_ServicePackageName(std::wstring const & value) { packageName_ = value; }

        __declspec(property(get=get_RolloutVersion, put=put_RolloutVersion)) ServiceModel::RolloutVersion const & RolloutVersionValue;
        ServiceModel::RolloutVersion const & get_RolloutVersion() const { return packageRolloutVersion_; }
        void put_RolloutVersion(ServiceModel::RolloutVersion const & value) { packageRolloutVersion_ = value; }

        ServicePackageDownloadSpecification const & operator = (ServicePackageDownloadSpecification const & other);
        ServicePackageDownloadSpecification const & operator = (ServicePackageDownloadSpecification && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(packageName_, packageRolloutVersion_);

    private:
        std::wstring packageName_;
        ServiceModel::RolloutVersion packageRolloutVersion_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Hosting2::ServicePackageDownloadSpecification);
