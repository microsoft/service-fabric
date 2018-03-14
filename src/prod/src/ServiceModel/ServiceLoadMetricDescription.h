// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationServiceDescription;
    struct ServiceGroupTypeMemberDescription;
    struct ServiceGroupTypeDescription;
    struct ServiceGroupMemberDescription;
    struct ServiceTypeDescription;
	struct ApplicationServiceTemplateDescription;
    struct ServiceLoadMetricDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceLoadMetricDescription();
        ServiceLoadMetricDescription(std::wstring name, 
            WeightType::Enum weight,
            uint primaryDefaultLoad,
            uint secondaryDefaultLoad);
        ServiceLoadMetricDescription(ServiceLoadMetricDescription const & other);
        ServiceLoadMetricDescription(ServiceLoadMetricDescription && other);

        ServiceLoadMetricDescription const & operator = (ServiceLoadMetricDescription const & other);
        ServiceLoadMetricDescription const & operator = (ServiceLoadMetricDescription && other);

        bool operator == (ServiceLoadMetricDescription const & other) const;
        bool operator != (ServiceLoadMetricDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION & publicDescription) const;
        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_LOAD_METRIC_DESCRIPTION const & publicDescription);

        void clear();

        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, Name)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Weight, Weight)
            SERIALIZABLE_PROPERTY(Constants::PrimaryDefaultLoad, PrimaryDefaultLoad)
            SERIALIZABLE_PROPERTY(Constants::SecondaryDefaultLoad, SecondaryDefaultLoad)
            SERIALIZABLE_PROPERTY(Constants::DefaultLoad, PrimaryDefaultLoad)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(PrimaryDefaultLoad, SecondaryDefaultLoad, Name, Weight);

    public:
        uint PrimaryDefaultLoad;
        uint SecondaryDefaultLoad;
        std::wstring Name;
        WeightType::Enum Weight;

    private:
        friend struct ApplicationServiceDescription;
        friend struct ServiceGroupTypeMemberDescription;
        friend struct ServiceGroupTypeDescription;
        friend struct ServiceGroupMemberDescription;
        friend struct ServiceTypeDescription;
		friend struct ApplicationServiceTemplateDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceLoadMetricDescription);
