// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/<ETWSource>/<LocalStore>
    struct ETWSourceDestinationsDescription;
    struct LocalStoreETWDescription : LocalStoreDescription
    {
    public:
        LocalStoreETWDescription();
        LocalStoreETWDescription(LocalStoreETWDescription const & other);
        LocalStoreETWDescription(LocalStoreETWDescription && other);

        LocalStoreETWDescription const & operator = (LocalStoreETWDescription const & other);
        LocalStoreETWDescription const & operator = (LocalStoreETWDescription && other);

        bool operator == (LocalStoreETWDescription const & other) const;
        bool operator != (LocalStoreETWDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring LevelFilter;

    private:
        friend struct ETWSourceDestinationsDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
