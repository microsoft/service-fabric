// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <LogCollectionPolicy> element.
    struct ApplicationPoliciesDescription;
	struct ApplicationManifestPoliciesDescription;
    struct LogCollectionPolicyDescription
    {
    public:
        LogCollectionPolicyDescription(); 
        LogCollectionPolicyDescription(LogCollectionPolicyDescription const & other);
        LogCollectionPolicyDescription(LogCollectionPolicyDescription && other);

        LogCollectionPolicyDescription const & operator = (LogCollectionPolicyDescription const & other);
        LogCollectionPolicyDescription const & operator = (LogCollectionPolicyDescription && other);

        bool operator == (LogCollectionPolicyDescription const & other) const;
        bool operator != (LogCollectionPolicyDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();
    public:
        std::wstring Path;

    private:
        friend struct ApplicationPoliciesDescription;
		friend struct ApplicationManifestPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

    };
}
