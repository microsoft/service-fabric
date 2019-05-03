// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(AutoScalingTrigger)

AutoScalingTrigger::AutoScalingTrigger()
    : kind_(AutoScalingTriggerKind::AverageLoad)
{
}

AutoScalingTrigger::AutoScalingTrigger(AutoScalingTriggerKind::Enum kind)
    : kind_(kind)
{
}

bool AutoScalingTrigger::operator == (AutoScalingTriggerSPtr const & other) const
{
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

bool AutoScalingTrigger::Equals(AutoScalingTriggerSPtr const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    if (other == nullptr)
    {
        return false;
    }
    return kind_ == other->kind_;
}

Serialization::IFabricSerializable * AutoScalingTrigger::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(AutoScalingTriggerKind::Enum))
    {
        return nullptr;
    }

    AutoScalingTriggerKind::Enum kind = *(reinterpret_cast<AutoScalingTriggerKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case AutoScalingTriggerKind::AverageLoad:
        return new AverageLoadScalingTrigger();
    default:
        return nullptr;
    }
}

NTSTATUS AutoScalingTrigger::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

 AutoScalingTriggerSPtr AutoScalingTrigger::CreateSPtr(AutoScalingTriggerKind::Enum kind)
{
    switch (kind)
    {
    case AutoScalingTriggerKind::AverageLoad:
        return make_shared<AverageLoadScalingTrigger>();
    default:
        return nullptr;
    }
}

Common::ErrorCode AutoScalingTrigger::Validate() const
{
    return ErrorCodeValue::InvalidServiceScalingPolicy;
}
