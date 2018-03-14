// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ServiceGroupTypeDescription;
    struct ServiceTypeDescription;
    struct DescriptionExtension 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DescriptionExtension();
        DescriptionExtension(std::wstring name, std::wstring value);
        DescriptionExtension(DescriptionExtension const & other);
        DescriptionExtension(DescriptionExtension && other);

        DescriptionExtension const & operator = (DescriptionExtension const & other);
        DescriptionExtension const & operator = (DescriptionExtension && other);

        bool operator == (DescriptionExtension const & other) const;
        bool operator != (DescriptionExtension const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION & publicDescription) const;
        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION const & publicDescription);

        void clear();

        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Key, Name)
            SERIALIZABLE_PROPERTY(Constants::Value, Value)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_02(Name, Value);

    public:
        std::wstring Name;
        std::wstring Value;

    private:
        friend struct ServiceGroupTypeDescription;
        friend struct ServiceTypeDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DescriptionExtension);
