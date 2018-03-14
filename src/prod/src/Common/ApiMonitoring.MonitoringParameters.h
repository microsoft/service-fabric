// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        class MonitoringParameters
        {
        public:
            MonitoringParameters() :
                isApiLifeCycleTraceEnabled_(false),
                isHealthReportEnabled_(false),
                isApiSlowTraceEnabled_(false),
                apiSlowDuration_(Common::TimeSpan::MaxValue),
				periodicSlowTraceDuration_(Common::TimeSpan::MaxValue)
            {
            }

            MonitoringParameters(
                bool isHealthReportEnabled, 
                bool isApiSlowTraceEnabled, 
                bool isApiLifeCycleTraceEnabled,
                Common::TimeSpan apiSlowDuration,
				Common::TimeSpan periodicSlowTraceDuration = Common::TimeSpan::MaxValue) :
                isApiLifeCycleTraceEnabled_(isApiLifeCycleTraceEnabled),
                isHealthReportEnabled_(isHealthReportEnabled),
                isApiSlowTraceEnabled_(isApiSlowTraceEnabled),
                apiSlowDuration_(apiSlowDuration),
				periodicSlowTraceDuration_(periodicSlowTraceDuration)
            {
            }

            __declspec(property(get = get_IsHealthReportEnabled, put = set_IsHealthReportEnabled)) bool IsHealthReportEnabled;
            bool get_IsHealthReportEnabled() const { return isHealthReportEnabled_; }
            void set_IsHealthReportEnabled(bool value) { isHealthReportEnabled_ = value; }

            __declspec(property(get = get_IsApiSlowTraceEnabled, put = set_IsApiSlowTraceEnabled)) bool IsApiSlowTraceEnabled;
            bool get_IsApiSlowTraceEnabled() const { return isApiSlowTraceEnabled_; }
            void set_IsApiSlowTraceEnabled(bool value) { isApiSlowTraceEnabled_ = value; }

            __declspec(property(get = get_IsApiLifeCycleTraceEnabled, put = set_IsApiLifeCycleTraceEnabled)) bool IsApiLifeCycleTraceEnabled;
            bool get_IsApiLifeCycleTraceEnabled() const { return isApiLifeCycleTraceEnabled_; }
            void set_IsApiLifeCycleTraceEnabled(bool value) { isApiLifeCycleTraceEnabled_ = value; }

            __declspec(property(get = get_ApiSlowDuration, put = set_ApiSlowDuration)) Common::TimeSpan ApiSlowDuration;
            Common::TimeSpan get_ApiSlowDuration() const { return apiSlowDuration_;}
            void set_ApiSlowDuration(Common::TimeSpan const& value) { apiSlowDuration_ = value; }

			__declspec(property(get = get_PeriodicSlowTraceDuration, put = set_PeriodicSlowTraceDuration)) Common::TimeSpan PeriodicSlowTraceDuration;
			Common::TimeSpan get_PeriodicSlowTraceDuration() const { return periodicSlowTraceDuration_; }
			void set_PeriodicSlowTraceDuration(Common::TimeSpan const& value) { periodicSlowTraceDuration_ = value; }

        private:
            bool isApiLifeCycleTraceEnabled_;
            bool isHealthReportEnabled_;
            bool isApiSlowTraceEnabled_;
            Common::TimeSpan apiSlowDuration_;
			Common::TimeSpan periodicSlowTraceDuration_;
        };
    }
}


