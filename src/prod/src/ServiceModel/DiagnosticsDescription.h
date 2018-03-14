// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>
    struct DigestedEnvironmentDescription;
	struct ApplicationManifestDescription;
    struct DiagnosticsDescription
    {
    public:
        DiagnosticsDescription();
        DiagnosticsDescription(DiagnosticsDescription const & other);
        DiagnosticsDescription(DiagnosticsDescription && other);

        DiagnosticsDescription const & operator = (DiagnosticsDescription const & other);
        DiagnosticsDescription const & operator = (DiagnosticsDescription && other);

        bool operator == (DiagnosticsDescription const & other) const;
        bool operator != (DiagnosticsDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        CrashDumpSourceDescription CrashDumpSource;
        ETWSourceDescription ETWSource;
        std::vector<FolderSourceDescription> FolderSource;

    private:
        friend struct DigestedEnvironmentDescription;
		friend struct ApplicationManifestDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);

    };
}
