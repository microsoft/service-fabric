// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DigestedEnvironment> element.
    struct ApplicationManifestDescription;
    struct ApplicationPackageDescription;
    struct DigestedEnvironmentDescription
    {
        DigestedEnvironmentDescription();
        DigestedEnvironmentDescription(DigestedEnvironmentDescription const & other);
        DigestedEnvironmentDescription(DigestedEnvironmentDescription && other);

        DigestedEnvironmentDescription const & operator = (DigestedEnvironmentDescription const & other);
        DigestedEnvironmentDescription const & operator = (DigestedEnvironmentDescription && other);

        bool operator == (DigestedEnvironmentDescription const & other) const;
        bool operator != (DigestedEnvironmentDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();
    public:
        RolloutVersion RolloutVersionValue;
        PrincipalsDescription Principals;
        ApplicationPoliciesDescription Policies;
        DiagnosticsDescription Diagnostics;

    private:
        friend struct ApplicationManifestDescription;
        friend struct ApplicationPackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
