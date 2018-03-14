// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace std;

INITIALIZE_SIZE_ESTIMATION(SafetyCheckWrapper);

SafetyCheckWrapper::SafetyCheckWrapper()
    : safetyCheck_()
{
}

SafetyCheckWrapper::SafetyCheckWrapper(
    SafetyCheckSPtr && safetyCheck)
    : safetyCheck_(move(safetyCheck))
{
}

SafetyCheckWrapper::SafetyCheckWrapper(
    SafetyCheckSPtr const& safetyCheck)
    : safetyCheck_(safetyCheck)
{
}

SafetyCheckWrapper::SafetyCheckWrapper(SafetyCheckWrapper && other)
    : safetyCheck_(move(other.safetyCheck_))
{
}

SafetyCheckWrapper & SafetyCheckWrapper::operator=(SafetyCheckWrapper && other)
{
    if (this != &other)
    {
        safetyCheck_ = move(other.safetyCheck_);
    }

    return *this;
}

SafetyCheckWrapper::SafetyCheckWrapper(SafetyCheckWrapper const& other)
    : safetyCheck_(other.safetyCheck_)
{
}

SafetyCheckWrapper & SafetyCheckWrapper::operator=(SafetyCheckWrapper const & other)
{
    if (this != &other)
    {
        safetyCheck_ = other.safetyCheck_;
    }

    return *this;
}

bool SafetyCheckWrapper::operator==(SafetyCheckWrapper const & other) const
{
    return this->Equals(other, false);
}

bool SafetyCheckWrapper::operator!=(SafetyCheckWrapper const & other) const
{
    return !(*this == other);
}

bool SafetyCheckWrapper::Equals(SafetyCheckWrapper const & other, bool ignoreDynamicContent) const
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

ErrorCode SafetyCheckWrapper::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_SAFETY_CHECK & safetyCheck) const
{
    if (safetyCheck_)
    {
        return safetyCheck_->ToPublicApi(heap, safetyCheck);
    }

    return ErrorCode::Success();
}

ErrorCode SafetyCheckWrapper::FromPublicApi(
    FABRIC_SAFETY_CHECK const & safetyCheck)
{
    switch (safetyCheck.Kind)
    {
    case FABRIC_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM:
    {
        safetyCheck_ = make_shared<SeedNodeSafetyCheck>();
        return safetyCheck_->FromPublicApi(safetyCheck);
    }

    case FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM:
    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT:
    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP:
    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION:
    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA:
    case FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY:
    {
        safetyCheck_ = make_shared<PartitionSafetyCheck>();
        return safetyCheck_->FromPublicApi(safetyCheck);
    }

    default:
        return ErrorCodeValue::InvalidArgument;
    }
}

void SafetyCheckWrapper::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write(*safetyCheck_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION(SafetyCheck);

SafetyCheck::SafetyCheck()
    : kind_(SafetyCheckKind::Invalid)
{
}

SafetyCheck::SafetyCheck(SafetyCheckKind::Enum kind)
    : kind_(kind)
{
}

bool SafetyCheck::Equals(SafetyCheck const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(other);
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    return false;
}

ErrorCode SafetyCheck::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_SAFETY_CHECK & safetyCheck) const
{
    UNREFERENCED_PARAMETER(heap);
    UNREFERENCED_PARAMETER(safetyCheck);

    return ErrorCode::Success();
}

ErrorCode SafetyCheck::FromPublicApi(
    FABRIC_SAFETY_CHECK const & safetyCheck)
{
    UNREFERENCED_PARAMETER(safetyCheck);
    return ErrorCodeValue::NotImplemented;
}

Serialization::IFabricSerializable * SafetyCheck::FabricSerializerActivator(
    Serialization::FabricTypeInformation typeInformation)
{
    if (typeInformation.buffer == nullptr ||
        typeInformation.length != sizeof(SafetyCheckKind::Enum))
    {
        return nullptr;
    }

    SafetyCheckKind::Enum kind = *(reinterpret_cast<SafetyCheckKind::Enum const *>(typeInformation.buffer));

    switch (kind)
    {
    case Reliability::SafetyCheckKind::EnsureSeedNodeQuorum:
        return new SeedNodeSafetyCheck();
    case Reliability::SafetyCheckKind::EnsurePartitionQuorum:
    case Reliability::SafetyCheckKind::WaitForPrimaryPlacement:
    case Reliability::SafetyCheckKind::WaitForPrimarySwap:
    case Reliability::SafetyCheckKind::WaitForReconfiguration:
    case Reliability::SafetyCheckKind::WaitForInBuildReplica:
    case Reliability::SafetyCheckKind::EnsureAvailability:
        return new PartitionSafetyCheck();
    default:
        return new SafetyCheck();
    }
}

NTSTATUS SafetyCheck::GetTypeInformation(
    __out Serialization::FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&kind_);
    typeInformation.length = sizeof(kind_);
    return STATUS_SUCCESS;
}

SafetyCheckSPtr SafetyCheck::CreateSPtr(SafetyCheckKind::Enum kind)
{
    switch (kind)
    {
    case Reliability::SafetyCheckKind::EnsureSeedNodeQuorum:
        return make_shared<SeedNodeSafetyCheck>();
    case Reliability::SafetyCheckKind::EnsurePartitionQuorum:
    case Reliability::SafetyCheckKind::WaitForPrimaryPlacement:
    case Reliability::SafetyCheckKind::WaitForPrimarySwap:
    case Reliability::SafetyCheckKind::WaitForReconfiguration:
    case Reliability::SafetyCheckKind::WaitForInBuildReplica:
    case Reliability::SafetyCheckKind::EnsureAvailability:
        return make_shared<PartitionSafetyCheck>();
    default:
        return make_shared<SafetyCheck>();
    }
}

void SafetyCheck::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("SafetyCheck={0}", kind_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INITIALIZE_SIZE_ESTIMATION(SeedNodeSafetyCheck);

SeedNodeSafetyCheck::SeedNodeSafetyCheck()
{
}

SeedNodeSafetyCheck::SeedNodeSafetyCheck(SafetyCheckKind::Enum kind)
    : SafetyCheck(kind)
{
}

bool SeedNodeSafetyCheck::Equals(SafetyCheck const & other, bool ignoreDynamicContent) const
{
    UNREFERENCED_PARAMETER(ignoreDynamicContent);
    dynamic_cast<SeedNodeSafetyCheck const &>(other);

    return true;
}

ErrorCode SeedNodeSafetyCheck::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_SAFETY_CHECK & safetyCheck) const
{
    auto seedNodeSafetyCheck = heap.AddItem<FABRIC_SEED_NODE_SAFETY_CHECK>();
    seedNodeSafetyCheck->Reserved = NULL;

    safetyCheck.Kind = FABRIC_SEED_NODE_SAFETY_CHECK_KIND_ENSURE_QUORUM;
    safetyCheck.Value = seedNodeSafetyCheck.GetRawPointer();

    return ErrorCode::Success();
}

ErrorCode SeedNodeSafetyCheck::FromPublicApi(
    FABRIC_SAFETY_CHECK const & safetyCheck)
{
    UNREFERENCED_PARAMETER(safetyCheck);

    kind_ = Reliability::SafetyCheckKind::EnsureSeedNodeQuorum;

    return ErrorCodeValue::Success;
}

void SeedNodeSafetyCheck::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("SafetyCheck={0}", kind_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INITIALIZE_SIZE_ESTIMATION(PartitionSafetyCheck);

PartitionSafetyCheck::PartitionSafetyCheck()
{
}

PartitionSafetyCheck::PartitionSafetyCheck(SafetyCheckKind::Enum kind, Guid partitionId)
    : SafetyCheck(kind),
      partitionId_(partitionId)
{
}

bool PartitionSafetyCheck::Equals(SafetyCheck const & other, bool ignoreDynamicContent) const
{
    auto const & casted = dynamic_cast<PartitionSafetyCheck const &>(other);

    if (!ignoreDynamicContent)
    {
        if (partitionId_ != casted.partitionId_) { return false; }
    }

    return (kind_ == casted.kind_);
}

ErrorCode PartitionSafetyCheck::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_SAFETY_CHECK & safetyCheck) const
{
    auto partitionSafetyCheck = heap.AddItem<FABRIC_PARTITION_SAFETY_CHECK>();
    partitionSafetyCheck->PartitionId = partitionId_.AsGUID();
    partitionSafetyCheck->Reserved = NULL;

    switch (kind_)
    {
    case SafetyCheckKind::EnsurePartitionQuorum:
        safetyCheck.Kind = FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM;
        break;
    case SafetyCheckKind::WaitForPrimaryPlacement:
        safetyCheck.Kind = FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT;
        break;
    case SafetyCheckKind::WaitForPrimarySwap:
        safetyCheck.Kind = FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP;
        break;
    case SafetyCheckKind::WaitForReconfiguration:
        safetyCheck.Kind = FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION;
        break;
    case SafetyCheckKind::WaitForInBuildReplica:
        safetyCheck.Kind = FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA;
        break;
    case SafetyCheckKind::EnsureAvailability:
        safetyCheck.Kind = FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY;
        break;
    default:
        safetyCheck.Kind = FABRIC_SAFETY_CHECK_KIND_INVALID;
        break;
    }

    safetyCheck.Value = partitionSafetyCheck.GetRawPointer();

    return ErrorCode::Success();
}

ErrorCode PartitionSafetyCheck::FromPublicApi(
    FABRIC_SAFETY_CHECK const & safetyCheck)
{
    auto const * casted = static_cast<FABRIC_PARTITION_SAFETY_CHECK*>(safetyCheck.Value);
    partitionId_ = Guid(casted->PartitionId);

    switch (safetyCheck.Kind)
    {
    case FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_QUORUM:
        kind_ = Reliability::SafetyCheckKind::EnsurePartitionQuorum;
        break;

    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_PLACEMENT:
        kind_ = Reliability::SafetyCheckKind::WaitForPrimaryPlacement;
        break;

    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_PRIMARY_SWAP:
        kind_ = Reliability::SafetyCheckKind::WaitForPrimarySwap;
        break;

    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_RECONFIGURATION:
        kind_ = Reliability::SafetyCheckKind::WaitForReconfiguration;
        break;

    case FABRIC_PARTITION_SAFETY_CHECK_KIND_WAIT_FOR_INBUILD_REPLICA:
        kind_ = Reliability::SafetyCheckKind::WaitForInBuildReplica;
        break;

    case FABRIC_PARTITION_SAFETY_CHECK_KIND_ENSURE_AVAILABILITY:
        kind_ = Reliability::SafetyCheckKind::EnsureAvailability;
        break;

    default:
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

void PartitionSafetyCheck::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.Write("SafetyCheck={0}, PartitionId={1}", kind_, partitionId_);
}
