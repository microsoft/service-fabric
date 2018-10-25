// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Text;

    [Flags]
    internal enum CopyMode : uint
    {
        Invalid = 0,

        FalseProgress = 1,

        None = 2,

        Partial = 4,

        Full = 8
    }

    internal enum FullCopyReason
    {
        Invalid,
        DataLoss,
        InsufficientLogs,
        AtomicRedoOperationFalseProgressed,
        Other,
        ProgressVectorTrimmed,
        ValidationFailed
    }

    internal class CopyContextParameters
    {
        public CopyContextParameters(ProgressVector pv, Epoch logHeadEpoch, LogicalSequenceNumber logHeadLsn, LogicalSequenceNumber logTailLsn)
        {
            this.ProgressVector = pv;
            this.LogHeadEpoch = logHeadEpoch;
            this.LogHeadLsn = logHeadLsn;
            this.LogTailLsn = logTailLsn;
        }

        public ProgressVector ProgressVector { get; private set; }

        public Epoch LogHeadEpoch { get; private set; }

        public LogicalSequenceNumber LogHeadLsn { get; private set; }

        public LogicalSequenceNumber LogTailLsn { get; private set; }
    }

    internal class SharedProgressVectorEntry
    {
        public ProgressVectorEntry SourceProgressVectorEntry { get; set; }

        public int SourceIndex { get; set; }

        public ProgressVectorEntry TargetProgressVectorEntry { get; set; }

        public int TargetIndex { get; set; }

        public FullCopyReason FullCopyReason { get; set; }

        public string FailedValidationMessage { get; set; }
    }

    internal class CopyModeResult
    {
        private CopyModeResult()
        {
            this.CopyMode = CopyMode.Invalid;
            this.FullCopyReason = FullCopyReason.Invalid;
            this.SourceStartingLsn = LogicalSequenceNumber.InvalidLsn;
            this.TargetStartingLsn = LogicalSequenceNumber.InvalidLsn;
            this.SharedProgressVectorEntry = new SharedProgressVectorEntry();
        }

        public static CopyModeResult CreateCopyNoneResult(SharedProgressVectorEntry sharedProgressVectorEntry)
        {
            return new CopyModeResult
            {
                SharedProgressVectorEntry = sharedProgressVectorEntry,
                SourceStartingLsn = LogicalSequenceNumber.InvalidLsn,
                TargetStartingLsn = LogicalSequenceNumber.InvalidLsn,
                CopyMode = CopyMode.None
            };
        }

        public static CopyModeResult CreateFullCopyResult(
            SharedProgressVectorEntry sharedProgressVectorEntry,
            FullCopyReason reason)
        {
            Utility.Assert(reason != FullCopyReason.Invalid, "reason!= FullCopyReason.Invalid");

            return new CopyModeResult
            {
                SharedProgressVectorEntry = sharedProgressVectorEntry,
                SourceStartingLsn = LogicalSequenceNumber.InvalidLsn,
                TargetStartingLsn = LogicalSequenceNumber.InvalidLsn,
                CopyMode = CopyMode.Full,
                FullCopyReason = reason
            };
        }

        public static CopyModeResult CreateFalseProgressResult(
            long lastRecoveredAtomicRedoOperationLsnAtTarget,
            SharedProgressVectorEntry sharedProgressVectorEntry,
            LogicalSequenceNumber sourceStartingLsn,
            LogicalSequenceNumber targetStartingLsn)
        {
            if (lastRecoveredAtomicRedoOperationLsnAtTarget > sourceStartingLsn.LSN)
            {
                // Atomic Redo operations cannot be undone in false progress.
                // So resort to full copy
                return CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason.AtomicRedoOperationFalseProgressed);
            }

            return new CopyModeResult
            {
                SourceStartingLsn = sourceStartingLsn,
                TargetStartingLsn = targetStartingLsn,
                CopyMode = CopyMode.FalseProgress | CopyMode.Partial,
                SharedProgressVectorEntry = sharedProgressVectorEntry
            };
        }

        public static CopyModeResult CreatePartialCopyResult(
            SharedProgressVectorEntry sharedProgressVectorEntry,
            LogicalSequenceNumber sourceStartingLsn,
            LogicalSequenceNumber targetStartingLsn)
        {
            return new CopyModeResult
            {
                SourceStartingLsn = sourceStartingLsn,
                TargetStartingLsn = targetStartingLsn,
                CopyMode = CopyMode.Partial,
                SharedProgressVectorEntry = sharedProgressVectorEntry
            };
        }

        public CopyMode CopyMode { get; set; }

        public FullCopyReason FullCopyReason { get; set; }

        public LogicalSequenceNumber SourceStartingLsn { get; set; }

        public LogicalSequenceNumber TargetStartingLsn { get; set; }

        public SharedProgressVectorEntry SharedProgressVectorEntry { get; set; }
    }

    internal class ProgressVector
    {
        private static ProgressVector zeroProgressVector = new ProgressVector(true);

        private List<ProgressVectorEntry> vectors;

        private uint progressVectorMaxEntries;

        internal ProgressVector()
        {
            this.vectors = new List<ProgressVectorEntry>();
            // progress vector max entries config is only set through the call to ProgressVector.Clone function
            this.progressVectorMaxEntries = 0;
        }

        private ProgressVector(bool isPrivate)
        {
            this.vectors = new List<ProgressVectorEntry>();
            this.vectors.Add(ProgressVectorEntry.ZeroProgressVectorEntry);
            // progress vector max entries config is only set through the call to ProgressVector.Clone function
            this.progressVectorMaxEntries = 0;
        }

        internal static ProgressVector ZeroProgressVector
        {
            get { return zeroProgressVector; }

            set { zeroProgressVector = value; }
        }

        internal void Test_SetProgressVectorMaxEntries(uint progressVectorMaxEntries)
        {
            this.progressVectorMaxEntries = progressVectorMaxEntries;
        }

        internal int ByteCount
        {
            get { return sizeof(int) + (this.vectors.Count * 40); }
        }

        internal ProgressVectorEntry LastProgressVectorEntry
        {
            get
            {
                if (this.vectors.Count > 0)
                {
                    return this.vectors[this.vectors.Count - 1];
                }

                return null;
            }
        }

        internal int Length
        {
            get { return this.vectors.Count; }
        }

        public static bool operator ==(ProgressVector left, ProgressVector right)
        {
            if (ReferenceEquals(left, right) == true)
            {
                return true;
            }

            if (ReferenceEquals(left, null) == true)
            {
                return false;
            }

            if (ReferenceEquals(right, null) == true)
            {
                return false;
            }

            if (left.vectors.Count != right.vectors.Count)
            {
                return false;
            }

            for (var i = 0; i < left.vectors.Count; i++)
            {
                if (left.vectors[i] != right.vectors[i])
                {
                    return false;
                }
            }

            return true;
        }

        public static bool operator !=(ProgressVector left, ProgressVector right)
        {
            return (left == right) == false;
        }

        public override bool Equals(object obj)
        {
            var arg = obj as ProgressVector;
            if (arg == null)
            {
                return false;
            }

            return this == arg;
        }

        public override int GetHashCode()
        {
            return this.vectors.Count;
        }

        public override string ToString()
        {
            var sb = new StringBuilder();
            this.ToString(sb, string.Empty, Constants.ProgressVectorMaxStringSizeInKb);

            return sb.ToString();
        }

        internal static bool IsBrandNewReplica(CopyContextParameters copyContextParameters)
        {
            return copyContextParameters.ProgressVector.vectors.Count == 1 &&
                   copyContextParameters.ProgressVector.vectors[0].Epoch.DataLossNumber == 0 &&
                   copyContextParameters.ProgressVector.vectors[0].Lsn == LogicalSequenceNumber.ZeroLsn &&
                   copyContextParameters.LogTailLsn == LogicalSequenceNumber.OneLsn;
        }

        internal static ProgressVector Clone(ProgressVector originalProgressVector, uint progressVectorMaxEntries, Epoch highestBackedUpEpoch, Epoch headEpoch)
        {
            //try trimming the progress vector before cloning
            originalProgressVector.TrimProgressVectorIfNeeded(highestBackedUpEpoch, headEpoch);

            var copiedProgressVector = new ProgressVector();
            copiedProgressVector.progressVectorMaxEntries = progressVectorMaxEntries;

            foreach (var vector in originalProgressVector.vectors)
            {
                copiedProgressVector.vectors.Add(vector);
            }

            return copiedProgressVector;
        }

        internal static CopyModeResult FindCopyMode(
            CopyContextParameters sourceParamaters,
            CopyContextParameters targetParameters,
            long lastRecoveredAtomicRedoOperationLsnAtTarget)
        {
            var result = FindCopyModePrivate(sourceParamaters, targetParameters, lastRecoveredAtomicRedoOperationLsnAtTarget);

            if (result.CopyMode.HasFlag(CopyMode.FalseProgress))
            {
                string failureMsg = string.Format(
                        "(sourceStartingLsn is expected to be lesser or equal to targetStartingLsn). Source starting lsn : {0}, target starting lsn :{1} ",
                        result.SourceStartingLsn.LSN,
                        result.TargetStartingLsn.LSN);

                if (!ValidateIfDebugEnabled(
                    result.SourceStartingLsn <= result.TargetStartingLsn,
                    failureMsg))
                {
                    SharedProgressVectorEntry failedValidationResult = new SharedProgressVectorEntry
                    {
                        SourceIndex = result.SharedProgressVectorEntry.SourceIndex,
                        TargetIndex = result.SharedProgressVectorEntry.TargetIndex,
                        SourceProgressVectorEntry = result.SharedProgressVectorEntry.SourceProgressVectorEntry,
                        TargetProgressVectorEntry = result.SharedProgressVectorEntry.TargetProgressVectorEntry,
                        FullCopyReason = FullCopyReason.ValidationFailed,
                        FailedValidationMessage = failureMsg
                    };

                    return CopyModeResult.CreateFullCopyResult(
                        failedValidationResult,
                        FullCopyReason.ValidationFailed);
                }
            }

            return result;
        }

        private static CopyModeResult FindCopyModePrivate(
            CopyContextParameters sourceParamaters,
            CopyContextParameters targetParameters,
            long lastRecoveredAtomicRedoOperationLsnAtTarget)
        {
            // We can perform the check for CASE 1(Shown Below) before finding the shared ProgressVectorEntry.
            // However, we do not do that because the shared ProgressVectorEntry method provides additional checks of epoch history
            var sharedProgressVectorEntry = FindSharedVector(
                sourceParamaters.ProgressVector,
                targetParameters.ProgressVector);

            // If we detect that the shared progress vector cannot be obtained because of a trimmed progress vector, we decide to do a full copy
            if (sharedProgressVectorEntry.FullCopyReason == FullCopyReason.ProgressVectorTrimmed)
            {
                return CopyModeResult.CreateFullCopyResult(null, FullCopyReason.ProgressVectorTrimmed);
            }

            // Validation failed, perform full copy
            if (sharedProgressVectorEntry.FullCopyReason == FullCopyReason.ValidationFailed)
            {
                return CopyModeResult.CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason.ValidationFailed);
            }

            // *******************************************CASE 1 - COPY_NONE***********************************************************
            //
            // The last entry in the source and the target vectors are same with the same tail.
            // This can happen if target goes down and comes back up and the primary made no progress
            if (sourceParamaters.ProgressVector.LastProgressVectorEntry == targetParameters.ProgressVector.LastProgressVectorEntry &&
                sourceParamaters.LogTailLsn == targetParameters.LogTailLsn)
            {
                // Note that copy none covers only cases where nothing needs to be copied (including the UpdateEpochLogRecord)
                // Hence, service creation scenario is Partial copy, not None.
                return CopyModeResult.CreateCopyNoneResult(sharedProgressVectorEntry);
            }

            // *******************************************CASE 2a - COPY_FULL - Due to Data Loss******************************************
            // 
            // Check for any dataloss in the source or target log
            // If there was data loss, perform full copy
            // No full copy with DataLoss reason, if the target is a brand new replica
            if (!ProgressVector.IsBrandNewReplica(targetParameters))
            {
                if (sharedProgressVectorEntry.SourceProgressVectorEntry.Epoch.DataLossNumber != sourceParamaters.ProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber ||
                    sharedProgressVectorEntry.TargetProgressVectorEntry.Epoch.DataLossNumber != targetParameters.ProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber ||
                    sourceParamaters.LogHeadEpoch.DataLossNumber > sharedProgressVectorEntry.TargetProgressVectorEntry.Epoch.DataLossNumber ||
                    targetParameters.LogHeadEpoch.DataLossNumber > sharedProgressVectorEntry.SourceProgressVectorEntry.Epoch.DataLossNumber)
                {
                    return CopyModeResult.CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason.DataLoss);
                }
            }

            var sourceStartingLsn = (sharedProgressVectorEntry.SourceIndex == (sourceParamaters.ProgressVector.vectors.Count - 1))
                ? sourceParamaters.LogTailLsn
                : sourceParamaters.ProgressVector.vectors[sharedProgressVectorEntry.SourceIndex + 1].Lsn;

            var targetStartingLsn = (sharedProgressVectorEntry.TargetIndex == (targetParameters.ProgressVector.vectors.Count - 1))
                ? targetParameters.LogTailLsn
                : targetParameters.ProgressVector.vectors[sharedProgressVectorEntry.TargetIndex + 1].Lsn;

            // *******************************************CASE 2b - COPY_FULL - Due to Insufficient Loss******************************************
            //
            // Since there was no data loss, check if we can perform partial copy and there are sufficient logs in the source 
            // and target to do so
            if (sourceParamaters.LogHeadLsn > targetParameters.LogTailLsn ||
                sourceStartingLsn < sourceParamaters.LogHeadLsn ||
                targetParameters.LogHeadLsn > sourceStartingLsn)
            {
                return CopyModeResult.CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason.InsufficientLogs);
            }

            // *******************************************CASE 3a - FALSE_PROGRESS -Basic Case******************************************
            //
            // The simple case where target made more progress in an epoch than the primary
            //          Source = (1,1,0) (1,3,10) (1,4,15) Tail = 16
            //          Target = (1,1,0) (1,3,10) Tail = 20
            if (sourceStartingLsn < targetStartingLsn)
            {
                return CopyModeResult.CreateFalseProgressResult(
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

            //          Source = (1,3,10) (1,5,17) Tail = 18
            //          Target = (1,3,10) (1,4,15) Tail = 17

            if (targetStartingLsn != targetParameters.LogTailLsn)
            {
                sourceStartingLsn = targetStartingLsn;

                return CopyModeResult.CreateFalseProgressResult(
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
            if (sourceParamaters.ProgressVector.vectors[sharedProgressVectorEntry.SourceIndex].Epoch < targetParameters.ProgressVector.LastProgressVectorEntry.Epoch)
            {
                return CopyModeResult.CreateFullCopyResult(sharedProgressVectorEntry, FullCopyReason.Other);
            }

            return CopyModeResult.CreatePartialCopyResult(sharedProgressVectorEntry, sourceStartingLsn, targetStartingLsn);
        }

        private static bool DecrementIndexUntilLeq(ref int index, ProgressVector vector, ProgressVectorEntry comparand)
        {
            do
            {
                if (vector.vectors[index] <= comparand)
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

        private static ProgressVectorEntry DecrementIndexUntilLsnIsEqual(ref int index, ProgressVector vector, long lsn)
        {
            while (index > 0)
            {
                var previousSourceVector = vector.vectors[index - 1];
                if (previousSourceVector.Lsn.LSN != lsn)
                {
                    return previousSourceVector;
                }

                --index;
            }

            return null;
        }

        private static bool ValidateIfDebugEnabled(bool condition, string message)
        {
#if DEBUG
            Utility.Assert(condition == true, message);
#endif
            return condition;
        }

        internal static SharedProgressVectorEntry FindSharedVector(
            ProgressVector sourceProgressVector,
            ProgressVector targetProgressVector)
        {
            // Initialize indices
            var sourceIndex = sourceProgressVector.vectors.Count - 1;
            var targetIndex = targetProgressVector.vectors.Count - 1;

            Utility.Assert(
                targetProgressVector.vectors[targetIndex].Epoch.DataLossNumber <= sourceProgressVector.vectors[sourceIndex].Epoch.DataLossNumber,
                "targetDataLossNumber ({0}) <= sourceDataLossNumber ({1})",
                targetProgressVector.vectors[targetIndex].Epoch.DataLossNumber, sourceProgressVector.vectors[sourceIndex].Epoch.DataLossNumber);

            SharedProgressVectorEntry progrssVectorTrimmedResult = new SharedProgressVectorEntry()
            {
                FullCopyReason = FullCopyReason.ProgressVectorTrimmed
            };

            do
            {
                bool decrementIndexSuccess = DecrementIndexUntilLeq(
                    ref targetIndex,
                    targetProgressVector,
                    sourceProgressVector.vectors[sourceIndex]);

                if (!decrementIndexSuccess)
                {
                    return progrssVectorTrimmedResult;
                }

                decrementIndexSuccess = DecrementIndexUntilLeq(
                    ref sourceIndex,
                    sourceProgressVector,
                    targetProgressVector.vectors[targetIndex]);

                if (!decrementIndexSuccess)
                {
                    return progrssVectorTrimmedResult;
                }

            } while (sourceProgressVector.vectors[sourceIndex] != targetProgressVector.vectors[targetIndex]);

            var sourceProgressVectorEntry = sourceProgressVector.vectors[sourceIndex];
            var targetProgressVectorEntry = targetProgressVector.vectors[targetIndex];

            // WITHOUT MINI RECONFIG ENABLED, it is not possible for the target to make VALID progress TWICE from the shared epoch
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

            string failureMsg;

            if (targetProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber
                == sourceProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber
                && targetIndex < targetProgressVector.vectors.Count - 1 &&
                targetProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber == targetProgressVector.vectors[targetIndex].Epoch.DataLossNumber)
            {
                // Target replica could only have repeatedly attempted to become a
                // primary without ever making progress at the shared dataloss number
                // We can do "double progress check" only if there has NOT been any other data loss happening on the target after shared point. Having a differnt dataloss numbers in shared vector and target's tail
                // means that target has been a valid primary and progressed in LSN until hitting a dataloss and then a very old secondary can become primary and invalidate all the progress made by the target.
                var failureLsn = targetProgressVector.vectors[targetIndex + 1].Lsn;
                var failureLsnIncremented = 0;

                for (var n = targetIndex + 2; n <= targetProgressVector.vectors.Count - 1; n++)
                {
                    var vector = targetProgressVector.vectors[n];
                    if (vector.Epoch.DataLossNumber != targetProgressVectorEntry.Epoch.DataLossNumber)
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

                failureMsg = string.Format(
                        "FailureLsn incremented must be <= 1. It is {0}",
                        failureLsnIncremented);

                if (!ValidateIfDebugEnabled(
                    failureLsnIncremented <= 1,
                    failureMsg))
                {
                    return new SharedProgressVectorEntry
                    {
                        SourceIndex = sourceIndex,
                        TargetIndex = targetIndex,
                        SourceProgressVectorEntry = sourceProgressVectorEntry,
                        TargetProgressVectorEntry = targetProgressVectorEntry,
                        FullCopyReason = FullCopyReason.ValidationFailed,
                        FailedValidationMessage = failureMsg
                    };
                }
            }

            // Sanity check that source and target progress vectors are always in 
            // agreement upto above determined ProgressVectorEntry shared by both of them
            var i = sourceIndex;
            var sourceEntry = sourceProgressVectorEntry;
            var j = targetIndex;
            var targetEntry = targetProgressVectorEntry;
            failureMsg = "sourceEntry == targetEntry";

            do
            {
                sourceEntry = DecrementIndexUntilLsnIsEqual(ref i, sourceProgressVector, sourceEntry.Lsn.LSN);
                if (i == 0)
                {
                    break;
                }

                targetEntry = DecrementIndexUntilLsnIsEqual(ref j, targetProgressVector, targetEntry.Lsn.LSN);
                if (j == 0)
                {
                    break;
                }

                if (!ValidateIfDebugEnabled(
                    sourceEntry == targetEntry,
                    failureMsg))
                {
                    return new SharedProgressVectorEntry
                    {
                        SourceIndex = sourceIndex,
                        TargetIndex = targetIndex,
                        SourceProgressVectorEntry = sourceProgressVectorEntry,
                        TargetProgressVectorEntry = targetProgressVectorEntry,
                        FullCopyReason = FullCopyReason.ValidationFailed,
                        FailedValidationMessage = failureMsg
                    };
                }

            } while (true);

            Utility.Assert(sourceProgressVectorEntry != null && targetProgressVectorEntry != null, "Source and target expected to be non null");

            return new SharedProgressVectorEntry
            {
                SourceIndex = sourceIndex,
                TargetIndex = targetIndex,
                SourceProgressVectorEntry = sourceProgressVectorEntry,
                TargetProgressVectorEntry = targetProgressVectorEntry
            };
        }

        internal void Add(ProgressVectorEntry progressVectorEntry)
        {
            var lastVector = this.LastProgressVectorEntry;
            Utility.Assert(
                (lastVector == null) || ((lastVector.Epoch < progressVectorEntry.Epoch) && (lastVector.Lsn <= progressVectorEntry.Lsn)),
                "(LastProgressVectorEntry == null) || ((LastProgressVectorEntry.Epoch < ProgressVectorEntry.Epoch) && (LastProgressVectorEntry.Lsn <= ProgressVectorEntry.Lsn)) IsLastVectorNull :{0}",
                lastVector == null);

            this.vectors.Add(progressVectorEntry);
            return;
        }

        internal ProgressVectorEntry Find(Epoch epoch)
        {
            for (var i = this.vectors.Count - 1; i >= 0; i--)
            {
                var existingVector = this.vectors[i];
                if (existingVector.Epoch == epoch)
                {
                    return existingVector;
                }
            }

            return null;
        }

        internal Epoch FindEpoch(LogicalSequenceNumber lsn)
        {
            if (this.vectors.Count == 0)
            {
                return new Epoch(-1, -1);
            }

            if (lsn.LSN == 0)
            {
                return new Epoch(-1, -1);
            }

            for (var i = this.vectors.Count - 1; i >= 0; i--)
            {
                if (this.vectors[i].Lsn < lsn)
                {
                    return this.vectors[i].Epoch;
                }
            }

            // Currently the progress ProgressVectorEntry is never truncated, so this code should not execute.
            // Following line is for future proofing.
            return new Epoch(-1, -1);
        }

        internal bool Insert(ProgressVectorEntry progressVectorEntry)
        {
            for (var i = this.vectors.Count - 1; i >= 0; i--)
            {
                var existingVector = this.vectors[i];
                if (existingVector.Epoch == progressVectorEntry.Epoch)
                {
                    Utility.Assert(existingVector.Lsn == progressVectorEntry.Lsn, "existingVector.Lsn == ProgressVectorEntry.Lsn");
                    return false;
                }
                else if (existingVector.Epoch < progressVectorEntry.Epoch)
                {
                    this.vectors.Insert(i + 1, progressVectorEntry);
                    return true;
                }

                Utility.Assert(existingVector.Lsn == progressVectorEntry.Lsn, "existingVector.Lsn == ProgressVectorEntry.Lsn");
            }

            // We should never reach here
            this.vectors.Insert(0, progressVectorEntry);
            return true;
        }

        internal void Read(BinaryReader br, bool isPhysicalRead)
        {
            var count = br.ReadInt32();
            for (var i = 0; i < count; i++)
            {
                var vector = new ProgressVectorEntry();
                vector.Read(br, isPhysicalRead);
                this.vectors.Add(vector);
            }

            return;
        }

        internal void ToString(StringBuilder sb, string prefix, int maxVectorStringLengthInKb)
        {
            this.ToString(sb, prefix, maxVectorStringLengthInKb, this.vectors.Count - 1);
        }

        /// <summary>
        /// The progress vector ToString method provides functionality to capture vector context
        /// based on a desired progress vector entry index when validation fails.
        ///
        ///  Example:
        ///    - Progress vector PV has 1,000 entries
        ///    - Validation failed at entry 140/1,000
        ///
        ///     In this example, printing entries from the latest value (1000, highest index) 
        ///     results in output that does NOT contain the relevant vector entries for analysis. 
        ///
        ///     This can be avoided by performing the following steps.
        ///    - Calculate the maximum string length for entries greater and lower than the target index
        ///    - Determine the actual bytes in both directions
        ///    - Allocate leftover bytes to either direction if available
        ///    - Calculate the highest/lowest indexes based on the directional byte allocation above
        ///    - Traverse backwards and exit the loop when our string length requirement has been exceeded
        /// </summary>
        /// <param name="sb"></param>
        /// <param name="prefix"></param>
        /// <param name="maxVectorStringLengthInKb"></param>
        /// <param name="targetIndex"></param>
        internal void ToString(StringBuilder sb, string prefix, int maxVectorStringLengthInKb, int targetIndex)
        {
            int latestProgressVectorEntryIndex;
            int earliestProgressVectorEntryIndex;
            var maxVectorStringLengthInBytes = maxVectorStringLengthInKb * 1024;

            // Size of prefix + vector entry size + 2 (size of newline characters "/r/n")
            var outputEntrySize = ProgressVectorEntry.SizeOfStringInBytes + prefix.Length + 2;

            // Calculate the max # of bytes before/after the target index
            var directionalMaxStringLengthInBytes = maxVectorStringLengthInBytes / 2;

            // # of bytes in entries AFTER the startingIndex
            var remainingForwardBytes = (this.vectors.Count - 1 - targetIndex) * outputEntrySize;

            // # of bytes in entries BEFORE the startingIndex
            var remainingBackwardBytes = targetIndex * outputEntrySize;

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
            earliestProgressVectorEntryIndex = targetIndex - (remainingBackwardBytes / outputEntrySize);
            latestProgressVectorEntryIndex = targetIndex + (remainingForwardBytes / outputEntrySize);

            // Ensure indices are within bounds of vectors
            if (earliestProgressVectorEntryIndex < 0) { earliestProgressVectorEntryIndex = 0; }
            if (latestProgressVectorEntryIndex > this.vectors.Count - 1) { latestProgressVectorEntryIndex = this.vectors.Count - 1; }

            var isFirstPass = true;
            var startIndex = sb.Length;

            for (var i = latestProgressVectorEntryIndex; i >= earliestProgressVectorEntryIndex; i--)
            {
                if (((sb.Length - startIndex) * sizeof(char) + ProgressVectorEntry.SizeOfStringInBytes) >= maxVectorStringLengthInBytes)
                {
                    break;
                }

                if (isFirstPass == false)
                {
                    sb.AppendLine();
                    sb.Append(prefix);
                }
                else
                {
                    isFirstPass = false;
                }

                this.vectors[i].ToString(sb);
            }
        }

        internal string ToString(int maxVectorStringLengthInKb)
        {
            var sb = new StringBuilder();
            this.ToString(sb, Constants.ProgressVectorTracePrefix, maxVectorStringLengthInKb);
            return sb.ToString();
        }

        internal string ToString(int targetIndex, int maxVectorStringLengthInKb)
        {
            var sb = new StringBuilder();
            this.ToString(sb, Constants.ProgressVectorTracePrefix, maxVectorStringLengthInKb, targetIndex);
            return sb.ToString();
        }

        internal void TruncateHead(ProgressVectorEntry firstProgressVectorEntry)
        {
            for (var i = 0; i < this.vectors.Count; i++)
            {
                if (this.vectors[i].Epoch == firstProgressVectorEntry.Epoch)
                {
                    this.vectors[i] = firstProgressVectorEntry;
                    if (i > 0)
                    {
                        this.vectors.RemoveRange(0, i);
                    }

                    return;
                }
            }

            return;
        }

        internal void TruncateTail(ProgressVectorEntry lastProgressVectorEntry)
        {
            var lastIndex = this.vectors.Count - 1;
            Utility.Assert(this.vectors[lastIndex] == lastProgressVectorEntry, "this.vectors[lastIndex] == LastProgressVectorEntry");
            this.vectors.RemoveAt(lastIndex);

            return;
        }

        internal void Write(BinaryWriter bw)
        {
            this.Write(bw, this.vectors.Count);

            return;
        }

        internal void Write(BinaryWriter bw, int progressVectorLength)
        {
            bw.Write(progressVectorLength);
            for (var i = 0; i < progressVectorLength; i++)
            {
                this.vectors[i].Write(bw);
            }

            return;
        }

        internal void TrimProgressVectorIfNeeded(Epoch highestBackedUpEpoch, Epoch headEpoch)
        {
            if (this.progressVectorMaxEntries == 0)
            {
                return;
            }

            // do not trim the vector if the size is not exceeding the max entries threshold.
            if (vectors.Count <= progressVectorMaxEntries)
            {
                return;
            }

            Epoch trimmingEpochPoint = headEpoch > highestBackedUpEpoch ? headEpoch : highestBackedUpEpoch;

            int trimmingIndex = this.vectors.FindLastIndex(pve => pve.Epoch < trimmingEpochPoint);

            if (trimmingIndex == -1)
            {
                return;
            }

            this.vectors.RemoveRange(0, trimmingIndex + 1);
        }

        // progress vector entries are stutter of each other if they represent a same data loss and LSN number and they don't have any other entry between them.
        // configuration numbers could be (and MUST be) different as we have no repeating progress vector entries in the list.
        // caller must make sure that p1 and p2 are indeed neighbors of each other in the progress vector list and only after that confirmation, call this function for stutter check.
        private bool EntriesStutter(ProgressVectorEntry p1, ProgressVectorEntry p2)
        {
            return p1.Epoch.DataLossNumber == p2.Epoch.DataLossNumber && p1.Lsn.LSN == p2.Lsn.LSN;
        }
    }

    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "related classes")]
    internal class ProgressVectorEntry
    {
        private static long invalidReplicaId = -1;

        private static long universalReplicaId = 0;

        private static int sizeOfStringInBytes = -1;

        private static ProgressVectorEntry zeroProgressVectorEntry = new ProgressVectorEntry(
        LogicalSequenceNumber.ZeroEpoch,
        LogicalSequenceNumber.ZeroLsn,
        UniversalReplicaId,
        DateTime.UtcNow);

        private Epoch epoch;

        private LogicalSequenceNumber lsn;

        private long primaryReplicaId;

        private DateTime timestamp;

        internal ProgressVectorEntry()
        : this(
            LogicalSequenceNumber.InvalidEpoch,
            LogicalSequenceNumber.InvalidLsn,
            InvalidReplicaId,
            DateTime.MinValue)
        {
        }

        internal ProgressVectorEntry(UpdateEpochLogRecord record)
        : this(record.Epoch, record.Lsn, record.PrimaryReplicaId, record.Timestamp)
        {
        }

        internal ProgressVectorEntry(Epoch epoch, LogicalSequenceNumber lsn, long primaryReplicaId, DateTime timestamp)
        {
            this.epoch = epoch;
            this.lsn = lsn;
            this.primaryReplicaId = primaryReplicaId;
            this.timestamp = timestamp;
        }

        internal static long InvalidReplicaId
        {
            get { return invalidReplicaId; }

            set { invalidReplicaId = value; }
        }

        internal static long UniversalReplicaId
        {
            get { return universalReplicaId; }

            set { universalReplicaId = value; }
        }

        internal static ProgressVectorEntry ZeroProgressVectorEntry
        {
            get { return zeroProgressVectorEntry; }

            set { zeroProgressVectorEntry = value; }
        }

        internal static int SizeOfStringInBytes
        {
            get
            {
                if (sizeOfStringInBytes == -1)
                {
                    var sb = new StringBuilder();
                    ZeroProgressVectorEntry.ToString(sb);
                    sizeOfStringInBytes = sb.ToString().Length * sizeof(char);
                }

                return sizeOfStringInBytes;
            }
        }

        internal Epoch Epoch
        {
            get { return this.epoch; }
        }

        internal LogicalSequenceNumber Lsn
        {
            get { return this.lsn; }
        }

        internal long PrimaryReplicaId
        {
            get { return this.primaryReplicaId; }
        }

        internal DateTime Timestamp
        {
            get { return this.timestamp; }
        }

        public static bool operator ==(ProgressVectorEntry left, ProgressVectorEntry right)
        {
            if (ReferenceEquals(left, right) == true)
            {
                return true;
            }

            if (ReferenceEquals(left, null) == true)
            {
                return false;
            }

            if (ReferenceEquals(right, null) == true)
            {
                return false;
            }

            if (left.epoch == right.epoch)
            {
                return left.lsn == right.lsn;
            }

            return false;
        }

        public static bool operator >(ProgressVectorEntry left, ProgressVectorEntry right)
        {
            if (left.epoch.DataLossNumber > right.epoch.DataLossNumber)
            {
                return true;
            }

            if (left.epoch.DataLossNumber < right.epoch.DataLossNumber)
            {
                return false;
            }

            if (left.epoch.ConfigurationNumber > right.epoch.ConfigurationNumber)
            {
                return true;
            }

            if (left.epoch.ConfigurationNumber < right.epoch.ConfigurationNumber)
            {
                return false;
            }

            return left.lsn > right.lsn;
        }

        public static bool operator >=(ProgressVectorEntry left, ProgressVectorEntry right)
        {
            return (left < right) == false;
        }

        public static bool operator !=(ProgressVectorEntry left, ProgressVectorEntry right)
        {
            return (left == right) == false;
        }

        public static bool operator <(ProgressVectorEntry left, ProgressVectorEntry right)
        {
            if (left.epoch.DataLossNumber < right.epoch.DataLossNumber)
            {
                return true;
            }

            if (left.epoch.DataLossNumber > right.epoch.DataLossNumber)
            {
                return false;
            }

            if (left.epoch.ConfigurationNumber < right.epoch.ConfigurationNumber)
            {
                return true;
            }

            if (left.epoch.ConfigurationNumber > right.epoch.ConfigurationNumber)
            {
                return false;
            }

            return left.lsn < right.lsn;
        }

        public static bool operator <=(ProgressVectorEntry left, ProgressVectorEntry right)
        {
            return (left > right) == false;
        }

        public override bool Equals(object obj)
        {
            var arg = obj as ProgressVectorEntry;
            if (arg == null)
            {
                return false;
            }

            return this == arg;
        }

        public override int GetHashCode()
        {
            return this.lsn.GetHashCode();
        }

        public override string ToString()
        {
            var sb = new StringBuilder();
            this.ToString(sb);

            return sb.ToString();
        }

        public bool IsDataLossBetween(ProgressVectorEntry other)
        {
            return this.Epoch.DataLossNumber != other.Epoch.DataLossNumber;
        }

        internal void Read(BinaryReader br, bool isPhysicalRead)
        {
            var dataLossNumber = br.ReadInt64();
            var configurationNumber = br.ReadInt64();
            this.epoch = new Epoch(dataLossNumber, configurationNumber);
            this.lsn = new LogicalSequenceNumber(br.ReadInt64());
            this.primaryReplicaId = br.ReadInt64();
            this.timestamp = DateTime.FromBinary(br.ReadInt64());

            return;
        }

        internal void ToString(StringBuilder sb)
        {
            sb.Append("\t[(");
            sb.Append(this.epoch.DataLossNumber);
            sb.Append(',');
            sb.Append(this.epoch.ConfigurationNumber);
            sb.Append("),");
            sb.Append(this.lsn.LSN);
            sb.Append(',');
            sb.Append(this.primaryReplicaId);
            sb.Append(',');
            sb.Append(
                this.timestamp.ToString(
                    System.Globalization.CultureInfo.InvariantCulture.DateTimeFormat.SortableDateTimePattern));
            sb.Append("]");

            return;
        }

        internal void Write(BinaryWriter bw)
        {
            bw.Write(this.epoch.DataLossNumber);
            bw.Write(this.epoch.ConfigurationNumber);
            bw.Write(this.lsn.LSN);
            bw.Write(this.primaryReplicaId);
            bw.Write(this.timestamp.ToBinary());

            return;
        }
    }
}