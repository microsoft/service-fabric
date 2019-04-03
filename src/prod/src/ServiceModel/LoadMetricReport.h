// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class LoadMetricReport
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(LoadMetricReport)
        DEFAULT_COPY_ASSIGNMENT(LoadMetricReport)
        
    public:
        LoadMetricReport(
            std::wstring const & name,
            uint value,
            double currentValue,
            Common::DateTime lastReportedTime);

        LoadMetricReport();

        LoadMetricReport(LoadMetricReport && other);

        LoadMetricReport & operator=(LoadMetricReport && other);
        
        __declspec(property(get = get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return name_; }

        __declspec(property(get = get_Value)) uint Value;
        uint get_Value() const { return value_; }

        __declspec(property(get = get_CurrentValue)) double CurrentValue;
        double get_CurrentValue() const { return currentValue_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_LOAD_METRIC_REPORT & publicResult) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_LOAD_METRIC_REPORT const& publicResult);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_04(
            name_, 
            value_, 
            lastReportedTime_,
            currentValue_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, name_)
            SERIALIZABLE_PROPERTY(Constants::Value, value_)
            SERIALIZABLE_PROPERTY(Constants::LastReportedUtc, lastReportedTime_)
            SERIALIZABLE_PROPERTY(Constants::CurrentValue, currentValue_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring name_;
        uint value_;
        double currentValue_;
        Common::DateTime lastReportedTime_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::LoadMetricReport);
