// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

INITIALIZE_SIZE_ESTIMATION(ScalingTrigger)

ScalingTrigger::ScalingTrigger()
    : kind_(ScalingTriggerKind::Invalid)
{

}

ScalingTrigger::ScalingTrigger(ScalingTriggerKind::Enum kind)
    : kind_(kind)
{
}

bool ScalingTrigger::Equals(ScalingTriggerSPtr const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

void ScalingTrigger::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("{0}", kind_);
}

NTSTATUS ScalingTrigger::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

HRESULT Reliability::ScalingTrigger::CheckScalingTrigger(std::shared_ptr<ScalingTrigger>* scalingTriggerSPtr)
{
    //if the scaling policy is None that means the user is calling update service and resetting it so no need to keep it
    if (*scalingTriggerSPtr != nullptr && (*scalingTriggerSPtr)->Kind == ScalingTriggerKind::Invalid)
    {
        scalingTriggerSPtr->reset();
        *scalingTriggerSPtr = nullptr;
    }
    return S_OK;
}

Serialization::IFabricSerializable * ScalingTrigger::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(ScalingTriggerKind::Enum))
    {
        return nullptr;
    }

    ScalingTriggerKind::Enum kind = *(reinterpret_cast<ScalingTriggerKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case ScalingTriggerKind::AveragePartitionLoad:
        return new AveragePartitionLoadScalingTrigger();
    case ScalingTriggerKind::AverageServiceLoad:
        return new AverageServiceLoadScalingTrigger();
    default:
        return nullptr;
    }
}

 ScalingTriggerSPtr ScalingTrigger::CreateSPtr(ScalingTriggerKind::Enum kind)
{
    switch (kind)
    {
    case ScalingTriggerKind::AveragePartitionLoad:
        return make_shared<AveragePartitionLoadScalingTrigger>();
    case ScalingTriggerKind::AverageServiceLoad:
        return make_shared<AverageServiceLoadScalingTrigger>();
    default:
        return nullptr;
    }
}

Common::ErrorCode ScalingTrigger::FromPublicApi(FABRIC_SCALING_TRIGGER const & policy)
{
    UNREFERENCED_PARAMETER(policy);
    kind_ = ScalingTriggerKind::Invalid;
    return ErrorCodeValue::Success;
}

void ScalingTrigger::ToPublicApi(Common::ScopedHeap & heap, FABRIC_SCALING_TRIGGER & scalingTrigger) const
{
    UNREFERENCED_PARAMETER(heap);
    scalingTrigger.ScalingTriggerKind = FABRIC_SCALING_TRIGGER_KIND_INVALID;
    scalingTrigger.ScalingTriggerDescription = NULL;
}

Common::ErrorCode ScalingTrigger::Validate(bool isStateful) const
{
    UNREFERENCED_PARAMETER(isStateful);
    return ErrorCodeValue::InvalidServiceScalingPolicy;
}

