// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct DigestedServiceTypesDescription;
    struct ServiceManifestDescription;
    struct ServiceTypeDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceTypeDescription();
        ServiceTypeDescription(ServiceTypeDescription const & other);
        ServiceTypeDescription(ServiceTypeDescription && other);

        ServiceTypeDescription const & operator = (ServiceTypeDescription const & other);
        ServiceTypeDescription const & operator = (ServiceTypeDescription && other);

        bool operator == (ServiceTypeDescription const & other) const;
        bool operator != (ServiceTypeDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        static void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in std::vector<ServiceTypeDescription> const & serviceTypes,
            __out FABRIC_SERVICE_TYPE_DESCRIPTION_LIST & publicServiceTypes);

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_TYPE_DESCRIPTION & publicDescription) const;
        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_TYPE_DESCRIPTION const & publicDescription);
        void clear();

        std::wstring ToString() const;

         BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZE_CONSTANT_IF(Constants::Kind, L"Stateful", IsStateful)
            SERIALIZE_CONSTANT_IF(Constants::Kind, L"Stateless", !IsStateful)
            SERIALIZABLE_PROPERTY(Constants::IsStateful, IsStateful)
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeName, ServiceTypeName)
            SERIALIZABLE_PROPERTY(Constants::PlacementConstraints, PlacementConstraints)
            SERIALIZABLE_PROPERTY_IF(Constants::HasPersistedState, HasPersistedState, IsStateful)
            SERIALIZABLE_PROPERTY_IF(Constants::UseImplicitHost, UseImplicitHost, !IsStateful)
            SERIALIZABLE_PROPERTY(Constants::Extensions, Extensions)
            SERIALIZABLE_PROPERTY(Constants::LoadMetrics, LoadMetrics)
            SERIALIZABLE_PROPERTY(Constants::ServicePlacementPolicies, ServicePlacementPolicies)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_08(IsStateful, ServiceTypeName, PlacementConstraints, HasPersistedState, UseImplicitHost, Extensions, LoadMetrics, ServicePlacementPolicies);

    public:
        bool IsStateful;
        std::wstring ServiceTypeName;
        std::wstring PlacementConstraints;
        bool HasPersistedState;
        bool UseImplicitHost;
        std::vector<DescriptionExtension> Extensions;
        std::vector<ServiceLoadMetricDescription> LoadMetrics;
        std::vector<ServicePlacementPolicyDescription> ServicePlacementPolicies;

    private:
        friend struct DigestedServiceTypesDescription;
        friend struct ServiceManifestDescription;

		void ReadFromXml(Common::XmlReaderUPtr const &, bool isStateful);
        void ParsePlacementConstraints(Common::XmlReaderUPtr const & xmlReader);
        void ParseLoadBalancingMetics(Common::XmlReaderUPtr const & xmlReader);
        void ParseExtensions(Common::XmlReaderUPtr const & xmlReader);
        void ParseServicePlacementPolicies(Common::XmlReaderUPtr const & xmlReader);
		
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
		Common::ErrorCode WritePlacementConstraints(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteLoadBalancingMetics(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteExtensions(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteServicePlacementPolicies(Common::XmlWriterUPtr const & xmlWriter);

    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceTypeDescription);
