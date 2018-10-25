// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

AddRemoveIncrementalNamedPartitionScalingMechanism::AddRemoveIncrementalNamedPartitionScalingMechanism()
    : ScalingMechanism(ScalingMechanismKind::AddRemoveIncrementalNamedPartition)
{
}

AddRemoveIncrementalNamedPartitionScalingMechanism::AddRemoveIncrementalNamedPartitionScalingMechanism(
    int minPartitionCount,
    int maxPartitionCount,
    int scaleIncrement)
    : ScalingMechanism(ScalingMechanismKind::AddRemoveIncrementalNamedPartition),
      minimumPartitionCount_(minPartitionCount),
      maximumPartitionCount_(maxPartitionCount),
      scaleIncrement_(scaleIncrement)
{
}


bool AddRemoveIncrementalNamedPartitionScalingMechanism::Equals(ScalingMechanismSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<AddRemoveIncrementalNamedPartitionScalingMechanism> (other);

    if (!ignoreDynamicContent)
    {
        if (minimumPartitionCount_ != castedOther->minimumPartitionCount_ ||
            maximumPartitionCount_ != castedOther->maximumPartitionCount_ ||
            scaleIncrement_ != castedOther->scaleIncrement_)
        {
            return false;
        }
    }

    return true;
}

void AddRemoveIncrementalNamedPartitionScalingMechanism::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3}", kind_, minimumPartitionCount_, maximumPartitionCount_, scaleIncrement_);
}

void AddRemoveIncrementalNamedPartitionScalingMechanism::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SCALING_MECHANISM & scalingMechanism) const
{
    auto arMechanism = heap.AddItem<FABRIC_SCALING_MECHANISM_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION>();
    arMechanism->Reserved = NULL;
    arMechanism->MaximumPartitionCount = maximumPartitionCount_;
    arMechanism->MinimumPartitionCount = minimumPartitionCount_;
    arMechanism->ScaleIncrement = scaleIncrement_;

    scalingMechanism.ScalingMechanismKind = FABRIC_SCALING_MECHANISM_KIND_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION;
    scalingMechanism.ScalingMechanismDescription = arMechanism.GetRawPointer();
}

Common::ErrorCode AddRemoveIncrementalNamedPartitionScalingMechanism::FromPublicApi(
    FABRIC_SCALING_MECHANISM const & scalingMechanism)
{
    kind_ = ScalingMechanismKind::AddRemoveIncrementalNamedPartition;
    if (scalingMechanism.ScalingMechanismDescription != nullptr)
    {
        auto arMechanism = reinterpret_cast<FABRIC_SCALING_MECHANISM_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION*> (scalingMechanism.ScalingMechanismDescription);
        maximumPartitionCount_ = arMechanism->MaximumPartitionCount;
        minimumPartitionCount_ = arMechanism->MinimumPartitionCount;
        scaleIncrement_ = arMechanism->ScaleIncrement;
    }
    return ErrorCodeValue::Success;
}

Common::ErrorCode AddRemoveIncrementalNamedPartitionScalingMechanism::Validate(bool isStateful) const
{
    UNREFERENCED_PARAMETER(isStateful);
    if (minimumPartitionCount_ < 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_MinPartitionCount), minimumPartitionCount_));
    }
    if (maximumPartitionCount_ < -1 || maximumPartitionCount_ == 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_MaxPartitionCount), maximumPartitionCount_));
    }

    if (scaleIncrement_ <= 0)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Increment));
    }
    if (maximumPartitionCount_ > 0 && minimumPartitionCount_ > maximumPartitionCount_)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_MinMaxPartitions), minimumPartitionCount_, maximumPartitionCount_));
    }
    return ErrorCodeValue::Success;
}

void AddRemoveIncrementalNamedPartitionScalingMechanism::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingMechanismAddRemoveIncrementalNamedPartition,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_ScalingMechanismAddRemoveIncrementalNamedPartition,
            *SchemaNames::Namespace,
            true);

        int64 minPartitionCount;
        wstring attrValueMinCount = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyMinPartitionCount);
        if (StringUtility::TryFromWString<int64>(attrValueMinCount, minPartitionCount))
        {
            this->minimumPartitionCount_ = (int)minPartitionCount;
        }

        int64 maxPartitionCount;
        wstring attrValueMaxCount = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyMaxPartitionCount);
        if (StringUtility::TryFromWString<int64>(attrValueMaxCount, maxPartitionCount))
        {
            this->maximumPartitionCount_ = (int)maxPartitionCount;
        }

        int64 scaleIncrement;
        wstring attrValueScaleIncrement = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyScaleIncrement);
        if (StringUtility::TryFromWString<int64>(attrValueScaleIncrement, scaleIncrement))
        {
            this->scaleIncrement_ = (int)scaleIncrement;
        }

        xmlReader->ReadElement();
    }
}

ErrorCode AddRemoveIncrementalNamedPartitionScalingMechanism::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    auto er = xmlWriter->WriteStartElement(*SchemaNames::Element_ScalingMechanismAddRemoveIncrementalNamedPartition, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyMinPartitionCount, this->maximumPartitionCount_);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyMaxPartitionCount, this->minimumPartitionCount_);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyScaleIncrement, this->scaleIncrement_);
    if (!er.IsSuccess())
    {
        return er;
    }
    return xmlWriter->WriteEndElement();
}


