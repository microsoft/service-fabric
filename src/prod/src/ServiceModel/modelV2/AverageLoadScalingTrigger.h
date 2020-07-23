// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        struct AverageLoadScalingTrigger;
        using AverageLoadScalingTriggerSPtr = std::shared_ptr<AverageLoadScalingTrigger>;

        struct AverageLoadScalingTrigger : public AutoScalingTrigger
        {
        public:
            AverageLoadScalingTrigger();

            AverageLoadScalingTrigger(
                AutoScalingMetricSPtr autoScalingMetric,
                double lowerLoadThreshold,
                double upperLoadThreshold,
                uint scaleIntervalInSeconds);

            DEFAULT_MOVE_ASSIGNMENT(AverageLoadScalingTrigger)
            DEFAULT_MOVE_CONSTRUCTOR(AverageLoadScalingTrigger)
            DEFAULT_COPY_ASSIGNMENT(AverageLoadScalingTrigger)
            DEFAULT_COPY_CONSTRUCTOR(AverageLoadScalingTrigger)

            virtual ~AverageLoadScalingTrigger() = default;

            virtual bool Equals(AutoScalingTriggerSPtr const & other, bool ignoreDynamicContent) const;

            __declspec (property(get = get_AutoScalingMetric)) AutoScalingMetricSPtr AutoScalingMetric;
            AutoScalingMetricSPtr get_AutoScalingMetric() const { return autoScalingMetric_; }

            __declspec(property(get = get_LowerLoadThreshold)) double LowerLoadThreshold;
            double get_LowerLoadThreshold() const { return lowerLoadThreshold_; }

            __declspec(property(get = get_UpperLoadThreshold)) double UpperLoadThreshold;
            double get_UpperLoadThreshold() const { return upperLoadThreshold_; }

            __declspec(property(get = get_ScaleIntervalInSeconds)) uint ScaleIntervalInSeconds;
            uint get_ScaleIntervalInSeconds() const { return scaleIntervalInSeconds_; }

            std::wstring GetMetricName() const { return autoScalingMetric_->GetName(); }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingMetric, autoScalingMetric_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingLowerLoadThreshold, lowerLoadThreshold_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingUpperLoadThreshold, upperLoadThreshold_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScaleIntervalInSeconds, scaleIntervalInSeconds_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_CHAIN()
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode Validate() const;

            FABRIC_FIELDS_04(autoScalingMetric_, lowerLoadThreshold_, upperLoadThreshold_, scaleIntervalInSeconds_);
        private:
            AutoScalingMetricSPtr autoScalingMetric_;
            double lowerLoadThreshold_;
            double upperLoadThreshold_;
            uint scaleIntervalInSeconds_;
        };
    }
}