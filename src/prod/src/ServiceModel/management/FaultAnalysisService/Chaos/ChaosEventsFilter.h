//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosEventsFilter
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosEventsFilter)

        public:
            ChaosEventsFilter();

            ChaosEventsFilter(
                Common::DateTime const & startTimeUtc,
                Common::DateTime const & endTimeUtc);

            __declspec(property(get = get_StartTimeUtc)) Common::DateTime const & StartTimeUtc;
            Common::DateTime const & get_StartTimeUtc() const { return startTimeUtc_; }

            __declspec(property(get = get_EndTimeUtc)) Common::DateTime const & EndTimeUtc;
            Common::DateTime const & get_EndTimeUtc() const { return endTimeUtc_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_EVENTS_SEGMENT_FILTER const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_EVENTS_SEGMENT_FILTER &) const;

            FABRIC_FIELDS_02(
                startTimeUtc_,
                endTimeUtc_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StartTimeUtc, startTimeUtc_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::EndTimeUtc, endTimeUtc_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            Common::DateTime startTimeUtc_;
            Common::DateTime endTimeUtc_;
        };
    }
}
