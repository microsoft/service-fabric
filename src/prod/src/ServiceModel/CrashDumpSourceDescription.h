// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<CrashDumpSource>
    struct DiagnosticsDescription;
    struct CrashDumpSourceDescription
    {
    public:
        CrashDumpSourceDescription();
        CrashDumpSourceDescription(CrashDumpSourceDescription const & other);
        CrashDumpSourceDescription(CrashDumpSourceDescription && other);

        CrashDumpSourceDescription const & operator = (CrashDumpSourceDescription const & other);
        CrashDumpSourceDescription const & operator = (CrashDumpSourceDescription && other);

        bool operator == (CrashDumpSourceDescription const & other) const;
        bool operator != (CrashDumpSourceDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring IsEnabled;
        CrashDumpSourceDestinationsDescription Destinations;
        ParametersDescription Parameters;

    private:
        friend struct DiagnosticsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
