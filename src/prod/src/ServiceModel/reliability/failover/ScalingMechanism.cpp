// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

INITIALIZE_SIZE_ESTIMATION(ScalingMechanism)

ScalingMechanism::ScalingMechanism()
    : kind_(ScalingMechanismKind::Invalid)
{

}

ScalingMechanism::ScalingMechanism(ScalingMechanismKind::Enum kind)
    : kind_(kind)
{
}

bool ScalingMechanism::Equals(ScalingMechanismSPtr const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

void ScalingMechanism::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("{0}", kind_);
}

NTSTATUS ScalingMechanism::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

Serialization::IFabricSerializable * ScalingMechanism::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(ScalingMechanismKind::Enum))
    {
        return nullptr;
    }

    ScalingMechanismKind::Enum kind = *(reinterpret_cast<ScalingMechanismKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case ScalingMechanismKind::AddRemoveIncrementalNamedPartition:
        return new AddRemoveIncrementalNamedPartitionScalingMechanism();
    case ScalingMechanismKind::PartitionInstanceCount:
        return new InstanceCountScalingMechanism();
    default:
        return nullptr;
    }
}

HRESULT ScalingMechanism::CheckScalingMechanism(std::shared_ptr<ScalingMechanism> * scalingMechanismSptr)
{
    //if the scaling policy is None that means the user is calling update service and resetting it so no need to keep it
    if (*scalingMechanismSptr != nullptr && (*scalingMechanismSptr)->Kind == ScalingMechanismKind::Invalid)
    {
        scalingMechanismSptr->reset();
        *scalingMechanismSptr = nullptr;
    }
    return S_OK;
}

ScalingMechanismSPtr ScalingMechanism::CreateSPtr(ScalingMechanismKind::Enum kind)
{
    switch (kind)
    {
    case ScalingMechanismKind::AddRemoveIncrementalNamedPartition:
        return make_shared<AddRemoveIncrementalNamedPartitionScalingMechanism>();
    case ScalingMechanismKind::PartitionInstanceCount:
        return make_shared<InstanceCountScalingMechanism>();
    default:
        return nullptr;
    }
}

Common::ErrorCode ScalingMechanism::FromPublicApi(FABRIC_SCALING_MECHANISM const & mechanism)
{
    UNREFERENCED_PARAMETER(mechanism);
    kind_ = ScalingMechanismKind::Invalid;
    return ErrorCodeValue::Success;
}

void ScalingMechanism::ToPublicApi(Common::ScopedHeap & heap, FABRIC_SCALING_MECHANISM & scalingMechanism) const
{
    UNREFERENCED_PARAMETER(heap);
    scalingMechanism.ScalingMechanismKind = FABRIC_SCALING_MECHANISM_INVALID;
    scalingMechanism.ScalingMechanismDescription = NULL;
}

Common::ErrorCode ScalingMechanism::Validate(bool isStateful) const
{
    UNREFERENCED_PARAMETER(isStateful);
    return ErrorCodeValue::InvalidServiceScalingPolicy;
}
