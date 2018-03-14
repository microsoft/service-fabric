// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ApplicationCapacitiesDescription
        {
            DENY_COPY_ASSIGNMENT(ApplicationCapacitiesDescription);

        public:
            ApplicationCapacitiesDescription(
                std::wstring && metricName,
                int totalCapacity,
                int maxInstanceCapacity,
                int reservationCapacity);

            ApplicationCapacitiesDescription(ApplicationCapacitiesDescription const & other);

            ApplicationCapacitiesDescription(ApplicationCapacitiesDescription && other);

            ApplicationCapacitiesDescription & operator = (ApplicationCapacitiesDescription && other);

            bool operator == (ApplicationCapacitiesDescription const& other) const;
            bool operator != (ApplicationCapacitiesDescription const& other) const;

            __declspec (property(get = get_MetricName)) std::wstring const& MetricName;
            std::wstring const& get_MetricName() const { return metricName_; }

            __declspec (property(get = get_TotalCapacity)) int TotalCapacity;
            int get_TotalCapacity() const { return totalCapacity_; }

            __declspec (property(get = get_MaxInstanceCapacity)) int MaxInstanceCapacity;
            int get_MaxInstanceCapacity() const { return maxInstanceCapacity_; }

            __declspec (property(get = get_ReservationCapacity)) int ReservationCapacity;
            int get_ReservationCapacity() const { return reservationCapacity_; }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring metricName_;

            int totalCapacity_;
            int maxInstanceCapacity_;
            int reservationCapacity_;

        };
    }
}
