// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ServiceGroupTypeDescription;
    struct ServiceGroupTypeMemberDescription : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
    {
    public:
        ServiceGroupTypeMemberDescription();
        ServiceGroupTypeMemberDescription(ServiceGroupTypeMemberDescription const & other);
        ServiceGroupTypeMemberDescription(ServiceGroupTypeMemberDescription && other);

        ServiceGroupTypeMemberDescription const & operator = (ServiceGroupTypeMemberDescription const & other);
        ServiceGroupTypeMemberDescription const & operator = (ServiceGroupTypeMemberDescription && other);

        bool operator == (ServiceGroupTypeMemberDescription const & other) const;
        bool operator != (ServiceGroupTypeMemberDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION & publicDescription) const;
        
        void clear();

        FABRIC_FIELDS_02(ServiceTypeName, LoadMetrics);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeName, ServiceTypeName)
            SERIALIZABLE_PROPERTY(Constants::LoadMetrics, LoadMetrics)
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::wstring ServiceTypeName;
        std::vector<ServiceLoadMetricDescription> LoadMetrics;

    private:
        friend struct ServiceGroupTypeDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        void ParseLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader);

		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteLoadBalancingMetrics(Common::XmlWriterUPtr const &);
    };

    struct ServiceManifestDescription;
    struct ServiceGroupTypeDescription : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
    {
    public:
        ServiceGroupTypeDescription();
        ServiceGroupTypeDescription(ServiceGroupTypeDescription const & other);
        ServiceGroupTypeDescription(ServiceGroupTypeDescription && other);

        ServiceGroupTypeDescription const & operator = (ServiceGroupTypeDescription const & other);
        ServiceGroupTypeDescription const & operator = (ServiceGroupTypeDescription && other);

        bool operator == (ServiceGroupTypeDescription const & other) const;
        bool operator != (ServiceGroupTypeDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION & publicDescription) const;
        static void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __in std::vector<ServiceGroupTypeDescription> const & serviceGroupTypes,
            __out FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST & publicServiceGroupTypes);

        void clear();

        FABRIC_FIELDS_03(UseImplicitFactory, Description, Members);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeDescription, Description)
            SERIALIZABLE_PROPERTY(Constants::ServiceGroupTypeMemberDescription, Members)
            SERIALIZABLE_PROPERTY(Constants::UseImplicitFactory, UseImplicitFactory)
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        bool UseImplicitFactory;
        ServiceTypeDescription Description;
        std::vector<ServiceGroupTypeMemberDescription> Members;

    private:
        friend struct ServiceManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &, bool isStateful);
        void ParseServiceGroupMembers(Common::XmlReaderUPtr const & xmlReader);

        void ParsePlacementConstraints(Common::XmlReaderUPtr const & xmlReader);
        void ParseExtensions(Common::XmlReaderUPtr const & xmlReader);

        void ParseLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader);



		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteServiceGroupMembers(Common::XmlWriterUPtr const & xmlWriter);

		Common::ErrorCode WritePlacementConstraints(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteExtensions(Common::XmlWriterUPtr const & xmlWriter);

		Common::ErrorCode WriteLoadBalancingMetrics(Common::XmlWriterUPtr const & xmlWriter);


        void CombineLoadBalancingMetrics(Common::XmlReaderUPtr const & xmlReader);
        uint CombineMetric(uint lhs, uint rhs, Common::XmlReaderUPtr const & xmlReader);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceGroupTypeMemberDescription);
DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceGroupTypeDescription);
