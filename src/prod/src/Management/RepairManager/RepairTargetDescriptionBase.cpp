// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::RepairManager;

RepairTargetDescriptionBase::RepairTargetDescriptionBase()
    : kind_(RepairTargetKind::Invalid)
{
}

RepairTargetDescriptionBase::RepairTargetDescriptionBase(RepairTargetKind::Enum kind)
    : kind_(kind)
{
}

RepairTargetDescriptionBase::RepairTargetDescriptionBase(RepairTargetDescriptionBase const & other)
    : kind_(other.kind_)
{
}

RepairTargetDescriptionBase::RepairTargetDescriptionBase(RepairTargetDescriptionBase && other)
    : kind_(move(other.kind_))
{
}

bool RepairTargetDescriptionBase::operator == (RepairTargetDescriptionBase const & other) const
{
    return (kind_ == other.kind_);
}

bool RepairTargetDescriptionBase::operator != (RepairTargetDescriptionBase const & other) const
{
    return !(*this == other);
}

ErrorCode RepairTargetDescriptionBase::FromPublicApi(FABRIC_REPAIR_TARGET_DESCRIPTION const & publicDescription)
{
    UNREFERENCED_PARAMETER(publicDescription);
    return ErrorCodeValue::InvalidArgument;
}

void RepairTargetDescriptionBase::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_TARGET_DESCRIPTION & result) const
{
    UNREFERENCED_PARAMETER(heap);
    result.Kind = RepairTargetKind::ToPublicApi(kind_);
    result.Value = NULL;
}

void RepairTargetDescriptionBase::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("RepairTargetDescriptionBase[{0}]", kind_);
}

bool RepairTargetDescriptionBase::IsValid() const
{
    return false;
}

RepairTargetDescriptionBaseSPtr RepairTargetDescriptionBase::Clone() const
{
    return make_shared<RepairTargetDescriptionBase>(kind_);
}

Serialization::IFabricSerializable * RepairTargetDescriptionBase::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(FABRIC_REPAIR_TARGET_KIND))
    {
        return nullptr;
    }

    FABRIC_REPAIR_TARGET_KIND kind = *(reinterpret_cast<FABRIC_REPAIR_TARGET_KIND const *>(typeInformation.buffer));
    return CreateNew(kind);
}

NTSTATUS RepairTargetDescriptionBase::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

RepairTargetDescriptionBase * RepairTargetDescriptionBase::CreateNew(FABRIC_REPAIR_TARGET_KIND kind)
{
    switch (kind)
    {
    case FABRIC_REPAIR_TARGET_KIND_NODE:
        return RepairTargetSerializationTypeActivator<FABRIC_REPAIR_TARGET_KIND_NODE>::CreateNew();
    default:
        return new RepairTargetDescriptionBase(RepairTargetKind::Invalid);
    }
}

RepairTargetDescriptionBaseSPtr RepairTargetDescriptionBase::CreateSPtr(FABRIC_REPAIR_TARGET_KIND kind)
{
    switch (kind)
    {
    case FABRIC_REPAIR_TARGET_KIND_NODE:
        return RepairTargetSerializationTypeActivator<FABRIC_REPAIR_TARGET_KIND_NODE>::CreateSPtr();
    default:
        return make_shared<RepairTargetDescriptionBase>(RepairTargetKind::Invalid);
    }
}
