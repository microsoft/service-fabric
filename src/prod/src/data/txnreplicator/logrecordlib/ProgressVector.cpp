// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

ProgressVector::ProgressVector()
    : KObject()
    , vectors_(GetThisAllocator())
    , progressVectorMaxEntries_(0)
{
    THROW_ON_CONSTRUCTOR_FAILURE(vectors_);
}

ProgressVector::~ProgressVector()
{
}

ProgressVector::SPtr ProgressVector::Create(__in KAllocator & allocator)
{
    ProgressVector * pointer = _new(PROGRESSVECTOR_TAG, allocator) ProgressVector();
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return ProgressVector::SPtr(pointer);
}

ProgressVector::SPtr ProgressVector::CreateZeroProgressVector(__in KAllocator & allocator)
{
    ProgressVector::SPtr progressVector = Create(allocator);
    progressVector->Append(ProgressVectorEntry::ZeroProgressVectorEntry());
    return progressVector;
}

void ProgressVector::Test_SetProgressVectorMaxEntries(__in ULONG progressVectorMaxEntries)
{
    progressVectorMaxEntries_ = progressVectorMaxEntries;
}

ProgressVector::SPtr ProgressVector::Clone(
    __in ProgressVector & progressVector,
    __in ULONG progressVectorMaxEntries,
    __in Epoch const & highestBackedUpEpoch,
    __in Epoch const & headEpoch,
    __in KAllocator & allocator)
{
    //try trimming the progress vector before cloning
    progressVector.TrimProgressVectorIfNeeded(highestBackedUpEpoch, headEpoch);

    ProgressVector::SPtr copiedVector = Create(allocator);
    copiedVector->progressVectorMaxEntries_ = progressVectorMaxEntries;

    for (ULONG i = 0; i < progressVector.Length; i++)
    {
        copiedVector->Append(progressVector.vectors_[i]);
    }

    return copiedVector;
}

bool ProgressVector::Equals(__in ProgressVector & other) const
{
    if (vectors_.Count() != other.vectors_.Count())
    {
        return false;
    }

    for (ULONG i = 0; i < vectors_.Count(); i++)
    {
        if (vectors_[i] != other.vectors_[i])
        {
            return false;
        }
    }

    return true;
}

bool ProgressVector::IsBrandNewReplica(CopyContextParameters const & copyContextParameters)
{
    return copyContextParameters.ProgressVectorData->Length == 1 &&
           copyContextParameters.ProgressVectorData->vectors_[0].CurrentEpoch.DataLossVersion == 0 &&
           copyContextParameters.ProgressVectorData->vectors_[0].Lsn == Constants::ZeroLsn &&
           copyContextParameters.LogTailLsn == Constants::OneLsn;
}

CopyModeResult ProgressVector::FindCopyMode(
    __in CopyContextParameters const & sourceParameters,
    __in CopyContextParameters const & targetParameters,
    __in LONG64 lastRecoveredAtomicRedoOperationLsnAtTarget)
{
    CopyModeResult result = FindCopyModePrivate(
        sourceParameters,
        targetParameters,
        lastRecoveredAtomicRedoOperationLsnAtTarget);

    if ((result.CopyModeEnum & CopyModeFlag::Enum::FalseProgress) != 0)
    {
        std::string msg = Common::formatString(
            "sourceStartingLsn is expected to be <= targetStartingLsn, source lsn : {0}, target lsn :{1}",
            result.SourceStartingLsn,
            result.TargetStartingLsn);

        if (!ValidateIfDebugEnabled(
            result.SourceStartingLsn <= result.TargetStartingLsn,
            msg))
        {
            SharedProgressVectorEntry failedValidationResult = SharedProgressVectorEntry(
                result.SharedProgressVector.SourceIndex,
                result.SharedProgressVector.SourceProgressVectorEntry,
                result.SharedProgressVector.TargetIndex,
                result.SharedProgressVector.TargetProgressVectorEntry,
                FullCopyReason::Enum::ValidationFailed,
                msg);

            return CopyModeResult::CreateFullCopyResult(
                failedValidationResult,
                failedValidationResult.FullCopyReason);
        }
    }

    return result;
}

CopyModeResult ProgressVector::FindCopyModePrivate(
    __in CopyContextParameters const & sourceParameters,
    __in CopyContextParameters const & targetParameters,
    __in LONG64 lastRecoveredAtomicRedoOperationLsnAtTarget)
{
    // We can perform the check for CASE 1(Shown Below) before finding the shared ProgressVectorEntry.
    // However, we do not do that because the shared ProgressVectorEntry method provides additional checks of epoch history
    SharedProgressVectorEntry sharedProgressVectorEntry = FindSharedVector(
        *sourceParameters.ProgressVectorData,
        *targetParameters.ProgressVectorData);

    //If we detect that the shared progress vector cannot be obtained because of a trimmed progress vector, we decide to do a full copy
    if (sharedProgressVectorEntry.FullCopyReason == FullCopyReason::Enum::ProgressVectorTrimmed)
    {
        return CopyModeResult::CreateFullCopyResult(
            SharedProgressVectorEntry::CreateEmptySharedProgressVectorEntry(),
            FullCopyReason::Enum::ProgressVectorTrimmed);
    }

    // Validation failed, perform Full Copy
    if (sharedProgressVectorEntry.FullCopyReason == FullCopyReason::Enum::ValidationFailed)
    {
        return CopyModeResult::CreateFullCopyResult(
            sharedProgressVectorEntry,
            FullCopyReason::Enum::ValidationFailed);
    }


    // *******************************************CASE 1 - COPY_NONE***********************************************************
    //
    // The last entry in the source and the target vectors are same with the same tail.
    // This can happen if target goes down and comes back up and the primary made no progress
    if (sourceParameters.ProgressVectorData->LastProgressVectorEntry == targetParameters.ProgressVectorData->LastProgressVectorEntry &&
        sourceParameters.LogTailLsn == targetParameters.LogTailLsn)
    { 
        // Note that copy none covers only cases where nothing needs to be copied (including the UpdateEpochLogRecord)
        // Hence, service creation scenario is Partial copy, not None.
        return CopyModeResult::CreateCopyNoneResult(sharedProgressVectorEntry);
    }


    // *******************************************CASE 2a - COPY_FULL - Due to Data Loss******************************************
    // 
    // Check for any dataloss in the source or target log
    // If there was data loss, perform full copy
    // No full copy with DataLoss reason, if the target is a brand new replica
    if (!IsBrandNewReplica(targetParameters))
    {
        if (sharedProgressVectorEntry.SourceProgressVectorEntry.IsDataLossBetween(sourceParameters.ProgressVectorData->LastProgressVectorEntry) ||
            sharedProgressVectorEntry.TargetProgressVectorEntry.IsDataLossBetween(targetParameters.ProgressVectorData->LastProgressVectorEntry) ||
            sourceParameters.LogHeadEpoch.DataLossVersion > sharedProgressVectorEntry.TargetProgressVectorEntry.CurrentEpoch.DataLossVersion ||
            targetParameters.LogHeadEpoch.DataLossVersion > sharedProgressVectorEntry.SourceProgressVectorEntry.CurrentEpoch.DataLossVersion)
        {
            return CopyModeResult::CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason::Enum::DataLoss);
        }
    }

    LONG64 sourceStartingLsn = (sharedProgressVectorEntry.SourceIndex == (sourceParameters.ProgressVectorData->Length - 1))
        ? sourceParameters.LogTailLsn 
        : sourceParameters.ProgressVectorData->vectors_[sharedProgressVectorEntry.SourceIndex + 1].Lsn;

    LONG64 targetStartingLsn = (sharedProgressVectorEntry.TargetIndex == (targetParameters.ProgressVectorData->Length - 1))
        ? targetParameters.LogTailLsn 
        : targetParameters.ProgressVectorData->vectors_[sharedProgressVectorEntry.TargetIndex + 1].Lsn;


    // *******************************************CASE 2b - COPY_FULL - Due to Insufficient Loss******************************************
    //
    // Since there was no data loss, check if we can perform partial copy and there are sufficient logs in the source 
    // and target to do so
    if (sourceParameters.LogHeadLsn > targetParameters.LogTailLsn ||
        sourceStartingLsn < sourceParameters.LogHeadLsn ||
        targetParameters.LogHeadLsn > sourceStartingLsn)
    {
        return CopyModeResult::CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason::Enum::InsufficientLogs);
    }

    // *******************************************CASE 3a - FALSE_PROGRESS -Basic Case******************************************
    //
    // The simple case where target made more progress in an epoch than the primary
    //          Source = (1,1,0) (1,3,10) (1,4,15) Tail = 16
    //          Target = (1,1,0) (1,3,10) Tail = 20
    if (sourceStartingLsn < targetStartingLsn)
    {
        return CopyModeResult::CreateFalseProgressCopyResult(
            lastRecoveredAtomicRedoOperationLsnAtTarget, 
            sharedProgressVectorEntry, 
            sourceStartingLsn, 
            targetStartingLsn);
    }


    // *******************************************CASE 3b - FALSE_PROGRESS -Advanced Case******************************************
    //
    // The cases where target has made progress in an epoch that the primary has no knowledge of
    // Series of crashes could lead to the target having made false progress in an epoch that the primary DOES NOT KNOW about
    //          Source = (1,1,0) (1,3,10) Tail = 15
    //          Target = (1,1,0) (1,2,10) Tail = 12

    //          Source = (1,1,0) (1,3,10) Tail = 15
    //          Target = (1,1,0) (1,2,10) Tail = 10

    // The case where double false progress is made 
    //          Source = (1,3,10) (1,5,17) Tail = 18
    //          Target = (1,3,10) (1,4,15) Tail = 17
    if (targetStartingLsn != targetParameters.LogTailLsn)
    {
        sourceStartingLsn = targetStartingLsn;

        return CopyModeResult::CreateFalseProgressCopyResult(
            lastRecoveredAtomicRedoOperationLsnAtTarget, 
            sharedProgressVectorEntry, 
            sourceStartingLsn, 
            targetStartingLsn);
    }

    // *******************************************CASE 4 - Work Around Case to be able to assert******************************************
    // RDBUG : 3374739
    //          Source = (1,1,0) (1,3,10) Tail >= 10
    //          Target = (1,1,0) (1,2,5) Tail = 5
    // Target got UpdateEpoch(e5) at 5, while source got UpdateEpoch(e6) at 10 (without having an entry for e5)  
    // Source should detect this and do a full build - (We cannot undo false progress since the process at the target until 5 could be true progress and could be checkpointed)
    if (sourceParameters.ProgressVectorData->vectors_[sharedProgressVectorEntry.SourceIndex].CurrentEpoch < targetParameters.ProgressVectorData->LastProgressVectorEntry.CurrentEpoch)
    {
        return CopyModeResult::CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason::Enum::Other);
    }

    return CopyModeResult::CreatePartialCopyResult(sharedProgressVectorEntry, sourceStartingLsn, targetStartingLsn);
}

bool ProgressVector::DecrementIndexUntilLeq(
    __inout ULONG & index,
    __in ProgressVector const & vector,
    __in ProgressVectorEntry const & comparand)
{
    do
    {
        if (vector.vectors_[index] <= comparand)
        {
            break;
        }

        // this can happen when the progress vector is trimmed and does not contain the index relative to the comparand
        if (index == 0)
        {
            return false;
        }

        --index;
    } while (true);

    return true;
}

ProgressVectorEntry ProgressVector::DecrementIndexUntilLsnIsEqual(
    __inout ULONG & index,
    __in ProgressVector const & vector,
    __in LONG64 lsn)
{
    while (index > 0)
    {
        ProgressVectorEntry previousSourceVector = vector.vectors_[index - 1];

        if (previousSourceVector.Lsn != lsn)
        {
            return previousSourceVector;
        }

        --index;
    }

    return ProgressVectorEntry();
}

SharedProgressVectorEntry ProgressVector::FindSharedVector(
    __in ProgressVector & sourceProgressVector,
    __in ProgressVector & targetProgressVector)
{
    ULONG sourceIndex = sourceProgressVector.Length - 1;
    ULONG targetIndex = targetProgressVector.Length - 1;

    LONG64 targetDataLossVersion = targetProgressVector.vectors_[targetIndex].CurrentEpoch.DataLossVersion;
    LONG64 sourceDataLossVersion = sourceProgressVector.vectors_[sourceIndex].CurrentEpoch.DataLossVersion;

    ASSERT_IFNOT(
        targetDataLossVersion <= sourceDataLossVersion,
        "{0}: targetDataLossNumber: {1} should be <= sourceDataLossNumber: {2}", 
        targetDataLossVersion,
        sourceDataLossVersion);

    SharedProgressVectorEntry progressVectorTrimmedResult = SharedProgressVectorEntry::CreateTrimmedSharedProgressVectorEntry();

    do
    {
        bool decrementIndexSuccess = DecrementIndexUntilLeq(
            targetIndex,
            targetProgressVector,
            sourceProgressVector.vectors_[sourceIndex]);

        if (!decrementIndexSuccess)
        {
            return progressVectorTrimmedResult;
        }

        decrementIndexSuccess = DecrementIndexUntilLeq(
            sourceIndex,
            sourceProgressVector,
            targetProgressVector.vectors_[targetIndex]);

        if (!decrementIndexSuccess)
        {
            return progressVectorTrimmedResult;
        }
    }
    while (sourceProgressVector.vectors_[sourceIndex] != targetProgressVector.vectors_[targetIndex]);

    ProgressVectorEntry & sourceProgressVectorEntry = sourceProgressVector.vectors_[sourceIndex];
    ProgressVectorEntry & targetProgressVectorEntry = targetProgressVector.vectors_[targetIndex];

    // WITHOUT MINI RECONFIG ENABLED, it is not possible for the target to make VALID progress
    // Note that this check is only valid in the absence of dataloss and restore.
    // E.g
    /*
    * Target Replica Copy Context: 131383889527030885
    TargetLogHeadEpoch: 0,0 TargetLogHeadLsn: 0 TargetLogTailEpoch: 131383889406148342,64424509440 TargetLogTailLSN: 73
    SourceLogHeadEpoch: 0,0 SourceLogHeadLsn: 0 SourceLogTailEpoch: 131383889406148342,68719476736 SourceLogTailLSN: 68
    Target ProgressVector: [(131383889406148342,64424509440),73,131383889527030885,2017-05-04T16:32:22]
    [(131383889406148342,55834574848),68,131383889527030885,2017-05-04T16:31:54]
    [(131383889406148342,51539607552),62,131383889416461059,2017-05-04T16:31:37]
    [(131383889406148342,47244640256),56,131383889527030885,2017-05-04T16:31:20]
    [(0,0),0,0,2017-05-04T16:29:07]
    Source ProgressVector: [(131383889406148342,68719476736),68,131383890970495929,2017-05-04T16:32:33]
    [(131383889406148342,60129542144),68,131383890970495929,2017-05-04T16:32:09]
    [(131383889406148342,51539607552),62,131383889416461059,2017-05-04T16:31:37]
    [(131383889406148342,47244640256),56,131383889527030885,2017-05-04T16:31:20]
    [(0,0),0,0,2017-05-04T16:31:33]
    LastRecoveredAtomicRedoOperationLsn: 0.


    Target Replica Copy Context: 131399770096882719
    TargetLogHeadEpoch: 0,0 TargetLogHeadLsn: 0 TargetLogTailEpoch: 131399758616683034,390842023936 TargetLogTailLSN: 374
    SourceLogHeadEpoch: 0,0 SourceLogHeadLsn: 0 SourceLogTailEpoch: 131399758616683034,395136991232 SourceLogTailLSN: 369
    Target ProgressVector:
    [(131399758616683034,390842023936),374,131399770096882719,2017-05-23T01:42:03]
    [(131399758616683034,382252089344),374,131399770096882719,2017-05-23T01:41:29]
    [(131399758616683034,377957122048),369,131399770096882719,2017-05-23T01:41:00]
    [(131399758616683034,373662154752),338,131399758632647497,2017-05-23T01:40:31]
    Source ProgressVector:
    [(131399758616683034,395136991232),369,131399758701866693,2017-05-23T01:42:19]
    [(131399758616683034,386547056640),369,131399758701866693,2017-05-23T01:41:44]
    [(131399758616683034,373662154752),338,131399758632647497,2017-05-23T01:40:31]
    LastRecoveredAtomicRedoOperationLsn: 0
    */

    std::string errorMessage;

    if ((targetProgressVector.LastProgressVectorEntry.CurrentEpoch.DataLossVersion == 
            sourceProgressVector.LastProgressVectorEntry.CurrentEpoch.DataLossVersion) &&
        (targetIndex < targetProgressVector.Length - 1) && 
        targetProgressVector.LastProgressVectorEntry.CurrentEpoch.DataLossVersion == targetProgressVector.vectors_[targetIndex].CurrentEpoch.DataLossVersion)
    {
        // Target replica could only have repeatedly attempted to become a
        // primary without ever making progress at the shared dataloss number
        // We can do "double progress check" only if there has NOT been any other data loss happening on the target after shared point. Having a differnt dataloss numbers in shared vector and target's tail
        // means that target has been a valid primary and progressed in LSN until hitting a dataloss and then a very old secondary can become primary and invalidate all the progress made by the target.
        LONG64 failureLsn = targetProgressVector.vectors_[targetIndex + 1].Lsn;
        LONG64 failureLsnIncremented = 0;

        for (ULONG n = targetIndex + 2; n <= targetProgressVector.Length - 1; n++)
        {
            ProgressVectorEntry & vector = targetProgressVector.vectors_[n];
            if (vector.CurrentEpoch.DataLossVersion != targetProgressVectorEntry.CurrentEpoch.DataLossVersion)
            {
                break;
            }

            if (vector.Lsn != failureLsn)
            {
                // Update failureLsn to ensure we don't assert if there are multiple entries for same LSN like in 2nd example above
                failureLsn = vector.Lsn;
                failureLsnIncremented++;
            }
        }

        errorMessage = Common::formatString(
            "FailureLsn incremented must be <= 1. It is {0}",
            failureLsnIncremented);

        if (!ValidateIfDebugEnabled(failureLsnIncremented <= 1, errorMessage))
        {
            return SharedProgressVectorEntry(
                sourceIndex,
                sourceProgressVectorEntry,
                targetIndex,
                targetProgressVectorEntry,
                FullCopyReason::Enum::ValidationFailed,
                errorMessage);
        }
    }

    // Sanity check that source and target progress vectors are always in 
    // agreement upto above determined ProgressVectorEntry shared by both of them
    ULONG i = sourceIndex;
    ProgressVectorEntry sourceEntry = sourceProgressVectorEntry;
    ULONG j = targetIndex;
    ProgressVectorEntry targetEntry = targetProgressVectorEntry;

    errorMessage = "sourceEntry == targetEntry";

    do
    {
        sourceEntry = DecrementIndexUntilLsnIsEqual(
            i, 
            sourceProgressVector, 
            sourceEntry.Lsn);

        if (i == 0)
        {
            break;
        }

        targetEntry = DecrementIndexUntilLsnIsEqual(
            j, 
            targetProgressVector, 
            targetEntry.Lsn);

        if (j == 0)
        {
            break;
        }

        if (!ValidateIfDebugEnabled(sourceEntry == targetEntry, errorMessage))
        {
            return SharedProgressVectorEntry(
                sourceIndex,
                sourceProgressVectorEntry,
                targetIndex,
                targetProgressVectorEntry,
                FullCopyReason::Enum::ValidationFailed,
                errorMessage);
        }
    }
    while (true);

    return SharedProgressVectorEntry(
        sourceIndex,
        sourceProgressVectorEntry,
        targetIndex,
        targetProgressVectorEntry,
        FullCopyReason::Enum::Invalid);
}

void ProgressVector::Append(__in ProgressVectorEntry const & progressVectorEntry)
{
    ASSERT_IFNOT(
        vectors_.Count() == 0 ||
        LastProgressVectorEntry.CurrentEpoch < progressVectorEntry.CurrentEpoch && 
            LastProgressVectorEntry.Lsn <= progressVectorEntry.Lsn,
        "Invalid state appending progress vector entry, count: {0}, last entry lsn: {1}, input lsn: {2}",
        vectors_.Count(), LastProgressVectorEntry.Lsn, progressVectorEntry.Lsn);

    NTSTATUS status = vectors_.Append(progressVectorEntry);
    THROW_ON_FAILURE(status);
}

bool ProgressVector::Insert(__in ProgressVectorEntry const & progressVectorEntry)
{
    for (LONG i = vectors_.Count() - 1; i >= 0; i--)
    {
        ProgressVectorEntry existingVector = vectors_[i];

        if (existingVector.CurrentEpoch == progressVectorEntry.CurrentEpoch)
        {
            ASSERT_IFNOT(existingVector.Lsn == progressVectorEntry.Lsn,
                "Existing progress vector entry lsn: {0} is not equal to lsn: {1} to be inserted if current epoch is same",
                existingVector.Lsn, progressVectorEntry.Lsn);

            return false;
        }
        else if (existingVector.CurrentEpoch < progressVectorEntry.CurrentEpoch)
        {
            vectors_.InsertAt(i + 1, progressVectorEntry);
            return true;
        }

        ASSERT_IFNOT(
            existingVector.Lsn == progressVectorEntry.Lsn,
            "Existing progress vector lsn: {0} is not equal to entry lsn: {1} to be inserted",
            existingVector.Lsn, progressVectorEntry.Lsn);
    }

    NTSTATUS status = vectors_.InsertAt(0, progressVectorEntry);
    THROW_ON_FAILURE(status);

    return true;
}

ProgressVectorEntry ProgressVector::Find(__in Epoch const & epoch) const
{
    for (LONG i = vectors_.Count() - 1; i >= 0 ; i--)
    {
        ProgressVectorEntry existingVector = vectors_[i];

        if (existingVector.CurrentEpoch == epoch)
        {
            return existingVector;
        }
    }

    return ProgressVectorEntry();
}

Epoch ProgressVector::FindEpoch(__in LONG64 lsn) const
{
    if (vectors_.Count() == 0)
    {
        return Epoch(-1, -1);
    }

    if (lsn == Constants::ZeroLsn)
    {
        return Epoch(-1, -1);
    }

    for (LONG i = vectors_.Count() - 1; i >= 0; i--)
    {
        if (vectors_[i].Lsn < lsn)
        {
            return vectors_[i].CurrentEpoch;
        }
    }

    // Currently the progress ProgressVectorEntry is never truncated, so this code should not execute.
    // Following line is for future proofing.
    return Epoch(-1, -1);
}

void ProgressVector::TruncateTail(__in ProgressVectorEntry const & lastProgressVectorEntry)
{
    ASSERT_IFNOT(vectors_.Count() > 0, "No progress vector entry during truncate tail");

    ULONG lastIndex = vectors_.Count() - 1;
    ASSERT_IFNOT(
        vectors_[lastIndex] == lastProgressVectorEntry,
        "Last entry in ProgressVector array must equal target tail entry. Last Index={0}, Target Tail={1}",
        vectors_[lastIndex].ToString(),
        lastProgressVectorEntry.ToString());

    BOOLEAN result = vectors_.Remove(lastIndex);
    ASSERT_IFNOT(
        result,
        "Failed to remove entry at index {0} from ProgressVectorEntry array",
        lastIndex);
}

void ProgressVector::Read(
    __in BinaryReader & binaryReader, 
    __in bool isPhysicalRead)
{
    ULONG32 count;
    NTSTATUS status = STATUS_SUCCESS;
    binaryReader.Read(count);

    for (ULONG i = 0; i < count; i++)
    {
        ProgressVectorEntry result;
        result.Read(binaryReader, isPhysicalRead);
        status = vectors_.Append(result);
        THROW_ON_FAILURE(status);
    }
}

void ProgressVector::Write(__in BinaryWriter & binaryWriter)
{
    binaryWriter.Write(static_cast<ULONG32>(vectors_.Count()));
    for (ULONG i = 0; i < vectors_.Count(); i++)
    {
        vectors_[i].Write(binaryWriter);
    }
}

void ProgressVector::TrimProgressVectorIfNeeded(
    __in Epoch const & highestBackedUpEpoch,
    __in Epoch const & headEpoch)
{
    if (ProgressVectorMaxEntries == 0)
    {
        return;
    }

    // do not trim the vector if the size is not exceeding the max entries threshold.
    if (vectors_.Count() <= ProgressVectorMaxEntries)
    {
        return;
    }

    Epoch trimmingEpochPoint = headEpoch > highestBackedUpEpoch ? headEpoch : highestBackedUpEpoch;

    int trimmingIndex = -1;
    for (int i = vectors_.Count() - 1; i >= 0; i--)
    {
        if (vectors_[i].CurrentEpoch < trimmingEpochPoint)
        {
            trimmingIndex = i;
            break;
        }
    }

    if (trimmingIndex == -1)
    {
        return;
    }

    vectors_.RemoveRange(0, trimmingIndex + 1);
}

bool ProgressVector::ValidateIfDebugEnabled(
    __in bool condition,
    __in std::string const & msg)
{
#ifdef DBG
    ASSERT_IFNOT(
        condition == true,
        ToStringLiteral(msg));
#else
    UNREFERENCED_PARAMETER(msg);
#endif

    return condition;
}

std::wstring ProgressVector::ToString() const
{
    return ToString(Constants::ProgressVectorMaxStringSizeInKb);
}

std::wstring ProgressVector::ToString(
    __in LONG64 maxVectorStringLengthInKb) const
{
    // Print entries from the latest value until the maximum allowed vector string length is exceeded. 
    return ToString(
        vectors_.Count() - 1,
        maxVectorStringLengthInKb);
}

std::wstring ProgressVector::ToString(
    __in ULONG targetIndex,
    __in LONG64 maxVectorStringLengthInKb) const
{
    /*
        The progress vector ToString method provides functionality to capture vector context
        based on a desired progress vector entry index when validation fails. 

        Example:
            - Progress vector PV has 1,000 entries
            - Validation failed at entry 140/1,000
            
            In this example, printing entries from the latest value (1000, highest index) 
            results in output that does NOT contain the relevant vector entries for analysis. 

            This can be avoided by performing the following steps.
            - Calculate the maximum string length for entries greater and lower than the target index
            - Determine the actual bytes in both directions
            - Allocate leftover bytes to either direction if available
            - Calculate the highest/lowest indexes based on the directional byte allocation above
            - Traverse backwards and exit the loop when our string length requirement has been exceeded
    */

    LONG64 latestProgressVectorEntryIndex;
    LONG64 earliestProgressVectorEntryIndex;

    LONG64 maxVectorStringLengthInBytes = maxVectorStringLengthInKb * 1024;

    // Size of vector entry size + 2 (size of newline characters "/r/n")
    LONG64 outputEntrySize = ProgressVectorEntry::SizeOfStringInBytes() + 2;

    // Calculate the max # of bytes before/after the target index
    LONG64 directionalMaxStringLengthInBytes = maxVectorStringLengthInBytes / 2;

    // # of bytes in entries AFTER the startingIndex
    LONG64 remainingForwardBytes = (vectors_.Count() - 1 - targetIndex) * outputEntrySize;

    // # of bytes in entries BEFORE the startingIndex
    LONG64 remainingBackwardBytes = targetIndex * outputEntrySize;

    // Ensure backward entries are not greater than allowed size
    if (remainingBackwardBytes > directionalMaxStringLengthInBytes)
    {
        remainingBackwardBytes = directionalMaxStringLengthInBytes;
    }

    // Ensure forward entries are not greater than allowed size
    if (remainingForwardBytes > directionalMaxStringLengthInBytes)
    {
        remainingForwardBytes = directionalMaxStringLengthInBytes;
    }

    // Allocate excess string length for forward entries
    if (remainingBackwardBytes < directionalMaxStringLengthInBytes)
    {
        remainingForwardBytes += (directionalMaxStringLengthInBytes - remainingBackwardBytes);
    }

    // Allocate excess string length for backward entries
    if (remainingForwardBytes < directionalMaxStringLengthInBytes)
    {
        remainingBackwardBytes += (directionalMaxStringLengthInBytes - remainingForwardBytes);
    }

    // Calculate the earliest and latest vector entry indices
    earliestProgressVectorEntryIndex = (LONG64)targetIndex - (remainingBackwardBytes / outputEntrySize);
    latestProgressVectorEntryIndex = (LONG64)targetIndex + (remainingForwardBytes / outputEntrySize);

    // Ensure indices are within bounds of vectors_
    if (earliestProgressVectorEntryIndex < 0) { earliestProgressVectorEntryIndex = 0; }
    if (latestProgressVectorEntryIndex > vectors_.Count() - 1) { latestProgressVectorEntryIndex = vectors_.Count() - 1; }

    std::wstring output;
    Common::StringWriter writer(output);

    writer.Write("\r\n");

    for (LONG64 i = latestProgressVectorEntryIndex; i >= earliestProgressVectorEntryIndex; i--)
    {
        if ((LONG64)(output.length() * sizeof(char) + ProgressVectorEntry::SizeOfStringInBytes()) >= maxVectorStringLengthInBytes)
        {
            break;
        }

        writer.WriteLine(vectors_[(ULONG)i].ToString());
    }

    writer.Flush();
    return output;
}
