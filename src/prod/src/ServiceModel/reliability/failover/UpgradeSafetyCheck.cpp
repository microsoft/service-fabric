// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace std;

UpgradeSafetyCheckWrapper::UpgradeSafetyCheckWrapper()
    : safetyCheck_()
{
}

UpgradeSafetyCheckWrapper::UpgradeSafetyCheckWrapper(
    UpgradeSafetyCheckSPtr && safetyCheck)
    : safetyCheck_(move(safetyCheck))
{
}

UpgradeSafetyCheckWrapper::UpgradeSafetyCheckWrapper(
    UpgradeSafetyCheckSPtr const& safetyCheck)
    : safetyCheck_(safetyCheck)
{
}

UpgradeSafetyCheckWrapper::UpgradeSafetyCheckWrapper(UpgradeSafetyCheckWrapper && other)
    : safetyCheck_(move(other.safetyCheck_))
{
}

UpgradeSafetyCheckWrapper & UpgradeSafetyCheckWrapper::operator=(UpgradeSafetyCheckWrapper && other)
{
    if (this != &other)
    {
        safetyCheck_ = move(other.safetyCheck_);
    }

    return *this;
}

UpgradeSafetyCheckWrapper::UpgradeSafetyCheckWrapper(UpgradeSafetyCheckWrapper const& other)
    : safetyCheck_(other.safetyCheck_)
{
}

UpgradeSafetyCheckWrapper & UpgradeSafetyCheckWrapper::operator=(UpgradeSafetyCheckWrapper const & other)
{
    if (this != &other)
    {
        safetyCheck_ = other.safetyCheck_;
    }

    return *this;
}

bool UpgradeSafetyCheckWrapper::operator==(UpgradeSafetyCheckWrapper const & other) const
{
    return this->Equals(other, false);
}

bool UpgradeSafetyCheckWrapper::operator!=(UpgradeSafetyCheckWrapper const & other) const
{
    return !(*this == other);
}

bool UpgradeSafetyCheckWrapper::Equals(UpgradeSafetyCheckWrapper const & other, bool ignoreDynamicContent) const
{
    if (safetyCheck_ && other.safetyCheck_) 
    { 
        return safetyCheck_->Equals(*other.safetyCheck_, ignoreDynamicContent);
    }
    else if (!safetyCheck_ && !other.safetyCheck_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void UpgradeSafetyCheckWrapper::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const
{
    if (safetyCheck_)
    {
        return safetyCheck_->ToPublicApi(heap, safetyCheck);
    }
}

ErrorCode UpgradeSafetyCheckWrapper::FromPublicApi(
    FABRIC_UPGRADE_SAFETY_CHECK const & safetyCheck)
{
    switch (safetyCheck.Kind)
    {
    case FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM:
    {
        safetyCheck_ = make_shared<SeedNodeUpgradeSafetyCheck>();
        return safetyCheck_->FromPublicApi(safetyCheck);
    }

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM:
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT:
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP:
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION:
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA:
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY:
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RESOURCE_AVAILABILITY:
    {
        safetyCheck_ = make_shared<PartitionUpgradeSafetyCheck>();
        return safetyCheck_->FromPublicApi(safetyCheck);
    }

    default:
        return ErrorCodeValue::InvalidArgument;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UpgradeSafetyCheck::UpgradeSafetyCheck()
    : kind_(UpgradeSafetyCheckKind::Invalid)
{
}

UpgradeSafetyCheck::UpgradeSafetyCheck(UpgradeSafetyCheckKind::Enum kind)
    : kind_(kind)
{
}

bool UpgradeSafetyCheck::Equals(UpgradeSafetyCheck const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(other);
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    return false;
}

void UpgradeSafetyCheck::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const
{
    UNREFERENCED_PARAMETER(heap);
    UNREFERENCED_PARAMETER(safetyCheck);
}

ErrorCode UpgradeSafetyCheck::FromPublicApi(
    FABRIC_UPGRADE_SAFETY_CHECK const & safetyCheck)
{
    UNREFERENCED_PARAMETER(safetyCheck);
    return ErrorCodeValue::NotImplemented;
}

Serialization::IFabricSerializable * UpgradeSafetyCheck::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(UpgradeSafetyCheckKind::Enum))
    {
        return nullptr;
    }

    UpgradeSafetyCheckKind::Enum kind = *(reinterpret_cast<UpgradeSafetyCheckKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case Reliability::UpgradeSafetyCheckKind::EnsureSeedNodeQuorum:
        return new SeedNodeUpgradeSafetyCheck();
    case Reliability::UpgradeSafetyCheckKind::EnsurePartitionQuorum:
    case Reliability::UpgradeSafetyCheckKind::WaitForPrimaryPlacement:
    case Reliability::UpgradeSafetyCheckKind::WaitForPrimarySwap:
    case Reliability::UpgradeSafetyCheckKind::WaitForReconfiguration:
    case Reliability::UpgradeSafetyCheckKind::WaitForInBuildReplica:
    case Reliability::UpgradeSafetyCheckKind::EnsureAvailability:
    case Reliability::UpgradeSafetyCheckKind::WaitForResourceAvailability:
        return new PartitionUpgradeSafetyCheck();
    default:
        return new UpgradeSafetyCheck();
    }
}

NTSTATUS UpgradeSafetyCheck::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

UpgradeSafetyCheckSPtr UpgradeSafetyCheck::CreateSPtr(UpgradeSafetyCheckKind::Enum kind)
{
    switch (kind)
    {
    case Reliability::UpgradeSafetyCheckKind::EnsureSeedNodeQuorum:
        return make_shared<SeedNodeUpgradeSafetyCheck>();
    case Reliability::UpgradeSafetyCheckKind::EnsurePartitionQuorum:
    case Reliability::UpgradeSafetyCheckKind::WaitForPrimaryPlacement:
    case Reliability::UpgradeSafetyCheckKind::WaitForPrimarySwap:
    case Reliability::UpgradeSafetyCheckKind::WaitForReconfiguration:
    case Reliability::UpgradeSafetyCheckKind::WaitForInBuildReplica:
    case Reliability::UpgradeSafetyCheckKind::EnsureAvailability:
    case Reliability::UpgradeSafetyCheckKind::WaitForResourceAvailability:
        return make_shared<PartitionUpgradeSafetyCheck>();
    default:
        return make_shared<UpgradeSafetyCheck>();
    }
}

void UpgradeSafetyCheck::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("SafetyCheck={0}", kind_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SeedNodeUpgradeSafetyCheck::SeedNodeUpgradeSafetyCheck()
{
}

SeedNodeUpgradeSafetyCheck::SeedNodeUpgradeSafetyCheck(UpgradeSafetyCheckKind::Enum kind)
    : UpgradeSafetyCheck(kind)
{
}

bool SeedNodeUpgradeSafetyCheck::Equals(UpgradeSafetyCheck const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    dynamic_cast<SeedNodeUpgradeSafetyCheck const &>(other);

    return true;
}

void SeedNodeUpgradeSafetyCheck::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const
{
    auto seedNodeSafetyCheck = heap.AddItem<FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK>();
    seedNodeSafetyCheck->Reserved = NULL;

    safetyCheck.Kind = FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM;
    safetyCheck.Value = seedNodeSafetyCheck.GetRawPointer();
}

ErrorCode SeedNodeUpgradeSafetyCheck::FromPublicApi(
    FABRIC_UPGRADE_SAFETY_CHECK const & safetyCheck)
{
    UNREFERENCED_PARAMETER(safetyCheck);

    kind_ = Reliability::UpgradeSafetyCheckKind::EnsureSeedNodeQuorum;

    return ErrorCodeValue::Success;
}

void SeedNodeUpgradeSafetyCheck::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("SafetyCheck={0}", kind_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PartitionUpgradeSafetyCheck::PartitionUpgradeSafetyCheck()
{
}

PartitionUpgradeSafetyCheck::PartitionUpgradeSafetyCheck(UpgradeSafetyCheckKind::Enum kind, Guid partitionId)
    : UpgradeSafetyCheck(kind),
      partitionId_(partitionId)
{
}

bool PartitionUpgradeSafetyCheck::Equals(UpgradeSafetyCheck const & other, bool ignoreDynamicContent) const
{
    auto const & casted = dynamic_cast<PartitionUpgradeSafetyCheck const &>(other);

    if (!ignoreDynamicContent)
    {
        if (partitionId_ != casted.partitionId_) { return false; }
    }

    return (kind_ == casted.kind_);
}

void PartitionUpgradeSafetyCheck::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_UPGRADE_SAFETY_CHECK & safetyCheck) const
{
    auto partitionSafetyCheck = heap.AddItem<FABRIC_UPGRADE_PARTITION_SAFETY_CHECK>();
    partitionSafetyCheck->PartitionId = partitionId_.AsGUID();
    partitionSafetyCheck->Reserved = NULL;

    switch (kind_)
    {
    case UpgradeSafetyCheckKind::EnsurePartitionQuorum:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM;
        break;
    case UpgradeSafetyCheckKind::WaitForPrimaryPlacement:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT;
        break;
    case UpgradeSafetyCheckKind::WaitForPrimarySwap:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP;
        break;
    case UpgradeSafetyCheckKind::WaitForReconfiguration:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION;
        break;
    case UpgradeSafetyCheckKind::WaitForInBuildReplica:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA;
        break;
    case UpgradeSafetyCheckKind::EnsureAvailability:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY;
        break;
    case UpgradeSafetyCheckKind::WaitForResourceAvailability:
        safetyCheck.Kind = FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RESOURCE_AVAILABILITY;
        break;
    default:
        safetyCheck.Kind = FABRIC_UPGRADE_SAFETY_CHECK_KIND_INVALID;
        break;
    }

    safetyCheck.Value = partitionSafetyCheck.GetRawPointer();
}

ErrorCode PartitionUpgradeSafetyCheck::FromPublicApi(
    FABRIC_UPGRADE_SAFETY_CHECK const & safetyCheck)
{
    auto const * casted = static_cast<FABRIC_UPGRADE_PARTITION_SAFETY_CHECK*>(safetyCheck.Value);
    partitionId_ = Guid(casted->PartitionId);

    switch (safetyCheck.Kind)
    {
    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM:
        kind_ = Reliability::UpgradeSafetyCheckKind::EnsurePartitionQuorum;
        break;

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT:
        kind_ = Reliability::UpgradeSafetyCheckKind::WaitForPrimaryPlacement;
        break;

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP:
        kind_ = Reliability::UpgradeSafetyCheckKind::WaitForPrimarySwap;
        break;

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION:
        kind_ = Reliability::UpgradeSafetyCheckKind::WaitForReconfiguration;
        break;

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA:
        kind_ = Reliability::UpgradeSafetyCheckKind::WaitForInBuildReplica;
        break;

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY:
        kind_ = Reliability::UpgradeSafetyCheckKind::EnsureAvailability;
        break;

    case FABRIC_UPGRADE_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RESOURCE_AVAILABILITY:
        kind_ = Reliability::UpgradeSafetyCheckKind::WaitForResourceAvailability;
        break;

    default:
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

void PartitionUpgradeSafetyCheck::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("SafetyCheck={0}, PartitionId={1}", kind_, partitionId_);
}
