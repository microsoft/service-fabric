// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class VersionedServicePackageIdentifier : public Serialization::FabricSerializable
    {
    public:
        VersionedServicePackageIdentifier();
        VersionedServicePackageIdentifier(ServicePackageIdentifier const & id, ServicePackageVersion const & version);
        VersionedServicePackageIdentifier(VersionedServicePackageIdentifier const & other);
        VersionedServicePackageIdentifier(VersionedServicePackageIdentifier && other);

        __declspec(property(get=get_Id)) ServicePackageIdentifier const & Id;
        inline ServicePackageIdentifier const & get_Id() const { return id_; };
        
        __declspec(property(get=get_Version)) ServicePackageVersion const & Version;
        inline ServicePackageVersion const & get_Version() const { return version_; };

        VersionedServicePackageIdentifier const & operator = (VersionedServicePackageIdentifier const & other);
        VersionedServicePackageIdentifier const & operator = (VersionedServicePackageIdentifier && other);

        bool operator == (VersionedServicePackageIdentifier const & other) const;
        bool operator != (VersionedServicePackageIdentifier const & other) const;

        int compare(VersionedServicePackageIdentifier const & other) const;
        bool operator < (VersionedServicePackageIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        
        FABRIC_FIELDS_02(id_, version_);

    private:
        ServicePackageIdentifier id_;
        ServicePackageVersion version_;
    };
}
