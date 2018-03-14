// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ImageOverridesDescription;
    // <Image> element.
    struct ImageTypeDescription : 
        public Serialization::FabricSerializable
    {
    public:
        ImageTypeDescription(); 

        ImageTypeDescription(ImageTypeDescription const & other) = default;
        ImageTypeDescription(ImageTypeDescription && other) = default;

        ImageTypeDescription & operator = (ImageTypeDescription const & other) = default;
        ImageTypeDescription & operator = (ImageTypeDescription && other) = default;

        bool operator == (ImageTypeDescription const & other) const;
        bool operator != (ImageTypeDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_02(Name, Os);

    public:
        std::wstring Name;
        std::wstring Os;

    private:
        friend struct ServiceModel::ImageOverridesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::ImageTypeDescription);

