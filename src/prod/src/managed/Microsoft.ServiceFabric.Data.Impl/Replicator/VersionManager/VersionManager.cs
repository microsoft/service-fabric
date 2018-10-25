// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class VersionManager : IVersionManager, IInternalVersionManager
    {
        private readonly Dictionary<long, TaskCompletionSource<long>> readerRemovalNotifications;

        private readonly ConcurrentDictionary<NotificationKey, byte> registeredNotifications;

        private readonly List<long> versionList;

        private readonly ReaderWriterLockSlim versionListLock;

        private readonly IVersionProvider versionProvider;

        private CompletionTask lastDispatchingBarrier;

        /// <summary>
        /// Initializes a new instance of the VersionManager class.
        /// </summary>
        /// <param name="versionProvider">The version provider (LoggingReplicator)</param>
        internal VersionManager(IVersionProvider versionProvider)
        {
            this.versionListLock = new ReaderWriterLockSlim();
            this.versionProvider = versionProvider;
            this.readerRemovalNotifications = new Dictionary<long, TaskCompletionSource<long>>();
            this.registeredNotifications = new ConcurrentDictionary<NotificationKey, byte>();

            this.versionList = new List<long>();
            this.lastDispatchingBarrier = null;
        }

        public Task TryRemoveCheckpointAsync(long checkpointLsnToBeRemoved, long nextcheckpointLsn)
        {
            ISet<Task> result = null;

            Utility.Assert(
                nextcheckpointLsn > (checkpointLsnToBeRemoved + 1),
                "Checkpoint is preceded by Barrier Record. Hence numbers should at least differ by 2.");

            this.versionListLock.EnterReadLock();
            try
            {
                int indexVisibilitySequenceHigherThanVersionToBeRemoved;

                var isRemovable = this.CanVersionBeRemovedCallerHoldsLock(
                    checkpointLsnToBeRemoved,
                    nextcheckpointLsn,
                    out indexVisibilitySequenceHigherThanVersionToBeRemoved);

                if (true == isRemovable)
                {
                    return null;
                }

                result = new HashSet<Task>();

                var versionsKeepingItemAlive =
                    this.FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
                        indexVisibilitySequenceHigherThanVersionToBeRemoved,
                        nextcheckpointLsn);

                foreach (var visibility in versionsKeepingItemAlive)
                {
                    TaskCompletionSource<long> tcs;
                    this.readerRemovalNotifications.TryGetValue(visibility, out tcs);

                    Utility.Assert(null != tcs, "Reader removal notification must already exist");

                    result.Add(tcs.Task);
                }
            }
            finally
            {
                this.versionListLock.ExitReadLock();
            }

            return Task.WhenAll(result);
        }

        public TryRemoveVersionResult TryRemoveVersion(
            long stateProviderId,
            long commitSequenceNumber,
            long nextCommitSequenceNumber)
        {
            Utility.Assert(
                nextCommitSequenceNumber > commitSequenceNumber,
                "commit sequence number must be less than next commit sequence number.");

            var result = new TryRemoveVersionResult();

            this.versionListLock.EnterReadLock();
            try
            {
                int indexVisibilitySequenceHigherThanVersionToBeRemoved;

                var isRemovable = this.CanVersionBeRemovedCallerHoldsLock(
                    commitSequenceNumber,
                    nextCommitSequenceNumber,
                    out indexVisibilitySequenceHigherThanVersionToBeRemoved);

                result.CanBeRemoved = isRemovable;

                if (true == isRemovable)
                {
                    return result;
                }

                var versionsKeepingItemAlive =
                    this.FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
                        indexVisibilitySequenceHigherThanVersionToBeRemoved,
                        nextCommitSequenceNumber);

                result.EnumerationSet = versionsKeepingItemAlive;

                foreach (var visibility in result.EnumerationSet)
                {
                    var key = new NotificationKey(stateProviderId, visibility);
                    if (true == this.registeredNotifications.TryAdd(key, byte.MaxValue))
                    {
                        if (result.EnumerationCompletionNotifications == null)
                        {
                            result.EnumerationCompletionNotifications = new HashSet<EnumerationCompletionResult>();
                        }

                        TaskCompletionSource<long> tcs;
                        this.readerRemovalNotifications.TryGetValue(visibility, out tcs);

                        Utility.Assert(null != tcs, "Reader removal notification must already exist");

                        tcs.Task.IgnoreException().ContinueWith(
                            task =>
                            {
                                byte outputByte;
                                var isRemoved = this.registeredNotifications.TryRemove(key, out outputByte);

                                Utility.Assert(
                                    isRemoved == true,
                                    "only one can register and only one can unregister.");
                                Utility.Assert(outputByte == byte.MaxValue, "byte is always set to MaxValue");
                            }).IgnoreExceptionVoid();

                        result.EnumerationCompletionNotifications.Add(
                            new EnumerationCompletionResult(visibility, tcs.Task));
                    }
                }
            }
            finally
            {
                this.versionListLock.ExitReadLock();
            }

            return result;
        }

        public void UpdateDispatchingBarrierTask(CompletionTask barrierTask)
        {
            this.lastDispatchingBarrier = barrierTask;
        }

        /// <summary>
        /// Transaction is registered as a snapshot transaction.
        /// </summary>
        /// <returns>Visibility sequence number.</returns>
        /// <returns>
        /// A snapshot transaction must register with version manager before doing any reads.
        /// </returns>
        async Task<long> ITransactionVersionManager.RegisterAsync()
        {
            long visibilityVersionNumber;

            var snapLastDispatchingBarrier = this.lastDispatchingBarrier;

            if (snapLastDispatchingBarrier != null)
            {
                await this.lastDispatchingBarrier.AwaitCompletion().ConfigureAwait(false);
            }

            this.versionListLock.EnterWriteLock();
            try
            {
                visibilityVersionNumber = this.versionProvider.GetVersion();

                var currentSnapshotCount = this.versionList.Count;
                Utility.Assert(
                    currentSnapshotCount == 0 || visibilityVersionNumber >= this.versionList[currentSnapshotCount - 1],
                    "Barriers must come in sorted order.");

                this.versionList.Add(visibilityVersionNumber);
                var contains = this.readerRemovalNotifications.ContainsKey(visibilityVersionNumber);

                if (false == contains)
                {
                    this.readerRemovalNotifications.Add(visibilityVersionNumber, new TaskCompletionSource<long>());
                }
            }
            finally
            {
                this.versionListLock.ExitWriteLock();
            }

            Utility.Assert(
                visibilityVersionNumber != default(long),
                "Visibility version number {0} must not be default.",
                visibilityVersionNumber);

            return visibilityVersionNumber;
        }

        /// <summary>
        /// Unregister the given snapshot transaction.
        /// </summary>
        /// <param name="visibilityVersionNumber">Visibility sequence number that is being unregistered.</param>
        /// <remarks>
        /// A snapshot transaction must unregister itself once it knowns it will do no more reads (Dispose).
        /// </remarks>
        void ITransactionVersionManager.UnRegister(long visibilityVersionNumber)
        {
            TaskCompletionSource<long> notification = null;

            this.versionListLock.EnterWriteLock();
            try
            {
                var index = this.versionList.BinarySearch(visibilityVersionNumber);

                Utility.Assert(index >= 0, "An item that is not registered cannot be unregistered");

                var leftSideMatch = index > 0 && this.versionList[index - 1] == visibilityVersionNumber;
                var rightSideMatch = index < this.versionList.Count - 1
                                     && this.versionList[index + 1] == visibilityVersionNumber;

                if (leftSideMatch == false && rightSideMatch == false)
                {
                    notification = this.readerRemovalNotifications[visibilityVersionNumber];
                    this.readerRemovalNotifications.Remove(visibilityVersionNumber);
                }

                this.versionList.RemoveAt(index);
            }
            finally
            {
                this.versionListLock.ExitWriteLock();
            }

            // Making sure that Tasks continuation runs outside the lock.
            if (notification != null)
            {
                notification.SetResult(visibilityVersionNumber);
            }
        }

        internal bool TestMethod_VerifyState(int versionCount, int notificationCount, int registeredNotificationsCount)
        {
            return this.versionList.Count == versionCount && this.readerRemovalNotifications.Count == notificationCount
                   && this.registeredNotifications.Count == registeredNotificationsCount;
        }

        private bool CanVersionBeRemovedCallerHoldsLock(
            long versionLsnToBeRemoved,
            long newPreviousLastCommittedSequenceNumber,
            out int indexOfEqualOrImmediatelyHigher)
        {
            indexOfEqualOrImmediatelyHigher = int.MinValue;

            // List maintains _size
            if (this.versionList.Count == 0)
            {
                // It is removable.
                return true;
            }

            var index = this.versionList.BinarySearch(versionLsnToBeRemoved);

            // version list could have perfect match due to isBarrier records like InformationLogRecord 
            // that is not preceded by a Barrier log record.
            indexOfEqualOrImmediatelyHigher = (index < 0) ? ~index : index;

            Utility.Assert(
                indexOfEqualOrImmediatelyHigher <= this.versionList.Count,
                "larger index must be at max count");

            // if all versions are higher than versionLsnToBeRemoved
            if (indexOfEqualOrImmediatelyHigher == this.versionList.Count)
            {
                // It is removable.
                return true;
            }

            if (newPreviousLastCommittedSequenceNumber <= this.versionList[indexOfEqualOrImmediatelyHigher])
            {
                // It is removable.
                return true;
            }

            return false;
        }

        private ISet<long> FindAllVisibilitySequenceNumbersKeepingVersionAliveCallerHoldsLock(
            int startIndex,
            long newPreviousLastCommittedSequenceNumber)
        {
            var result = new HashSet<long>();

            var versionCount = this.versionList.Count;

            for (var index = startIndex; index < versionCount; index++)
            {
                var visibilitySequence = this.versionList[index];

                if (visibilitySequence > newPreviousLastCommittedSequenceNumber)
                {
                    break;
                }

                result.Add(visibilitySequence);
            }

            return result;
        }
    }
}