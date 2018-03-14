// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::RepairManager;

RepairImpactDescriptionBase::RepairImpactDescriptionBase()
: kind_(RepairImpactKind::Invalid)
{
}

RepairImpactDescriptionBase::RepairImpactDescriptionBase(RepairImpactKind::Enum kind)
    : kind_(kind)
{
}

RepairImpactDescriptionBase::RepairImpactDescriptionBase(RepairImpactDescriptionBase const & other)
    : kind_(other.kind_)
{
}

RepairImpactDescriptionBase::RepairImpactDescriptionBase(RepairImpactDescriptionBase && other)
    : kind_(move(other.kind_))
{
}

bool RepairImpactDescriptionBase::operator == (RepairImpactDescriptionBase const & other) const
{
    return (kind_ == other.kind_);
}

bool RepairImpactDescriptionBase::operator != (RepairImpactDescriptionBase const & other) const
{
    return !(*this == other);
}

ErrorCode RepairImpactDescriptionBase::FromPublicApi(FABRIC_REPAIR_IMPACT_DESCRIPTION const & publicDescription)
{
    UNREFERENCED_PARAMETER(publicDescription);
    return ErrorCodeValue::InvalidArgument;
}

void RepairImpactDescriptionBase::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_IMPACT_DESCRIPTION & result) const
{
    UNREFERENCED_PARAMETER(heap);
    result.Kind = RepairImpactKind::ToPublicApi(kind_);
    result.Value = NULL;
}

void RepairImpactDescriptionBase::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("RepairImpactDescriptionBase[{0}]", kind_);
}

bool RepairImpactDescriptionBase::IsValid() const
{
    return false;
}

bool RepairImpactDescriptionBase::IsZeroImpact() const
{
    return true;
}

RepairImpactDescriptionBaseSPtr RepairImpactDescriptionBase::Clone() const
{
    return make_shared<RepairImpactDescriptionBase>(kind_);
}

Serialization::IFabricSerializable * RepairImpactDescriptionBase::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(FABRIC_REPAIR_IMPACT_KIND))
    {
        return nullptr;
    }

    FABRIC_REPAIR_IMPACT_KIND kind = *(reinterpret_cast<FABRIC_REPAIR_IMPACT_KIND const *>(typeInformation.buffer));
    return CreateNew(kind);
}

NTSTATUS RepairImpactDescriptionBase::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

RepairImpactDescriptionBase * RepairImpactDescriptionBase::CreateNew(FABRIC_REPAIR_IMPACT_KIND kind)
{
    switch (kind)
    {
    case FABRIC_REPAIR_IMPACT_KIND_NODE:
        return RepairImpactSerializationTypeActivator<FABRIC_REPAIR_IMPACT_KIND_NODE>::CreateNew();
    default:
        return new RepairImpactDescriptionBase(RepairImpactKind::Invalid);
    }
}

RepairImpactDescriptionBaseSPtr RepairImpactDescriptionBase::CreateSPtr(FABRIC_REPAIR_IMPACT_KIND kind)
{
    switch (kind)
    {
    case FABRIC_REPAIR_IMPACT_KIND_NODE:
        return RepairImpactSerializationTypeActivator<FABRIC_REPAIR_IMPACT_KIND_NODE>::CreateSPtr();
    default:
        return make_shared<RepairImpactDescriptionBase>(RepairImpactKind::Invalid);
    }
}
