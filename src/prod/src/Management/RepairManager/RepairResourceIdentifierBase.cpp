// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::RepairManager;

RepairScopeIdentifierBase::RepairScopeIdentifierBase()
    : kind_(RepairScopeIdentifierKind::Invalid)
{
}

RepairScopeIdentifierBase::RepairScopeIdentifierBase(RepairScopeIdentifierKind::Enum kind)
    : kind_(kind)
{
}

RepairScopeIdentifierBase::RepairScopeIdentifierBase(RepairScopeIdentifierBase const & other)
    : kind_(other.kind_)
{
}

RepairScopeIdentifierBase::RepairScopeIdentifierBase(RepairScopeIdentifierBase && other)
    : kind_(move(other.kind_))
{
}

bool RepairScopeIdentifierBase::operator == (RepairScopeIdentifierBase const & other) const
{
    return (kind_ == other.kind_);
}

bool RepairScopeIdentifierBase::operator != (RepairScopeIdentifierBase const & other) const
{
    return !(*this == other);
}

ErrorCode RepairScopeIdentifierBase::FromPublicApi(FABRIC_REPAIR_SCOPE_IDENTIFIER const & publicDescription)
{
    UNREFERENCED_PARAMETER(publicDescription);
    return ErrorCodeValue::InvalidArgument;
}

void RepairScopeIdentifierBase::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_SCOPE_IDENTIFIER & result) const
{
    UNREFERENCED_PARAMETER(heap);
    result.Kind = RepairScopeIdentifierKind::ToPublicApi(kind_);
    result.Value = NULL;
}

void RepairScopeIdentifierBase::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("RepairScopeIdentifierBase[{0}]", kind_);
}

Serialization::IFabricSerializable * RepairScopeIdentifierBase::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND))
    {
        return nullptr;
    }

    FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND kind = *(reinterpret_cast<FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND const *>(typeInformation.buffer));
    return CreateNew(kind);
}

NTSTATUS RepairScopeIdentifierBase::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

RepairScopeIdentifierBase * RepairScopeIdentifierBase::CreateNew(FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND kind)
{
    switch (kind)
    {
    case FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER:
        return RepairScopeIdentifierSerializationTypeActivator<FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER>::CreateNew();
    default:
        return new RepairScopeIdentifierBase();
    }
}

RepairScopeIdentifierBaseSPtr RepairScopeIdentifierBase::CreateSPtr(FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND kind)
{
    switch (kind)
    {
    case FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER:
        return RepairScopeIdentifierSerializationTypeActivator<FABRIC_REPAIR_SCOPE_IDENTIFIER_KIND_CLUSTER>::CreateSPtr();
    default:
        return make_shared<RepairScopeIdentifierBase>();
    }
}

ErrorCode RepairScopeIdentifierBase::CreateSPtrFromPublicApi(
    FABRIC_REPAIR_SCOPE_IDENTIFIER const & publicDescription,
    RepairScopeIdentifierBaseSPtr & internalPointer)
{
    internalPointer = CreateSPtr(publicDescription.Kind);
    return internalPointer->FromPublicApi(publicDescription);
}
