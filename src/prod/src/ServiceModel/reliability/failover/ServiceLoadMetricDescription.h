// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceLoadMetricDescription 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceLoadMetricDescription();

        ServiceLoadMetricDescription(
            std::wstring const& name, 
            FABRIC_SERVICE_LOAD_METRIC_WEIGHT weight, 
            uint primaryDefaultLoad,
            uint secondaryDefaultLoad);

        ServiceLoadMetricDescription(ServiceLoadMetricDescription const& other);

        ServiceLoadMetricDescription(ServiceLoadMetricDescription && other);

        ServiceLoadMetricDescription & operator = (ServiceLoadMetricDescription const& other);

        ServiceLoadMetricDescription & operator = (ServiceLoadMetricDescription && other);

        bool operator == (ServiceLoadMetricDescription const & other) const;
        bool operator != (ServiceLoadMetricDescription const & other) const;

        __declspec (property(get=get_Name)) std::wstring const& Name;
        std::wstring const& get_Name() const { return name_; }

        __declspec (property(get=get_Weight)) FABRIC_SERVICE_LOAD_METRIC_WEIGHT Weight;
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT get_Weight() const { return weight_; }

        __declspec (property(get=get_PrimaryDefaultLoad)) uint PrimaryDefaultLoad;
        uint get_PrimaryDefaultLoad() const { return primaryDefaultLoad_; }

        __declspec (property(get=get_SecondaryDefaultLoad)) uint SecondaryDefaultLoad;
        uint get_SecondaryDefaultLoad() const { return secondaryDefaultLoad_; }

        void WriteToEtw(uint16 contextSequenceId) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(name_, weight_, primaryDefaultLoad_, secondaryDefaultLoad_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, name_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Weight, weight_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PrimaryDefaultLoad, primaryDefaultLoad_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::SecondaryDefaultLoad, secondaryDefaultLoad_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::DefaultLoad, primaryDefaultLoad_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:

        std::wstring name_;
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT weight_;
        uint primaryDefaultLoad_;
        uint secondaryDefaultLoad_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ServiceLoadMetricDescription);
