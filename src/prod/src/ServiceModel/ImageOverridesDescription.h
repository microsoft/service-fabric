// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ImageOverridesDescription : public Serialization::FabricSerializable
    {
    public:
        ImageOverridesDescription();
        ImageOverridesDescription(ImageOverridesDescription const & other) = default;
        ImageOverridesDescription(ImageOverridesDescription && other) = default;

        ImageOverridesDescription & operator = (ImageOverridesDescription const & other) = default;
        ImageOverridesDescription & operator = (ImageOverridesDescription && other) = default;

        bool operator == (ImageOverridesDescription const & other) const;
        bool operator != (ImageOverridesDescription const & other) const;
        
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_01(Images);

    public:
        std::vector<ImageTypeDescription> Images;

    private:
        friend struct ContainerPoliciesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}

