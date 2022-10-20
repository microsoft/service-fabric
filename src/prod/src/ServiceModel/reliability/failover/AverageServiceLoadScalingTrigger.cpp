// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(AverageServiceLoadScalingTrigger)

AverageServiceLoadScalingTrigger::AverageServiceLoadScalingTrigger()
    : ScalingTrigger(ScalingTriggerKind::AverageServiceLoad)
{
}

AverageServiceLoadScalingTrigger::AverageServiceLoadScalingTrigger(
    std::wstring const& metricName,
    double lowerLoadThreshold,
    double upperLoadThreshold,
    uint scaleIntervalInSeconds, 
    bool useOnlyPrimaryLoad)
    : ScalingTrigger(ScalingTriggerKind::AverageServiceLoad),
      metricName_(metricName),
      lowerLoadThreshold_(lowerLoadThreshold),
      upperLoadThreshold_(upperLoadThreshold),
      scaleIntervalInSeconds_(scaleIntervalInSeconds),
      useOnlyPrimaryLoad_(useOnlyPrimaryLoad)
{
}

bool AverageServiceLoadScalingTrigger::Equals(ScalingTriggerSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<AverageServiceLoadScalingTrigger> (other);

    if (!ignoreDynamicContent)
    {
        if (metricName_ != castedOther->metricName_ ||
            upperLoadThreshold_ != castedOther->upperLoadThreshold_ ||
            lowerLoadThreshold_ != castedOther->lowerLoadThreshold_ ||
            scaleIntervalInSeconds_ != castedOther->scaleIntervalInSeconds_||
            useOnlyPrimaryLoad_ != castedOther->useOnlyPrimaryLoad_)
        {
            return false;
        }
    }

    return true;
}

void AverageServiceLoadScalingTrigger::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3} {4} {5}", kind_, metricName_, lowerLoadThreshold_, upperLoadThreshold_, scaleIntervalInSeconds_, useOnlyPrimaryLoad_);
}

void AverageServiceLoadScalingTrigger::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SCALING_TRIGGER & scalingPolicy) const
{
    auto averageSrvLoadLoad = heap.AddItem<FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD>();
    averageSrvLoadLoad->MetricName = heap.AddString(metricName_);
    averageSrvLoadLoad->LowerLoadThreshold = lowerLoadThreshold_;
    averageSrvLoadLoad->UpperLoadThreshold = upperLoadThreshold_;
    averageSrvLoadLoad->ScaleIntervalInSeconds = scaleIntervalInSeconds_;
    
    auto averageSrvLoadLoad2 = heap.AddItem<FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD_EX1>();
    averageSrvLoadLoad2->UseOnlyPrimaryLoad = useOnlyPrimaryLoad_;

    averageSrvLoadLoad->Reserved = averageSrvLoadLoad2.GetRawPointer();

    scalingPolicy.ScalingTriggerKind = FABRIC_SCALING_TRIGGER_KIND_AVERAGE_SERVICE_LOAD;
    scalingPolicy.ScalingTriggerDescription = averageSrvLoadLoad.GetRawPointer();
}

Common::ErrorCode AverageServiceLoadScalingTrigger::FromPublicApi(
    FABRIC_SCALING_TRIGGER const & scalingTrigger)
{
    kind_ = ScalingTriggerKind::AverageServiceLoad;
    if (scalingTrigger.ScalingTriggerDescription != nullptr)
    {
        auto averagePartLoad = reinterpret_cast<FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD*> (scalingTrigger.ScalingTriggerDescription);
        upperLoadThreshold_ = averagePartLoad->UpperLoadThreshold;
        lowerLoadThreshold_ = averagePartLoad->LowerLoadThreshold;
        scaleIntervalInSeconds_ = averagePartLoad->ScaleIntervalInSeconds;

        auto hr = StringUtility::LpcwstrToWstring(averagePartLoad->MetricName, false /*acceptNull*/, metricName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD_EX1* averagePartLoad2 = (FABRIC_SCALING_TRIGGER_AVERAGE_SERVICE_LOAD_EX1*) averagePartLoad->Reserved;
        if (averagePartLoad2) {
            useOnlyPrimaryLoad_ = averagePartLoad2->UseOnlyPrimaryLoad;
        }
    }
    return ErrorCodeValue::Success;
}

Common::ErrorCode AverageServiceLoadScalingTrigger::Validate(bool isStateful) const
{
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
    // Setting UseOnlyPrimaryLoad to true doesn't make sense for stateless services, since all replicas are modeled as secondaries
    if (!isStateful && useOnlyPrimaryLoad_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_UseOnlyPrimaryLoad)));
    }
    return ErrorCodeValue::Success;
}

void AverageServiceLoadScalingTrigger::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingTriggerAverageServiceLoad,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_ScalingTriggerAverageServiceLoad,
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

        bool useOnlyPrimaryLoad;
        if (xmlReader->HasAttribute(*SchemaNames::Attribute_ScalingPolicyUseOnlyPrimaryLoad))
        {
            wstring attrValuePrimaryOnly = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyUseOnlyPrimaryLoad);
            if (StringUtility::TryFromWString<bool>(attrValuePrimaryOnly, useOnlyPrimaryLoad))
            {
                this->useOnlyPrimaryLoad_ = useOnlyPrimaryLoad;
            }
            else
            {
                this->useOnlyPrimaryLoad_ = false;
            }
        }

        xmlReader->ReadElement();
    }
}

ErrorCode AverageServiceLoadScalingTrigger::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    auto er = xmlWriter->WriteStartElement(*SchemaNames::Element_ScalingTriggerAverageServiceLoad, L"", *SchemaNames::Namespace);
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
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_ScalingPolicyUseOnlyPrimaryLoad, this->useOnlyPrimaryLoad_);
    if (!er.IsSuccess())
    {
        return er;
    }

    return xmlWriter->WriteEndElement();
}
