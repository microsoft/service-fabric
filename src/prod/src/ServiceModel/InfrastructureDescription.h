// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <InfrastructureDescription> (InfrastructureDescriptionType) element.
    struct Parser;
    struct InfrastructureDescription
    {
    public:
        InfrastructureDescription();
        InfrastructureDescription(InfrastructureDescription const & other);
        InfrastructureDescription(InfrastructureDescription && other);

        InfrastructureDescription const & operator = (InfrastructureDescription const & other);
        InfrastructureDescription const & operator = (InfrastructureDescription && other);

        bool operator == (InfrastructureDescription const & other) const;
        bool operator != (InfrastructureDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        Common::ErrorCode FromXml(std::wstring const & fileName);

        void clear();

    private:
        friend struct Parser;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseNodeList(Common::XmlReaderUPtr const & xmlReader);

    public:
        std::vector<InfrastructureNodeDescription> NodeList;
    };
}
