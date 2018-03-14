// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <Node> (InfrastructureNodeType) element.
    struct InfrastructureDescription;
    struct InfrastructureNodeDescription
    {
    public:
        InfrastructureNodeDescription();
        InfrastructureNodeDescription(InfrastructureNodeDescription const & other);
        InfrastructureNodeDescription(InfrastructureNodeDescription && other);

        InfrastructureNodeDescription const & operator = (InfrastructureNodeDescription const & other);
        InfrastructureNodeDescription const & operator = (InfrastructureNodeDescription && other);

        bool operator == (InfrastructureNodeDescription const & other) const;
        bool operator != (InfrastructureNodeDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring NodeName;
        std::wstring NodeTypeRef;
        std::wstring RoleOrTierName;
        bool IsSeedNode;
        std::wstring IPAddressOrFQDN;
        std::wstring FaultDomain;
        std::wstring UpgradeDomain;

    private:
        friend struct InfrastructureDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
    };
}
