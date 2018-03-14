// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationLoadMetricInformation 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ApplicationLoadMetricInformation();

        ApplicationLoadMetricInformation(ApplicationLoadMetricInformation && other);
        ApplicationLoadMetricInformation(ApplicationLoadMetricInformation const & other);

        ApplicationLoadMetricInformation const & operator = (ApplicationLoadMetricInformation const & other);
        ApplicationLoadMetricInformation const & operator = (ApplicationLoadMetricInformation && other);
        
        __declspec(property(get=get_Name, put=put_Name)) std::wstring const& Name;
        std::wstring const& get_Name() const { return name_; }
        void put_Name(std::wstring && name) { name_ = std::move(name); }

        __declspec(property(get = get_ReservationCapacity, put = put_ReservationCapacity)) int64 ReservationCapacity;
        int64 get_ReservationCapacity() const { return reservationCapacity_; }
        void put_ReservationCapacity(int64 reservationCapacity) { reservationCapacity_ = reservationCapacity; }

        __declspec(property(get = get_ApplicationCapacity, put = put_ApplicationCapacity)) int64 ApplicationCapacity;
        int64 get_ApplicationCapacity() const { return applicationCapacity_; }
        void put_ApplicationCapacity(int64 applicationCapacity) { applicationCapacity_ = applicationCapacity; }

        __declspec(property(get = get_ApplicationLoad, put = put_ApplicationLoad)) int64 ApplicationLoad;
        int64 get_ApplicationLoad() const { return applicationLoad_; }
        void put_ApplicationLoad(int64 applicationLoad) { applicationLoad_ = applicationLoad; }

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_APPLICATION_LOAD_METRIC_INFORMATION  &) const;
        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_LOAD_METRIC_INFORMATION  const &);
        
        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, name_)
            SERIALIZABLE_PROPERTY(Constants::ReservationCapacity, reservationCapacity_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationCapacity, applicationCapacity_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationLoad, applicationLoad_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(name_,
            reservationCapacity_,
            applicationCapacity_,
            applicationLoad_);

    private:
        std::wstring name_;
        int64 reservationCapacity_;
        int64 applicationCapacity_;
        int64 applicationLoad_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ApplicationLoadMetricInformation);

