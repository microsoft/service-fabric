// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ApplicationMetricDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationMetricDescription();

        ApplicationMetricDescription(
            std::wstring const& name,
            uint reservationNodeCapacity,
            uint maximumNodeCapacity,
            uint totalApplicationCapacity);

        ApplicationMetricDescription(ApplicationMetricDescription const& other);

        ApplicationMetricDescription(ApplicationMetricDescription && other);

        ApplicationMetricDescription & operator = (ApplicationMetricDescription const& other);

        ApplicationMetricDescription & operator = (ApplicationMetricDescription && other);

        bool operator == (ApplicationMetricDescription const & other) const;
        bool operator != (ApplicationMetricDescription const & other) const;

        __declspec (property(get = get_Name)) std::wstring const& Name;
        std::wstring const& get_Name() const { return name_; }

        __declspec(property(get = get_ReservationNodeCapacity)) uint ReservationNodeCapacity;
        uint get_ReservationNodeCapacity() const { return reservationNodeCapacity_; }

        __declspec(property(get = get_TotalApplicationCapacity)) uint TotalApplicationCapacity;
        uint get_TotalApplicationCapacity() const { return totalApplicationCapacity_; }

        __declspec(property(get = get_MaximumNodeCapacity)) uint MaximumNodeCapacity;
        uint get_MaximumNodeCapacity() const { return maximumNodeCapacity_; }

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_METRIC_DESCRIPTION const & nativeDescription);

        bool TryValidate(__out std::wstring & validationErrorMessage, uint maximumNodes) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(name_, reservationNodeCapacity_, maximumNodeCapacity_, totalApplicationCapacity_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, name_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ReservationCapacity, reservationNodeCapacity_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaximumCapacity, maximumNodeCapacity_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::TotalApplicationCapacity, totalApplicationCapacity_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:

        std::wstring name_;
        uint reservationNodeCapacity_;
        uint maximumNodeCapacity_;
        uint totalApplicationCapacity_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ApplicationMetricDescription);
