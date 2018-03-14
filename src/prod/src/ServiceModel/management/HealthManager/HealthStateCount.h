// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class HealthStateCount
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::ISizeEstimator
        {
        public:
            HealthStateCount();
            HealthStateCount(ULONG okCount, ULONG warningCount, ULONG errorCount);

            ~HealthStateCount();

            HealthStateCount(HealthStateCount const & other);
            HealthStateCount & operator = (HealthStateCount const & other);

            HealthStateCount(HealthStateCount && other);
            HealthStateCount & operator = (HealthStateCount && other);
            
            __declspec(property(get = get_OkCount)) ULONG OkCount;
            ULONG get_OkCount() const { return okCount_; }

            __declspec(property(get = get_WarningCount)) ULONG WarningCount;
            ULONG get_WarningCount() const { return warningCount_; }

            __declspec(property(get = get_ErrorCount)) ULONG ErrorCount;
            ULONG get_ErrorCount() const { return errorCount_; }

            ULONG GetTotal() const { return okCount_ + warningCount_ + errorCount_; }

            bool IsEmpty() const { return okCount_ == 0 && warningCount_ == 0 && errorCount_ == 0; }

            void AppendCount(HealthStateCount const & other);

            void Add(FABRIC_HEALTH_STATE healthState);

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            void ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_HEALTH_STATE_COUNT & publicHealthCount) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_HEALTH_STATE_COUNT const & publicHealthCount);

            FABRIC_FIELDS_03(okCount_, warningCount_, errorCount_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::OkCount, okCount_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ErrorCount, errorCount_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::WarningCount, warningCount_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(okCount_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(warningCount_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(errorCount_)
            END_DYNAMIC_SIZE_ESTIMATION()
                
        protected:
            ULONG okCount_;
            ULONG warningCount_;
            ULONG errorCount_;
        };
    }
}
