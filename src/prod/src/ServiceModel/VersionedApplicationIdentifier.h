// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class VersionedApplicationIdentifier : public Serialization::FabricSerializable
    {
    public:
        VersionedApplicationIdentifier();
        VersionedApplicationIdentifier(ApplicationIdentifier const & id, ApplicationVersion const & version);
        VersionedApplicationIdentifier(VersionedApplicationIdentifier const & other);
        VersionedApplicationIdentifier(VersionedApplicationIdentifier && other);

        __declspec(property(get=get_Id)) ApplicationIdentifier const & Id;
        inline ApplicationIdentifier const & get_Id() const { return id_; };
        
        __declspec(property(get=get_Version)) ApplicationVersion const & Version;
        inline ApplicationVersion const & get_Version() const { return version_; };

        VersionedApplicationIdentifier const & operator = (VersionedApplicationIdentifier const & other);
        VersionedApplicationIdentifier const & operator = (VersionedApplicationIdentifier && other);

        bool operator == (VersionedApplicationIdentifier const & other) const;
        bool operator != (VersionedApplicationIdentifier const & other) const;

        int compare(VersionedApplicationIdentifier const & other) const;
        bool operator < (VersionedApplicationIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        
        FABRIC_FIELDS_02(id_, version_);

    private:
        ApplicationIdentifier id_;
        ApplicationVersion version_;
    };
}
