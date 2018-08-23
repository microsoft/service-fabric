// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct AveragePartitionLoadScalingTrigger;
    using AveragePartitionLoadScalingTriggerSPtr = std::shared_ptr<AveragePartitionLoadScalingTrigger>;

    struct AveragePartitionLoadScalingTrigger : public ScalingTrigger
    {
    public:
        AveragePartitionLoadScalingTrigger();

        AveragePartitionLoadScalingTrigger(
            std::wstring const& metricName,
            double lowerLoadThreshold,
            double upperLoadThreshold,
            uint scaleIntervalInSeconds);

        AveragePartitionLoadScalingTrigger(AveragePartitionLoadScalingTrigger const & other) = default;
        AveragePartitionLoadScalingTrigger & operator=(AveragePartitionLoadScalingTrigger const & other) = default;
        AveragePartitionLoadScalingTrigger(AveragePartitionLoadScalingTrigger && other) = default;
        AveragePartitionLoadScalingTrigger & operator=(AveragePartitionLoadScalingTrigger && other) = default;

        virtual ~AveragePartitionLoadScalingTrigger() = default;

        virtual bool Equals(ScalingTriggerSPtr const & other, bool ignoreDynamicContent) const;

        __declspec(property(get = get_MetricName)) std::wstring const& MetricName;
        std::wstring const& get_MetricName() const { return metricName_; }

        __declspec(property(get = get_LowerLoadThreshold)) double LowerLoadThreshold;
        double get_LowerLoadThreshold() const { return lowerLoadThreshold_; }

        __declspec(property(get = get_UpperLoadThreshold)) double UpperLoadThreshold;
        double get_UpperLoadThreshold() const { return upperLoadThreshold_; }

        __declspec(property(get = get_ScaleIntervalInSeconds)) uint ScaleIntervalInSeconds;
        uint get_ScaleIntervalInSeconds() const { return scaleIntervalInSeconds_; }

        virtual std::wstring GetMetricName() const { return metricName_; }

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MetricName, metricName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::LowerLoadThreshold, lowerLoadThreshold_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpperLoadThreshold, upperLoadThreshold_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScaleIntervalInSeconds, scaleIntervalInSeconds_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(metricName_)
        END_DYNAMIC_SIZE_ESTIMATION()

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SCALING_TRIGGER & scalingPolicy) const;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_SCALING_TRIGGER const &);

        virtual Common::ErrorCode Validate(bool isStateful) const;

        virtual void ReadFromXml(Common::XmlReaderUPtr const & xmlReader);
        virtual Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

        FABRIC_FIELDS_04(metricName_, lowerLoadThreshold_, upperLoadThreshold_, scaleIntervalInSeconds_);
    private:
        std::wstring metricName_;
        double lowerLoadThreshold_;
        double upperLoadThreshold_;
        uint scaleIntervalInSeconds_;
    };
}
