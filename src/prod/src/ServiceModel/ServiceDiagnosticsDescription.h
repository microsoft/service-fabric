// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <Diagnostics> element in service manifest and service package
    struct ServiceManifestDescription;
    struct ServicePackageDescription;
    struct ServiceDiagnosticsDescription
    {
    public:
        ServiceDiagnosticsDescription();
        ServiceDiagnosticsDescription(ServiceDiagnosticsDescription const & other);
        ServiceDiagnosticsDescription(ServiceDiagnosticsDescription && other);

        ServiceDiagnosticsDescription const & operator = (ServiceDiagnosticsDescription const & other);
        ServiceDiagnosticsDescription const & operator = (ServiceDiagnosticsDescription && other);

        bool operator == (ServiceDiagnosticsDescription const & other) const;
        bool operator != (ServiceDiagnosticsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        ETWDescription EtwDescription;

    private:
        friend struct ServiceManifestDescription;
        friend struct ServicePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
