// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class HealthReportSendOptions
        : public Serialization::FabricSerializable
    {
        DENY_COPY(HealthReportSendOptions)
    public:
        HealthReportSendOptions();
        explicit HealthReportSendOptions(bool immediate);
        
        HealthReportSendOptions(HealthReportSendOptions && other) = default;
        HealthReportSendOptions & operator = (HealthReportSendOptions && other) = default;

        ~HealthReportSendOptions();

        __declspec(property(get=get_Immediate)) bool Immediate;
        bool get_Immediate() const { return immediate_; }

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __inout FABRIC_HEALTH_REPORT_SEND_OPTIONS & sendOptions) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT_SEND_OPTIONS const & sendOptions);

        FABRIC_FIELDS_01(immediate_);
      
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;
        
    private:
        bool immediate_;
    };

    using HealthReportSendOptionsUPtr = std::unique_ptr<HealthReportSendOptions>;
}
