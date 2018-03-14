// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<FolderSource>
    struct DiagnosticsDescription;
    struct FolderSourceDescription
    {
    public:
        FolderSourceDescription();
        FolderSourceDescription(FolderSourceDescription const & other);
        FolderSourceDescription(FolderSourceDescription && other);

        FolderSourceDescription const & operator = (FolderSourceDescription const & other);
        FolderSourceDescription const & operator = (FolderSourceDescription && other);

        bool operator == (FolderSourceDescription const & other) const;
        bool operator != (FolderSourceDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring IsEnabled;
        std::wstring RelativeFolderPath;
        std::wstring DataDeletionAgeInDays;
        FolderSourceDestinationsDescription Destinations;
        ParametersDescription Parameters;

    private:
        friend struct DiagnosticsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
