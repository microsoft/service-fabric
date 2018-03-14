// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationInstance>/<Diagnostics>/.../<Parameters>/<Parameter>
    struct ParametersDescription;
    struct ParameterDescription
    {
    public:
        ParameterDescription();
        ParameterDescription(ParameterDescription const & other);
        ParameterDescription(ParameterDescription && other);

        ParameterDescription const & operator = (ParameterDescription const & other);
        ParameterDescription const & operator = (ParameterDescription && other);

        bool operator == (ParameterDescription const & other) const;
        bool operator != (ParameterDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring Name;
        std::wstring Value;
        std::wstring IsEncrypted;

    private:
        friend struct ParametersDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
