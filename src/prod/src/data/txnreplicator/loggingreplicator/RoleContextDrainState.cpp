// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;

RoleContextDrainState::RoleContextDrainState(__in IStatefulPartition & partition)
    : KShared()
    , KObject()
    , lock_()
    , partition_(&partition)
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , drainingStream_(DrainingStream::Enum::Invalid)
    , applyRedoContext_(ApplyContext::Enum::Invalid)
    , isClosing_(false)
{
    Reuse();
}

RoleContextDrainState::~RoleContextDrainState()
{
}

RoleContextDrainState::SPtr RoleContextDrainState::Create(
    __in IStatefulPartition & partition,
    __in KAllocator & allocator)
{
    RoleContextDrainState * pointer = _new(LOGGINGREPLICATOR_TAG, allocator) RoleContextDrainState(partition);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return RoleContextDrainState::SPtr(pointer);
}

void RoleContextDrainState::Reuse()
{
    {
        lock_.AcquireExclusive();

        role_ = FABRIC_REPLICA_ROLE_UNKNOWN;
        drainingStream_ = DrainingStream::Enum::Invalid;
        applyRedoContext_ = ApplyContext::Enum::Invalid;
        isClosing_.store(false);

        lock_.ReleaseExclusive();
    }
}

void RoleContextDrainState::ReportFault()
{
    bool expected = false;
    bool success = isClosing_.compare_exchange_weak(expected, true);

    if (!success)
    {
        return;
    }

    ASSERT_IFNOT(
        partition_ != nullptr,
        "ReportFault | Partition must not be null");

    partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
}

void RoleContextDrainState::OnClosing()
{
    isClosing_.store(true);
}

void RoleContextDrainState::OnRecovery()
{
    {
        lock_.AcquireExclusive();

        ASSERT_IFNOT(drainingStream_ == DrainingStream::Enum::Invalid, "Unexpected valid drain stream state on recovery: {0}", drainingStream_);
        ASSERT_IFNOT(role_ == FABRIC_REPLICA_ROLE_UNKNOWN, "Invalid replica role on recovery: {0}", role_);

        drainingStream_ = DrainingStream::Enum::Recovery;
        applyRedoContext_ = ApplyContext::Enum::RecoveryRedo;

        lock_.ReleaseExclusive();
    }
}

void RoleContextDrainState::OnRecoveryCompleted()
{
    {
        lock_.AcquireExclusive();

        ASSERT_IFNOT(drainingStream_ == DrainingStream::Enum::Recovery, "Unexpected valid drain stream state on recovery complete: {0}", drainingStream_);
        ASSERT_IFNOT(role_ == FABRIC_REPLICA_ROLE_UNKNOWN, "Invalid replica role on recovery complete: {0}", role_);
        ASSERT_IFNOT(applyRedoContext_ == ApplyContext::Enum::RecoveryRedo, "Invalid apply redo context on recovery complete");

        drainingStream_ = DrainingStream::Enum::Invalid;
        applyRedoContext_ = ApplyContext::Enum::Invalid;

        lock_.ReleaseExclusive();
    }
}

void RoleContextDrainState::OnDrainState()
{
    {
        lock_.AcquireExclusive();

        ASSERT_IFNOT(drainingStream_ == DrainingStream::Enum::Invalid, "Unexpected valid drain stream state: {0}", drainingStream_);
        ASSERT_IFNOT(role_ == FABRIC_REPLICA_ROLE_IDLE_SECONDARY, "Invalid replica role during drain: {0}", role_);

        drainingStream_ = DrainingStream::Enum::State;
        applyRedoContext_ = ApplyContext::Enum::Invalid;

        lock_.ReleaseExclusive();
    }
}

void RoleContextDrainState::OnDrainCopy()
{
    {
        lock_.AcquireExclusive();

        ASSERT_IFNOT(
            drainingStream_ == DrainingStream::Enum::Invalid || drainingStream_ == DrainingStream::Enum::State,
            "Unexpected valid drain stream state on copy: {0}", drainingStream_);
        ASSERT_IFNOT(role_ == FABRIC_REPLICA_ROLE_IDLE_SECONDARY, "Invalid replica role during drain copy: {0}", role_);

        drainingStream_ = DrainingStream::Enum::Copy;
        applyRedoContext_ = ApplyContext::Enum::SecondaryRedo;

        lock_.ReleaseExclusive();
    }
}

void RoleContextDrainState::OnDrainReplication()
{
    {
        lock_.AcquireExclusive();

        ASSERT_IFNOT(
            drainingStream_ == DrainingStream::Enum::Invalid ||
            drainingStream_ == DrainingStream::Enum::State ||
            drainingStream_ == DrainingStream::Enum::Copy,
            "Unexpected valid drain stream state on replication: {0}", drainingStream_);
        ASSERT_IFNOT(
            role_ == FABRIC_REPLICA_ROLE_IDLE_SECONDARY || role_ == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
            "Invalid replica role on drain replication: {0}", role_);

        drainingStream_ = DrainingStream::Enum::Replication;
        applyRedoContext_ = ApplyContext::Enum::SecondaryRedo;

        lock_.ReleaseExclusive();
    }
}

void RoleContextDrainState::BecomePrimaryCallerholdsLock()
{
    // U->P will have invalid draining stream
    // S/I -> P will have replication as the drain stream
    ASSERT_IFNOT(
        drainingStream_ == DrainingStream::Enum::Invalid ||
        drainingStream_ == DrainingStream::Enum::Replication ||
        drainingStream_ == DrainingStream::Enum::Copy,
        "Unexpected valid drain stream state on becoming Primary: {0}", drainingStream_);

    drainingStream_ = DrainingStream::Enum::Primary;
    applyRedoContext_ = ApplyContext::Enum::PrimaryRedo;
    role_ = FABRIC_REPLICA_ROLE_PRIMARY;
}

void RoleContextDrainState::BecomeActiveSecondaryCallerholdsLock()
{
    ASSERT_IFNOT(
        role_ == FABRIC_REPLICA_ROLE_IDLE_SECONDARY || role_ == FABRIC_REPLICA_ROLE_PRIMARY,
        "Invalid replica role during becoming Secondary replica: {0}", role_)

        role_ = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
}

void RoleContextDrainState::ChangeRole(__in FABRIC_REPLICA_ROLE newRole)
{
    {
        lock_.AcquireExclusive();

        if (role_ == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            ASSERT_IFNOT(
                newRole != FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                "Invalid replica role during change role: {0} {1}", role_, newRole);
        }

        if (role_ == FABRIC_REPLICA_ROLE_IDLE_SECONDARY && newRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            BecomeActiveSecondaryCallerholdsLock();
        }
        else if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            BecomePrimaryCallerholdsLock();
        }
        else
        {
            role_ = newRole;

            applyRedoContext_ = ApplyContext::Invalid;
            drainingStream_ = DrainingStream::Enum::Invalid;
        }

        lock_.ReleaseExclusive();
    }
}
