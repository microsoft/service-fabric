// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationServiceDescription;
	struct ApplicationServiceTemplateDescription;
    struct ServiceCorrelationDescription
    {
    public:
        ServiceCorrelationDescription();
        ServiceCorrelationDescription(ServiceCorrelationDescription const & other);
        ServiceCorrelationDescription(ServiceCorrelationDescription && other);

        ServiceCorrelationDescription const & operator = (ServiceCorrelationDescription const & other);
        ServiceCorrelationDescription const & operator = (ServiceCorrelationDescription && other);

        bool operator == (ServiceCorrelationDescription const & other) const;
        bool operator != (ServiceCorrelationDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring ServiceName;
        ServiceCorrelationScheme::Enum Scheme;

    private:
        friend struct ApplicationServiceDescription;
		friend struct ApplicationServiceTemplateDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		std::wstring EnumToString();
    };
}
