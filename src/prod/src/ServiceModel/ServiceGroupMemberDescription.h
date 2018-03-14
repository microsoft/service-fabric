// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationServiceDescription;
	struct ApplicationServiceTemplateDescription;
    struct ServiceGroupMemberDescription
    {
    public:
        ServiceGroupMemberDescription();
        ServiceGroupMemberDescription(ServiceGroupMemberDescription const & other);
        ServiceGroupMemberDescription(ServiceGroupMemberDescription && other);

        ServiceGroupMemberDescription const & operator = (ServiceGroupMemberDescription const & other);
        ServiceGroupMemberDescription const & operator = (ServiceGroupMemberDescription && other);

        bool operator == (ServiceGroupMemberDescription const & other) const;
        bool operator != (ServiceGroupMemberDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring ServiceTypeName;
        std::wstring MemberName;
        std::vector<ServiceLoadMetricDescription> LoadMetrics;

    private:
        friend struct ApplicationServiceDescription;
		friend struct ApplicationServiceTemplateDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
        void ParseLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader);
    };
}
 
