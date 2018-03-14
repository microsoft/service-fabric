// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        class ApiCallDescription 
        {
        public:
            ApiCallDescription(
                MonitoringData monitoringData,
                MonitoringParameters monitoringParameters)
            : monitoringData_(std::move(monitoringData)),
              monitoringParameters_(monitoringParameters),
			  hasExpired_(false),
              lastTraced_(Common::StopwatchTime::MaxValue),
			  hasReportedHealth_(false)
            {
            }

            ApiCallDescription(ApiCallDescription && other)
            : monitoringData_(std::move(other.monitoringData_)),
              monitoringParameters_(other.monitoringParameters_),
			  hasExpired_(false),
              lastTraced_(std::move(other.lastTraced_)),
			  hasReportedHealth_(false)
            {
            }

            ApiCallDescription & operator=(ApiCallDescription && other)
            {
                if (this != &other)
                {
                    monitoringData_ = std::move(other.monitoringData_);
                    monitoringParameters_ = other.monitoringParameters_;
					hasExpired_ = std::move(other.hasExpired_);
                    lastTraced_ = std::move(other.lastTraced_);
					hasReportedHealth_ = std::move(other.hasReportedHealth_);
                }
                
                return *this;
            }

            __declspec(property(get = get_Data)) MonitoringData const & Data;
            MonitoringData const & get_Data() const { return monitoringData_; }

            __declspec(property(get = get_Parameters)) MonitoringParameters const & Parameters;
            MonitoringParameters const & get_Parameters() const { return monitoringParameters_; }

			__declspec(property(get = get_HasExpired)) bool HasExpired;
			bool get_HasExpired() const { return hasExpired_; }

            MonitoringActionsNeeded GetActions(Common::StopwatchTime now)
            {
				bool needTrace = false;
				bool needReportHealth = false;

                if (now - monitoringData_.StartTime >= monitoringParameters_.ApiSlowDuration)
                {
					if (monitoringParameters_.IsApiSlowTraceEnabled)
					{
						if (!hasExpired_ || now - lastTraced_ >= monitoringParameters_.PeriodicSlowTraceDuration)
						{
							lastTraced_ = now;
							needTrace = true;
						}
					}

					if (monitoringParameters_.IsHealthReportEnabled)
					{
						needReportHealth = !hasReportedHealth_;
					}

					hasExpired_ = true;
                }

				if (needReportHealth)
				{
					hasReportedHealth_ = true;
				}

                return std::make_pair(needTrace, needReportHealth);
            }

            MonitoringHealthEvent CreateHealthEvent(FABRIC_SEQUENCE_NUMBER sequenceNumber) const
            {
                return std::make_pair(&monitoringData_, sequenceNumber);
            }

        private:
            MonitoringData monitoringData_;
            MonitoringParameters monitoringParameters_;
			bool hasExpired_;
			Common::StopwatchTime lastTraced_;
			bool hasReportedHealth_;
        };
    }
}
