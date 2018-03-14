// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceTypeRemovalSpecification : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(ServiceTypeRemovalSpecification)
        DEFAULT_COPY_ASSIGNMENT(ServiceTypeRemovalSpecification)
        
    public:
        ServiceTypeRemovalSpecification();
        ServiceTypeRemovalSpecification(
            std::wstring const & packageName, 
            RolloutVersion const & rolloutVersion);
        ServiceTypeRemovalSpecification(ServiceTypeRemovalSpecification && other);

        __declspec(property(get=get_ServicePackageName, put=put_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName() const { return packageName_; }
        void put_ServicePackageName(std::wstring const & value) { packageName_ = value; }

        __declspec(property(get=get_RolloutVersion, put=put_RolloutVersion)) RolloutVersion const & RolloutVersionValue;
        RolloutVersion const & get_RolloutVersion() const { return packageRolloutVersion_; }
        void put_RolloutVersion(RolloutVersion const & value) { packageRolloutVersion_ = value; }

        __declspec(property(get=get_ServiceTypeNames)) std::set<std::wstring> const & ServiceTypeNames;
        std::set<std::wstring> const & get_ServiceTypeNames() const { return typeNames_; }

        bool HasServiceTypeName(std::wstring const & serviceTypeName) const;

        ServiceTypeRemovalSpecification const & operator = (ServiceTypeRemovalSpecification && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        void AddServiceTypeName(std::wstring const & typeName);

        FABRIC_FIELDS_03(packageName_, packageRolloutVersion_, typeNames_);

    private:
        std::wstring packageName_;
        RolloutVersion packageRolloutVersion_;
        std::set<std::wstring> typeNames_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceTypeRemovalSpecification);

