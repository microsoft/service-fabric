// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // <Application><Policies> element.
    struct DigestedEnvironmentDescription;
    struct ApplicationManifestDescription;
    struct ApplicationPoliciesDescription
    {
    public:
        ApplicationPoliciesDescription();
        ApplicationPoliciesDescription(ApplicationPoliciesDescription const & other);
        ApplicationPoliciesDescription(ApplicationPoliciesDescription && other);

        ApplicationPoliciesDescription const & operator = (ApplicationPoliciesDescription const & other);
        ApplicationPoliciesDescription const & operator = (ApplicationPoliciesDescription && other);

        bool operator == (ApplicationPoliciesDescription const & other) const;
        bool operator != (ApplicationPoliciesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        // Optional parameters;
        // if they don't exist, their attributes are defaulted to empty
        DefaultRunAsPolicyDescription DefaultRunAs;
        std::vector<LogCollectionPolicyDescription> LogCollectionEntries;
        ApplicationHealthPolicy HealthPolicy;
        std::vector<SecurityAccessPolicyDescription> SecurityAccessPolicies;

    private:
        friend struct DigestedEnvironmentDescription;
        friend struct ApplicationManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
