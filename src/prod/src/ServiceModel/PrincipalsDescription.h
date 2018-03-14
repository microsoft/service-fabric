// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Environment>/<Principals>
    struct DigestedEnvironmentDescription;
	struct ApplicationManifestDescription;
    struct PrincipalsDescription : public Serialization::FabricSerializable
    {
    public:
        PrincipalsDescription();
        PrincipalsDescription(PrincipalsDescription const & other);
        PrincipalsDescription(PrincipalsDescription && other);

        PrincipalsDescription const & operator = (PrincipalsDescription const & other);
        PrincipalsDescription const & operator = (PrincipalsDescription && other);

        bool operator == (PrincipalsDescription const & other) const;
        bool operator != (PrincipalsDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        void ToPublicApi(__in Common::ScopedHeap & heap, __in std::map<std::wstring, std::wstring> const& sids, __out FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION & publicDescription) const;

        static bool WriteSIDsToFile(std::wstring const& fileName, std::map<std::wstring, std::wstring> const& sids);
        static bool ReadSIDsFromFile(std::wstring const& fileName, std::map<std::wstring, std::wstring> & sids);        

        void clear();

        FABRIC_FIELDS_02(Users, Groups);

    public:
        std::vector<SecurityGroupDescription> Groups;
        std::vector<SecurityUserDescription> Users;

    private:
        friend struct DigestedEnvironmentDescription;
		friend struct ApplicationManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseGroups(Common::XmlReaderUPtr const & xmlReader);
        void ParseUsers(Common::XmlReaderUPtr const & xmlReader);


		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteGroups(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteUsers(Common::XmlWriterUPtr const & xmlWriter);
    };

    using PrincipalsDescriptionUPtr = std::unique_ptr<PrincipalsDescription>;
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::PrincipalsDescription);
