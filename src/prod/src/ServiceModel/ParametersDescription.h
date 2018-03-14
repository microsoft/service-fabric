// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/.../<Parameters>
    struct CrashDumpSourceDescription;
    struct AzureStoreBaseDescription;
    struct FileStoreDescription;
    struct LocalStoreDescription;
    struct ETWSourceDescription;
    struct FolderSourceDescription;
    struct ParametersDescription
    {
    public:
        ParametersDescription();
        ParametersDescription(ParametersDescription const & other);
        ParametersDescription(ParametersDescription && other);

        ParametersDescription const & operator = (ParametersDescription const & other);
        ParametersDescription const & operator = (ParametersDescription && other);

        bool operator == (ParametersDescription const & other) const;
        bool operator != (ParametersDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::vector<ParameterDescription> Parameter;
		bool empty();

    private:
        friend struct CrashDumpSourceDescription;
        friend struct AzureStoreBaseDescription;
        friend struct FileStoreDescription;
        friend struct LocalStoreDescription;
        friend struct ETWSourceDescription;
        friend struct FolderSourceDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
