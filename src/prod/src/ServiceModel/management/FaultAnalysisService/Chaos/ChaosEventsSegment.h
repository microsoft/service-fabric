//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class ChaosEventsSegment
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DENY_COPY(ChaosEventsSegment)

        public:

            ChaosEventsSegment();
            ChaosEventsSegment(ChaosEventsSegment &&) = default;
            ChaosEventsSegment(ChaosReport &&);

            __declspec(property(get = get_ContinuationToken)) std::wstring const& ContinuationToken;
            std::wstring const& get_ContinuationToken() const { return continuationToken_; }

            __declspec(property(get = get_Events)) std::vector<ChaosEvent> const & History;
            std::vector<ChaosEvent> const & get_Events() const { return history_; }

            Common::ErrorCode FromPublicApi(FABRIC_CHAOS_EVENTS_SEGMENT const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CHAOS_EVENTS_SEGMENT &) const;

            FABRIC_FIELDS_02(
                continuationToken_,
                history_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationToken_);
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::History, history_);
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::wstring continuationToken_;
            std::vector<ChaosEvent> history_;
        };
    }
}
