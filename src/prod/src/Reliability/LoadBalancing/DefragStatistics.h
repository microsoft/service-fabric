// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Snapshot;

        class DefragStatistics
        {
        public:
            DefragStatistics();

            // Updates loads and configuration parameters
            void Update(Snapshot const&);

            void Clear();

            __declspec(property(get = get_BalancingMetricsCount)) uint64_t BalancingMetricsCount;
            uint64_t get_BalancingMetricsCount() const { return balancingMetricsCount_; }

            __declspec(property(get = get_BalancingReservationMetricsCount)) uint64_t BalancingReservationMetricsCount;
            uint64_t get_BalancingReservationMetricsCount() const { return balancingReservationMetricsCount_; }

            __declspec(property(get = get_ReservationMetricsCount)) uint64_t ReservationMetricsCount;
            uint64_t get_ReservationMetricsCount() const { return reservationMetricsCount_; }

            __declspec(property(get = get_PackReservationMetricsCount)) uint64_t PackReservationMetricsCount;
            uint64_t get_PackReservationMetricsCount() const { return packReservationMetricsCount_; }

            __declspec(property(get = get_DefragMetricsCount)) uint64_t DefragMetricsCount;
            uint64_t get_DefragMetricsCount() const { return defragMetricsCount_; }

            __declspec(property(get = get_OtherMetricsCount)) uint64_t OtherMetricsCount;
            uint64_t get_OtherMetricsCount() const { return otherMetricsCount_; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            uint64_t balancingMetricsCount_;
            uint64_t balancingReservationMetricsCount_;
            uint64_t reservationMetricsCount_;
            uint64_t packReservationMetricsCount_;
            uint64_t defragMetricsCount_;
            uint64_t otherMetricsCount_;
        };
    }
}
