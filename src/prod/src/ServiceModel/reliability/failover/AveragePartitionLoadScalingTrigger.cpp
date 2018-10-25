// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(AveragePartitionLoadScalingTrigger)

AveragePartitionLoadScalingTrigger::AveragePartitionLoadScalingTrigger()
    : ScalingTrigger(ScalingTriggerKind::AveragePartitionLoad)
{
}

AveragePartitionLoadScalingTrigger::AveragePartitionLoadScalingTrigger(
    std::wstring const& metricName,
    double lowerLoadThreshold,
    double upperLoadThreshold,
    uint scaleIntervalInSeconds)
    : ScalingTrigger(ScalingTriggerKind::AveragePartitionLoad),
      metricName_(metricName),
      lowerLoadThreshold_(lowerLoadThreshold),
      upperLoadThreshold_(upperLoadThreshold),
      scaleIntervalInSeconds_(scaleIntervalInSeconds)
{
}

bool AveragePartitionLoadScalingTrigger::Equals(ScalingTriggerSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<AveragePartitionLoadScalingTrigger> (other);

    if (!ignoreDynamicContent)
    {
        if (metricName_ != castedOther->metricName_ ||
            upperLoadThreshold_ != castedOther->upperLoadThreshold_ ||
            lowerLoadThreshold_ != castedOther->lowerLoadThreshold_ ||
            scaleIntervalInSeconds_ != castedOther->scaleIntervalInSeconds_)
        {
            return false;
        }
    }

    return true;
}

void AveragePartitionLoadScalingTrigger::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3} {4}", kind_, metricName_, lowerLoadThreshold_, upperLoadThreshold_, scaleIntervalInSeconds_);
}

void AveragePartitionLoadScalingTrigger::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SCALING_TRIGGER & scalingPolicy) const
{
    auto averagePartLoad = heap.AddItem<FABRIC_SCALING_TRIGGER_AVERAGE_PARTITION_LOAD>();
    averagePartLoad->Reserved = NULL;
    averagePartLoad->MetricName = heap.AddString(metricName_);
    averagePartLoad->LowerLoadThreshold = lowerLoadThreshold_;
    averagePartLoad->UpperLoadThreshold = upperLoadThreshold_;
    averagePartLoad->ScaleIntervalInSeconds = scaleIntervalInSeconds_;

    scalingPolicy.ScalingTriggerKind = FABRIC_SCALING_TRIGGER_KIND_AVERAGE_PARTITION_LOAD;
    scalingPolicy.ScalingTriggerDescription = averagePartLoad.GetRawPointer();
}

Common::ErrorCode AveragePartitionLoadScalingTrigger::FromPublicApi(
    FABRIC_SCALING_TRIGGER const & scalingTrigger)
{
    kind_ = ScalingTriggerKind::AveragePartitionLoad;
    if (scalingTrigger.ScalingTriggerDescription != nullptr)
    {
        auto averagePartLoad = reinterpret_cast<FABRIC_SCALING_TRIGGER_AVERAGE_PARTITION_LOAD*> (scalingTrigger.ScalingTriggerDescription);
        upperLoadThreshold_ = averagePartLoad->UpperLoadThreshold;
        lowerLoadThreshold_ = averagePartLoad->LowerLoadThreshold;
        scaleIntervalInSeconds_ = averagePartLoad->ScaleIntervalInSeconds;
        auto hr = StringUtility::LpcwstrToWstring(averagePartLoad->MetricName, false /*acceptNull*/, metricName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }
    }
    return ErrorCodeValue::Success;
}

Common::ErrorCode AveragePartitionLoadScalingTrigger::Validate(bool isStateful) const
{
    UNREFERENCED_PARAMETER(isStateful);
    if (lowerLoadThreshold_ < 0.0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_LowerLoadThreshold), lowerLoadThreshold_));
    }
    if (upperLoadThreshold_ <= 0.0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_UpperLoadThreshold), upperLoadThreshold_));
    }
    if (lowerLoadThreshold_ > upperLoadThreshold_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_MinMaxInstances), lowerLoadThreshold_, upperLoadThreshold_));
    }
    return ErrorCodeValue::Success;
}

void AveragePartitionLoadScalingTrigger::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingTriggerAveragePartitionLoad,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_ScalingTriggerAveragePartitionLoad,
            *SchemaNames::Namespace,
            true);

        wstring attrValueName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyMetricName);
        this->metricName_ = move(attrValueName);

        double metricLimitLow;
        wstring attrValueLimitLow = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyLowerLoadThreshold);
        if (StringUtility::TryFromWString<double>(attrValueLimitLow, metricLimitLow))
        {
            this->lowerLoadThreshold_ = metricLimitLow;
        }

        double metricLimitHigh;
        wstring attrValueLimitHigh = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyUpperLoadThreshold);
        if (StringUtility::TryFromWString<double>(attrValueLimitHigh, metricLimitHigh))
        {
            this->upperLoadThreshold_ = metricLimitHigh;
        }

        uint64 scaleIntervalSeconds;
        wstring attrValueScaleInterval = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyScaleInterval);
        if (StringUtility::TryFromWString<uint64>(attrValueScaleInterval, scaleIntervalSeconds))
        {
            this->scaleIntervalInSeconds_ = (DWORD)scaleIntervalSeconds;
        }

        xmlReader->ReadElement();
    }
}

ErrorCode AveragePartitionLoadScalingTrigger::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    auto er = xmlWriter->WriteStartElement(*SchemaNames::Element_ScalingTriggerAveragePartitionLoad, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ScalingPolicyMetricName, this->metricName_);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyLowerLoadThreshold, this->lowerLoadThreshold_);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyUpperLoadThreshold, this->upperLoadThreshold_);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyScaleInterval, this->scaleIntervalInSeconds_);
    if (!er.IsSuccess())
    {
        return er;
    }

    return xmlWriter->WriteEndElement();
}
