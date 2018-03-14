// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PackageSharingPolicyList
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(PackageSharingPolicyList)

    public:
        PackageSharingPolicyList();
        PackageSharingPolicyList(
            std::vector<PackageSharingPolicyQueryObject> const &);
        PackageSharingPolicyList(PackageSharingPolicyList && other);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

       __declspec(property(get=get_PackageSharingPolicy)) std::vector<PackageSharingPolicyQueryObject> const & PackageSharingPolicy;
       std::vector<PackageSharingPolicyQueryObject> const& get_PackageSharingPolicy() const { return packageSharingPolicies_; }

        PackageSharingPolicyList const & operator= (PackageSharingPolicyList && other);

        FABRIC_FIELDS_01(packageSharingPolicies_)

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PackageSharingPolicy, packageSharingPolicies_)
            END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::vector<PackageSharingPolicyQueryObject> packageSharingPolicies_;
    };
}
