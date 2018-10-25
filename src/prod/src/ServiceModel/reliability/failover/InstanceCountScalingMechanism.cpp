// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

InstanceCountScalingMechanism::InstanceCountScalingMechanism()
    : ScalingMechanism(ScalingMechanismKind::PartitionInstanceCount)
{
}

InstanceCountScalingMechanism::InstanceCountScalingMechanism(
    int minimumInstanceCount,
    int maximumInstanceCount,
    int scaleIncrement)
    : ScalingMechanism(ScalingMechanismKind::PartitionInstanceCount),
      minimumInstanceCount_(minimumInstanceCount),
      maximumInstanceCount_(maximumInstanceCount),
      scaleIncrement_(scaleIncrement)
{
}

bool InstanceCountScalingMechanism::Equals(ScalingMechanismSPtr const & other, bool ignoreDynamicContent) const
{
    if (other == nullptr)
    {
        return false;
    }

    if (kind_ != other->Kind)
    {
        return false;
    }

    auto castedOther = static_pointer_cast<InstanceCountScalingMechanism> (other);

    if (!ignoreDynamicContent)
    {
        if (minimumInstanceCount_ != castedOther->minimumInstanceCount_ ||
            maximumInstanceCount_ != castedOther->maximumInstanceCount_ ||
            scaleIncrement_ != castedOther->scaleIncrement_)
        {
            return false;
        }
    }

    return true;
}

void InstanceCountScalingMechanism::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3}", kind_, minimumInstanceCount_, maximumInstanceCount_, scaleIncrement_);
}

Common::ErrorCode InstanceCountScalingMechanism::Validate(bool isStateful) const
{
    if (minimumInstanceCount_ < 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_MinInstanceCount), minimumInstanceCount_));
    }
    if (maximumInstanceCount_ < -1 || maximumInstanceCount_ == 0)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidServiceScalingPolicy,
            wformatString(GET_NS_RC(ScalingPolicy_MaxInstanceCount), maximumInstanceCount_));
    }
    if (scaleIncrement_ < 0)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Increment));
    }
    if (maximumInstanceCount_ > 0 && minimumInstanceCount_ > maximumInstanceCount_)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, wformatString(GET_NS_RC(ScalingPolicy_MinMaxInstances), minimumInstanceCount_, maximumInstanceCount_));
    }
    // This mechanism scales the service by adding or removing instances.
    // It does not make any sense for stateful service
    if (isStateful)
    {
        return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Instance_Count));
    }
    return ErrorCodeValue::Success;
}

void InstanceCountScalingMechanism::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SCALING_MECHANISM & scalingMechanism) const
{
    auto arMechanism = heap.AddItem<FABRIC_SCALING_MECHANISM_PARTITION_INSTANCE_COUNT>();
    arMechanism->Reserved = NULL;
    arMechanism->MaximumInstanceCount = maximumInstanceCount_;
    arMechanism->MinimumInstanceCount = minimumInstanceCount_;
    arMechanism->ScaleIncrement = scaleIncrement_;

    scalingMechanism.ScalingMechanismKind = FABRIC_SCALING_MECHANISM_KIND_SCALE_PARTITION_INSTANCE_COUNT;
    scalingMechanism.ScalingMechanismDescription = arMechanism.GetRawPointer();
}

Common::ErrorCode InstanceCountScalingMechanism::FromPublicApi(
    FABRIC_SCALING_MECHANISM const & scalingMechanism)
{
    kind_ = ScalingMechanismKind::PartitionInstanceCount;
    if (scalingMechanism.ScalingMechanismDescription != nullptr)
    {
        auto icMechanism = reinterpret_cast<FABRIC_SCALING_MECHANISM_PARTITION_INSTANCE_COUNT*> (scalingMechanism.ScalingMechanismDescription);
        maximumInstanceCount_ = icMechanism->MaximumInstanceCount;
        minimumInstanceCount_ = icMechanism->MinimumInstanceCount;
        scaleIncrement_ = icMechanism->ScaleIncrement;
    }
    return ErrorCodeValue::Success;
}

void InstanceCountScalingMechanism::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingMechanismPartitionInstanceCount,
        *SchemaNames::Namespace,
        false))
    {
        xmlReader->StartElement(
            *SchemaNames::Element_ScalingMechanismPartitionInstanceCount,
            *SchemaNames::Namespace,
            true);

        int64 minInstanceCount;
        wstring attrValueMinCount = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyMinInstanceCount);
        if (StringUtility::TryFromWString<int64>(attrValueMinCount, minInstanceCount))
        {
            this->minimumInstanceCount_ = (int)minInstanceCount;
        }

        int64 maxInstanceCount;
        wstring attrValueMaxCount = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ScalingPolicyMaxInstanceCount);
        if (StringUtility::TryFromWString<int64>(attrValueMaxCount, maxInstanceCount))
        {
            this->maximumInstanceCount_ = (int)maxInstanceCount;
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

ErrorCode InstanceCountScalingMechanism::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    auto er = xmlWriter->WriteStartElement(*SchemaNames::Element_ScalingMechanismPartitionInstanceCount, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyMinInstanceCount, this->minimumInstanceCount_);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_ScalingPolicyMaxInstanceCount, this->maximumInstanceCount_);
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
