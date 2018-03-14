// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<ETWSource>
    struct DiagnosticsDescription;
    struct ETWSourceDescription
    {
    public:
        ETWSourceDescription();
        ETWSourceDescription(ETWSourceDescription const & other);
        ETWSourceDescription(ETWSourceDescription && other);

        ETWSourceDescription const & operator = (ETWSourceDescription const & other);
        ETWSourceDescription const & operator = (ETWSourceDescription && other);

        bool operator == (ETWSourceDescription const & other) const;
        bool operator != (ETWSourceDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring IsEnabled;
        ETWSourceDestinationsDescription Destinations;
        ParametersDescription Parameters;

    private:
        friend struct DiagnosticsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
