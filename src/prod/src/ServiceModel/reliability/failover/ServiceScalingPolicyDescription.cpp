//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ServiceScalingPolicyDescription)

ServiceScalingPolicyDescription::ServiceScalingPolicyDescription()
    : trigger_(),
      mechanism_()
{
}

ServiceScalingPolicyDescription::ServiceScalingPolicyDescription(ScalingTriggerSPtr trigger, ScalingMechanismSPtr mechanism)
    : trigger_(trigger),
    mechanism_(mechanism)
{
}

ErrorCode ServiceScalingPolicyDescription::Equals(
    ServiceScalingPolicyDescription const & other,
    bool ignoreDynamicContent) const
{
    if (trigger_ && other.trigger_)
    {
        if (!trigger_->Equals(other.trigger_, ignoreDynamicContent))
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_FM_RC(ServiceScalingPolicyDescription_ScalingTrigger_Changed), trigger_, other.trigger_));
        }
    }
    else if(trigger_ && !other.trigger_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceScalingPolicyDescription_ScalingTrigger_Removed), trigger_));
    }
    else if (!trigger_ && other.trigger_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceScalingPolicyDescription_ScalingTrigger_Added), other.trigger_));
    }

    if (mechanism_ && other.mechanism_)
    {
        if (!mechanism_->Equals(other.mechanism_, ignoreDynamicContent))
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_FM_RC(ServiceScalingPolicyDescription_ScalingMechanism_Changed), mechanism_, other.mechanism_));
        }
    }
    else if (mechanism_ && !other.mechanism_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceScalingPolicyDescription_ScalingMechanism_Removed), mechanism_));
    }
    else if(!mechanism_ && other.mechanism_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceScalingPolicyDescription_ScalingMechanism_Added), other.mechanism_));
    }

    return ErrorCode::Success();
}

bool ServiceScalingPolicyDescription::operator == (ServiceScalingPolicyDescription const & other) const
{
    auto error = Equals(other, false);

    return error.IsSuccess();
}

bool ServiceScalingPolicyDescription::operator != (ServiceScalingPolicyDescription const & other) const
{
    return !(*this == other);
}

Common::TimeSpan ServiceScalingPolicyDescription::GetScalingInterval() const
{
    if (trigger_ == nullptr)
    {
        Common::Assert::TestAssert("Attempting to get scaling interval when trigger_ is nullptr");
        return TimeSpan::MaxValue;
    }
    switch (trigger_->Kind)
    {
    case ScalingTriggerKind::AveragePartitionLoad:
        return Common::TimeSpan::FromSeconds((static_pointer_cast<AveragePartitionLoadScalingTrigger>(trigger_))->ScaleIntervalInSeconds);
    case ScalingTriggerKind::AverageServiceLoad:
        return Common::TimeSpan::FromSeconds((static_pointer_cast<AverageServiceLoadScalingTrigger>(trigger_))->ScaleIntervalInSeconds);
    default:
        Common::Assert::TestAssert(
            "Attempting to get scaling interval when trigger_ kind is unknown. Kind={0}.",
            trigger_->Kind);
        return TimeSpan::MaxValue;
    }
}

void ServiceScalingPolicyDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("{0} {1}", trigger_, mechanism_);
}

void ServiceScalingPolicyDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SERVICE_SCALING_POLICY & scalingPolicy) const
{
    if (trigger_)
    {
        trigger_->ToPublicApi(heap, scalingPolicy.ServiceScalingPolicyTrigger);
    }
    if (mechanism_)
    {
        mechanism_->ToPublicApi(heap, scalingPolicy.ServiceScalingPolicyMechanism);
    }
}

Common::ErrorCode Reliability::ServiceScalingPolicyDescription::FromPublicApi(FABRIC_SERVICE_SCALING_POLICY const & scalingPolicy)
{
    ErrorCode error = ErrorCodeValue::Success;

    switch (scalingPolicy.ServiceScalingPolicyTrigger.ScalingTriggerKind)
    {
    case FABRIC_SCALING_TRIGGER_KIND_AVERAGE_PARTITION_LOAD:
        trigger_ = make_shared<AveragePartitionLoadScalingTrigger>();
        error = trigger_->FromPublicApi(scalingPolicy.ServiceScalingPolicyTrigger);
        break;
    case FABRIC_SCALING_TRIGGER_KIND_AVERAGE_SERVICE_LOAD:
        trigger_ = make_shared<AverageServiceLoadScalingTrigger>();
        error = trigger_->FromPublicApi(scalingPolicy.ServiceScalingPolicyTrigger);
        break;
    default:
        error = ErrorCodeValue::InvalidServiceScalingPolicy;
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    switch (scalingPolicy.ServiceScalingPolicyMechanism.ScalingMechanismKind)
    {
    case FABRIC_SCALING_MECHANISM_KIND_SCALE_PARTITION_INSTANCE_COUNT:
        mechanism_ = make_shared<InstanceCountScalingMechanism>();
        error = mechanism_->FromPublicApi(scalingPolicy.ServiceScalingPolicyMechanism);
        break;
    case FABRIC_SCALING_MECHANISM_KIND_ADD_REMOVE_INCREMENTAL_NAMED_PARTITION:
        mechanism_ = make_shared<AddRemoveIncrementalNamedPartitionScalingMechanism>();
        error = mechanism_->FromPublicApi(scalingPolicy.ServiceScalingPolicyMechanism);;
        break;
    default:
        error = ErrorCodeValue::InvalidServiceScalingPolicy;
    }

    return error;
}

Common::ErrorCode ServiceScalingPolicyDescription::Validate(bool isStateful) const
{
    if (   (mechanism_ != nullptr && trigger_ == nullptr)
        || (mechanism_ == nullptr && trigger_ != nullptr))
    {
        // Both the trigger and the mechanism must be there.
        return ErrorCodeValue::InvalidServiceScalingPolicy;
    }
    if (mechanism_ != nullptr && trigger_ != nullptr)
    {
        if (   mechanism_->Kind == ScalingMechanismKind::AddRemoveIncrementalNamedPartition
            && trigger_->Kind == ScalingTriggerKind::AveragePartitionLoad)
        {
            // When looking at partition load, scaling is only by instance count.
            return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Partitions_Scaling));
        }
        if (   mechanism_->Kind == ScalingMechanismKind::PartitionInstanceCount
            && trigger_->Kind == ScalingTriggerKind::AverageServiceLoad)
        {
            // When looking at service load, scaling is only by adding or removing partitions.
            return ErrorCode(ErrorCodeValue::InvalidServiceScalingPolicy, GET_NS_RC(ScalingPolicy_Instances_Scaling));
        }
        auto error = mechanism_->Validate(isStateful);
        if (!error.IsSuccess())
        {
            return error;
        }
        error = trigger_->Validate(isStateful);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
    return ErrorCodeValue::Success;
}

void ServiceScalingPolicyDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    xmlReader->ReadStartElement();

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingTriggerAveragePartitionLoad,
        *SchemaNames::Namespace,
        false))
    {
        trigger_ = make_shared<AveragePartitionLoadScalingTrigger>();
        trigger_->ReadFromXml(xmlReader);
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingTriggerAverageServiceLoad,
        *SchemaNames::Namespace,
        false))
    {
        trigger_ = make_shared<AverageServiceLoadScalingTrigger>();
        trigger_->ReadFromXml(xmlReader);
    }
    else
    {
        Parser::ThrowInvalidContent(xmlReader, L"Invalid scaling trigger", L"");
        return; // ThrowInvalidContent is not declared as noreturn, and there will be an error if we don't return.
    }

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingMechanismPartitionInstanceCount,
        *SchemaNames::Namespace,
        false))
    {
        mechanism_ = make_shared<InstanceCountScalingMechanism>();
        mechanism_->ReadFromXml(xmlReader);
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_ScalingMechanismAddRemoveIncrementalNamedPartition,
        *SchemaNames::Namespace,
        false))
    {
        mechanism_ = make_shared<AddRemoveIncrementalNamedPartitionScalingMechanism>();
        mechanism_->ReadFromXml(xmlReader);
    }
    else
    {
        Parser::ThrowInvalidContent(xmlReader, L"Invalid scaling mechanism", L"");
        return; // ThrowInvalidContent is not declared as noreturn, and there will be an error if we don't return.
    }

    xmlReader->ReadEndElement();
}

ErrorCode ServiceScalingPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    auto er = xmlWriter->WriteStartElement(*SchemaNames::Element_ScalingPolicy, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    if (trigger_ != nullptr)
    {
        er = trigger_->WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    if (mechanism_ != nullptr)
    {
        er = mechanism_->WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    return xmlWriter->WriteEndElement();
}
