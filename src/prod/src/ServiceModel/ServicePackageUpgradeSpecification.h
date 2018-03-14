// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServicePackageUpgradeSpecification : public Serialization::FabricSerializable
    {
    public:
        ServicePackageUpgradeSpecification();
        ServicePackageUpgradeSpecification(
            std::wstring const & packageName, 
            RolloutVersion const & packageRolloutVersion,
            std::vector<std::wstring> const & codePackageNames);
        ServicePackageUpgradeSpecification(ServicePackageUpgradeSpecification const & other);
        ServicePackageUpgradeSpecification(ServicePackageUpgradeSpecification && other);

        __declspec(property(get=get_ServicePackageName, put=put_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName() const { return packageName_; }
        void put_ServicePackageName(std::wstring const & value) { packageName_ = value; }

        __declspec(property(get=get_RolloutVersion, put=put_RolloutVersion)) RolloutVersion const & RolloutVersionValue;
        RolloutVersion const & get_RolloutVersion() const { return packageRolloutVersion_; }
        void put_RolloutVersion(RolloutVersion const & value) { packageRolloutVersion_ = value; }

        __declspec(property(get = get_isCodePackageInfoIncluded)) bool IsCodePackageInfoIncluded;
        bool get_isCodePackageInfoIncluded() const { return isCodePackageInfoIncluded_; }

        __declspec(property(get=get_CodePackageNames)) std::vector<std::wstring> const & CodePackageNames;
        std::vector<std::wstring> const & get_CodePackageNames() const { return codePackageNames_; }

        ServicePackageUpgradeSpecification const & operator = (ServicePackageUpgradeSpecification const & other);
        ServicePackageUpgradeSpecification const & operator = (ServicePackageUpgradeSpecification && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(packageName_, packageRolloutVersion_, isCodePackageInfoIncluded_, codePackageNames_);

    private:
        std::wstring packageName_;
        RolloutVersion packageRolloutVersion_;

        bool isCodePackageInfoIncluded_;
        std::vector<std::wstring> codePackageNames_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServicePackageUpgradeSpecification);
