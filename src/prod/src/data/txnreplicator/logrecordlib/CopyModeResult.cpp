// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;

CopyModeResult::CopyModeResult(
    __in int copyMode,
    __in FullCopyReason::Enum fullCopyReason,
    __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
    __in LONG64 sourceStartingLsn,
    __in LONG64 targetStartingLsn)
    : KObject()
    , copyMode_(copyMode)
    , fullCopyReason_(fullCopyReason)
    , sharedProgressVectorEntry_(sharedProgressVectorEntry)
    , sourceStartingLsn_(sourceStartingLsn)
    , targetStartingLsn_(targetStartingLsn)
{
}

CopyModeResult::CopyModeResult(__in CopyModeResult const & other)
    : copyMode_(other.copyMode_)
    , fullCopyReason_(other.fullCopyReason_)
    , sharedProgressVectorEntry_(other.sharedProgressVectorEntry_)
    , sourceStartingLsn_(other.sourceStartingLsn_)
    , targetStartingLsn_(other.targetStartingLsn_)
{
}

CopyModeResult CopyModeResult::CreateCopyNoneResult(__in SharedProgressVectorEntry const & sharedProgressVectorEntry)
{
    return CopyModeResult(
        CopyModeFlag::Enum::None,
        FullCopyReason::Enum::Invalid,
        sharedProgressVectorEntry,
        Constants::InvalidLsn,
        Constants::InvalidLsn);
}

CopyModeResult CopyModeResult::CreateFullCopyResult(
    __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
    __in FullCopyReason::Enum fullCopyReason)
{
    ASSERT_IFNOT(
        fullCopyReason != FullCopyReason::Enum::Invalid, 
        "Invalid full copy reason in copy mode: {0}", fullCopyReason);

    return CopyModeResult(
        CopyModeFlag::Enum::Full,
        fullCopyReason,
        sharedProgressVectorEntry,
        Constants::InvalidLsn,
        Constants::InvalidLsn);
}

CopyModeResult CopyModeResult::CreatePartialCopyResult(
    __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
    __in LONG64 sourceStartingLsn,
    __in LONG64 targetStartingLsn)
{
    return CopyModeResult(
        CopyModeFlag::Enum::Partial,
        FullCopyReason::Enum::Invalid,
        sharedProgressVectorEntry,
        sourceStartingLsn,
        targetStartingLsn);
}

CopyModeResult CopyModeResult::CreateFalseProgressCopyResult(
    __in LONG64 lastRecoveredAtomicRedoOperationLsnAtTarget,
    __in SharedProgressVectorEntry const & sharedProgressVectorEntry,
    __in LONG64 sourceStartingLsn,
    __in LONG64 targetStartingLsn)
{
    if (lastRecoveredAtomicRedoOperationLsnAtTarget > sourceStartingLsn)
    {
        return CreateFullCopyResult(
            sharedProgressVectorEntry,
            FullCopyReason::Enum::AtomicRedoOperationFalseProgressed);
    }

    return CopyModeResult(
        CopyModeFlag::Enum::FalseProgress | CopyModeFlag::Enum::Partial,
        FullCopyReason::Enum::Invalid,
        sharedProgressVectorEntry,
        sourceStartingLsn,
        targetStartingLsn);
}
