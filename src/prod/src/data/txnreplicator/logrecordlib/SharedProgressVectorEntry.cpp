// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;

SharedProgressVectorEntry::SharedProgressVectorEntry(__in SharedProgressVectorEntry const & other)
    : sourceIndex_(other.sourceIndex_)
    , targetIndex_(other.targetIndex_)
    , sourceProgressEntry_(other.sourceProgressEntry_)
    , targetProgressEntry_(other.targetProgressEntry_)
    , fullCopyReason_(other.fullCopyReason_)
    , failedValidationMsg_(other.failedValidationMsg_)
{
}

SharedProgressVectorEntry::SharedProgressVectorEntry(
    __in ULONG sourceindex,
    __in ProgressVectorEntry const & sourceProgressVector,
    __in ULONG targetIndex,
    __in ProgressVectorEntry const & targetProgressVector,
    __in FullCopyReason::Enum fullCopyReason)
    : KObject()
    , sourceIndex_(sourceindex)
    , targetIndex_(targetIndex)
    , sourceProgressEntry_(sourceProgressVector)
    , targetProgressEntry_(targetProgressVector)
    , fullCopyReason_(fullCopyReason)
    , failedValidationMsg_()
{
}

SharedProgressVectorEntry::SharedProgressVectorEntry(
    __in ULONG sourceindex,
    __in ProgressVectorEntry const & sourceProgressVector,
    __in ULONG targetIndex,
    __in ProgressVectorEntry const & targetProgressVector,
    __in FullCopyReason::Enum fullCopyReason,
    __in std::string const & failedValidationMsg)
    : KObject()
    , sourceIndex_(sourceindex)
    , targetIndex_(targetIndex)
    , sourceProgressEntry_(sourceProgressVector)
    , targetProgressEntry_(targetProgressVector)
    , fullCopyReason_(fullCopyReason)
    , failedValidationMsg_(failedValidationMsg)
{
}

SharedProgressVectorEntry SharedProgressVectorEntry::CreateEmptySharedProgressVectorEntry()
{
    return SharedProgressVectorEntry(
        0,
        ProgressVectorEntry::ZeroProgressVectorEntry(),
        0,
        ProgressVectorEntry::ZeroProgressVectorEntry(),
        FullCopyReason::Enum::Invalid);
}

SharedProgressVectorEntry SharedProgressVectorEntry::CreateTrimmedSharedProgressVectorEntry()
{
    return SharedProgressVectorEntry(
        0,
        ProgressVectorEntry::ZeroProgressVectorEntry(),
        0,
        ProgressVectorEntry::ZeroProgressVectorEntry(),
        FullCopyReason::Enum::ProgressVectorTrimmed);
}
