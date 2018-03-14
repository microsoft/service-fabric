// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace HealthManager
    {
        class HealthReportResult
            : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(HealthReportResult)
            
        public:
            HealthReportResult();

            explicit HealthReportResult(
                ServiceModel::HealthReport const & healthReport);

            HealthReportResult(
                ServiceModel::HealthReport const & healthReport, 
                Common::ErrorCodeValue::Enum error);

            HealthReportResult(HealthReportResult && other);
            HealthReportResult & operator = (HealthReportResult && other);

            __declspec(property(get=get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
            FABRIC_HEALTH_REPORT_KIND get_Kind() const { return kind_; }

            __declspec(property(get=get_SourceId)) std::wstring const& SourceId;
            std::wstring const& get_SourceId() const { return sourceId_; }

            _declspec(property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return sequenceNumber_; }

            __declspec(property(get=get_Error, put=set_Error)) Common::ErrorCodeValue::Enum Error;
            Common::ErrorCodeValue::Enum get_Error() const { return error_; }
            void set_Error(Common::ErrorCodeValue::Enum value) { error_ = value; }

            __declspec(property(get=get_EntityPropertyId)) std::wstring const& EntityPropertyId;
            std::wstring const& get_EntityPropertyId() const { return entityPropertyId_; }
            
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_05(kind_, sourceId_, sequenceNumber_, entityPropertyId_, error_);

        private:
            FABRIC_HEALTH_REPORT_KIND kind_;
            std::wstring entityPropertyId_;
            std::wstring sourceId_;
            FABRIC_SEQUENCE_NUMBER sequenceNumber_;
       
            // Result
            Common::ErrorCodeValue::Enum error_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::HealthManager::HealthReportResult);
