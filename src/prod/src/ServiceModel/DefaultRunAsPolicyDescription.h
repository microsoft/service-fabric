// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DefaultDefaultRunAsPolicy> element.
    struct ApplicationPoliciesDescription;
	struct ApplicationManifestPoliciesDescription;
    struct DefaultRunAsPolicyDescription
    {
    public:
        DefaultRunAsPolicyDescription(); 
        DefaultRunAsPolicyDescription(DefaultRunAsPolicyDescription const & other);
        DefaultRunAsPolicyDescription(DefaultRunAsPolicyDescription && other);

        DefaultRunAsPolicyDescription const & operator = (DefaultRunAsPolicyDescription const & other);
        DefaultRunAsPolicyDescription const & operator = (DefaultRunAsPolicyDescription && other);

        bool operator == (DefaultRunAsPolicyDescription const & other) const;
        bool operator != (DefaultRunAsPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring UserRef;

    private:
        friend struct ApplicationPoliciesDescription;
		friend struct ApplicationManifestPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
