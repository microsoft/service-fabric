// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(AutoScalingMechanism)

AutoScalingMechanism::AutoScalingMechanism()
    : kind_(AutoScalingMechanismKind::AddRemoveReplica)
{
}

AutoScalingMechanism::AutoScalingMechanism(AutoScalingMechanismKind::Enum kind)
    : kind_(kind)
{
}

bool AutoScalingMechanism::operator == (AutoScalingMechanismSPtr const & other) const
{
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

bool AutoScalingMechanism::Equals(AutoScalingMechanismSPtr const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}


Serialization::IFabricSerializable * AutoScalingMechanism::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(AutoScalingMechanismKind::Enum))
    {
        return nullptr;
    }

    AutoScalingMechanismKind::Enum kind = *(reinterpret_cast<AutoScalingMechanismKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case AutoScalingMechanismKind::AddRemoveReplica:
        return new AddRemoveReplicaScalingMechanism();
    default:
        return nullptr;
    }
}

NTSTATUS AutoScalingMechanism::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

AutoScalingMechanismSPtr AutoScalingMechanism::CreateSPtr(AutoScalingMechanismKind::Enum kind)
{
    switch (kind)
    {
    case AutoScalingMechanismKind::AddRemoveReplica:
        return make_shared<AddRemoveReplicaScalingMechanism>();
    default:
        return nullptr;
    }
}

Common::ErrorCode AutoScalingMechanism::Validate() const
{
    return ErrorCodeValue::InvalidServiceScalingPolicy;
}

