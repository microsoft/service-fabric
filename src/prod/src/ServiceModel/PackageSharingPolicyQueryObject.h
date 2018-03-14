// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PackageSharingPolicyQueryObject
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(PackageSharingPolicyQueryObject)

    public:
        PackageSharingPolicyQueryObject();
        PackageSharingPolicyQueryObject(
            std::wstring const & packageName,
            ServicePackageSharingType::Enum sharingScope_);
        PackageSharingPolicyQueryObject(PackageSharingPolicyQueryObject && other);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec(property(get=get_SharedPackageName)) std::wstring const & SharedPackageName;
       std::wstring const& get_SharedPackageName() const { return packageName_; }

       __declspec(property(get=get_PackageSharingPolicyScope)) ServicePackageSharingType::Enum PackageSharingPolicyScope;
       ServicePackageSharingType::Enum get_PackageSharingPolicyScope() const { return (ServicePackageSharingType::Enum)sharingScope_; }

        PackageSharingPolicyQueryObject const & operator= (PackageSharingPolicyQueryObject && other);

        FABRIC_FIELDS_02(packageName_, sharingScope_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SharedPackageName, packageName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PackageSharingScope, sharingScope_);
        END_JSON_SERIALIZABLE_PROPERTIES()     


    private:
        std::wstring packageName_;
        DWORD sharingScope_;
    };
 /*   DEFINE_USER_ARRAY_UTILITY(PackageSharingPolicyQueryObject);*/
}
