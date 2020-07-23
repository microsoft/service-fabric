// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    using System.Collections.Concurrent;
    using System.Fabric.ReliableMessaging;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    /*
    * SingularSyncPoint is a simple synchronization mechanism is designed to be compatible with the TPL, i.e., with the use of awaitable methods
    * This class is used to protect a unique (not keyed) critical section and does not support timeouts
    * 
    * The usage pattern for a critical section for the given key type is
    *       SingularSyncPoint syncPoint = new SingularSyncPoint(syncContextString) // declared in context for critical section
    *       .....
    *       syncPoint.EnterAsync();
    *       critical section work
    *       syncPoint.Leave();
    *       .....
    * 
    * In case of work that may throw exceptions use a try/finally pattern
    */

    internal class SingularSyncPoint
    {
        private TaskCompletionSource<object> syncEvent;
        private object syncEventLock = new object();
        private string syncContext;

        internal SingularSyncPoint(string syncContext)
        {
            this.syncContext = syncContext;
            this.syncEvent = null;
        }

        internal async Task EnterAsync()
        {
            var entered = false;
            var mySyncEvent = new TaskCompletionSource<object>();

            while (!entered)
            {
                TaskCompletionSource<object> currentEvent;

                lock (this.syncEventLock)
                {
                    if (this.syncEvent == null)
                    {
                        this.syncEvent = mySyncEvent;
                        currentEvent = mySyncEvent;
                    }
                    else
                    {
                        currentEvent = this.syncEvent;
                    }
                }

                if (currentEvent == mySyncEvent)
                {
                    entered = true;
                }
                else
                {
                    await currentEvent.Task;
                }
            }
        }

        internal void Leave()
        {
            TaskCompletionSource<object> mySyncEvent;

            lock (this.syncEventLock)
            {
                // Guard against thread hijacking by SetResult
                mySyncEvent = this.syncEvent;
                this.syncEvent = null;
            }

            mySyncEvent.SetResult(null);
        }
    }

    /*
     * The SyncPoint synchronization mechanism is designed to be compatible with the TPL, i.e., with the use of awaitable methods
     * The TKey type parameter must implement ToString for tracing purposes
     * This class is used with a ConcurrentDictionary (syncPointCollection) of SyncPoints with a key of type TKey being locked for some context: 
     * the key is typically a Guid or a Uri; each syncPoint has an Id that is used in tracing its lifecycle
     * The syncPoint keeps a streamManager context for tracing as well as to be sensitive to role changes
     * 
     * The usage pattern for a critical section for the given key type is
     * using (SyncPoint<TKey> syncPoint = new SyncPoint<TKey>(streamManager, key, syncPointCollection))
     * {
     *       syncPoint.EnterAsync();
     *       critical section work
     * }
     * 
     * In case of work that may throw exceptions use a try/finally pattern with explicit Dispose();
     */

    internal class SyncPoint<TKey> : IDisposable
    {
        private readonly TaskCompletionSource<bool> syncEvent;
        private readonly StreamManager streamManager;
        private readonly Guid era;
        private readonly TKey key;
        private bool exitedAfterAcquiring;
        private readonly string traceType;
        private readonly long id;

        // inserts and deletes in syncPointCollections are only done within this class -- thus if we have a reference to such a collection, then it is safe
        // and even necessary to complete the SyncPoint lifecycle even when the syncPointCollection has lost its relevance due to a replica role transition
        private ConcurrentDictionary<TKey, SyncPoint<TKey>> syncPointCollection;

        private static long idgen = -1;

        internal SyncPoint(StreamManager streamManager, TKey key, ConcurrentDictionary<TKey, SyncPoint<TKey>> syncPointCollection)
        {
            this.syncEvent = new TaskCompletionSource<bool>();
            this.key = key;
            this.syncPointCollection = syncPointCollection;
            this.exitedAfterAcquiring = false;
            this.traceType = streamManager.TraceType;
            this.streamManager = streamManager;
            this.era = streamManager.Era;
            this.id = Interlocked.Increment(ref idgen);

            // we reset StreamManager level syncPointCollections to null when transitioning Primary -> NotPrimary and there may be a race
            if (syncPointCollection == null || this.era == Guid.Empty)
            {
                /*
                if (syncPointCollection == null)
                {
                    Tracing.WriteNoise("SyncPoint.Ctor", "{0} syncPointCollection is null", this.traceType);
                }

                Tracing.WriteNoise("SyncPoint.Ctor", "{0} Era = {1}", this.traceType, this.era);
                 */

                // we seem to have made a transition to not primary
                throw new FabricNotPrimaryException();
            }
        }

        public void Dispose()
        {
            var exitTrace = this.exitedAfterAcquiring ? "ExitNormal@" : "ExitTimeout@";

            FabricEvents.Events.SyncPointWait(exitTrace + this.id + "@" + this.traceType, this.key.ToString(), "0");

            if (this.exitedAfterAcquiring)
            {
                SyncPoint<TKey> removedSyncPoint = null;
                var removeSuccess = this.syncPointCollection.TryRemove(this.key, out removedSyncPoint);

                Diagnostics.Assert(removeSuccess, "Syncpoint@" + this.id + "@" + this.key + " Critical section syncpoint missing in SyncPoint.Dispose");
                Diagnostics.Assert(
                    removedSyncPoint == this,
                    "Syncpoint@" + this.id + "@" + this.key + "::" + removedSyncPoint.key + " mismatch in SyncPoint.Dispose");
            }

            this.syncEvent.SetResult(this.exitedAfterAcquiring);
        }

        internal async Task EnterAsync(TimeSpan timeout)
        {
            var entered = false;
            var remainingTimeout = timeout;
            FabricEvents.Events.SyncPointWait(
                "Enter@" + this.id + "@" + this.traceType,
                this.key.ToString(),
                ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));

            try
            {
                while (!entered)
                {
                    if (this.era != this.streamManager.Era)
                    {
                        // we could sit here spinning for a while and not notice that the replica role changed because we are not doing anything that requires replica state
                        // so double check that the stream manager did not go through a role change, and if that happened throw an exception
                        throw new FabricObjectClosedException();
                    }

                    var actualSyncPoint = this.syncPointCollection.GetOrAdd(this.key, this);
                    if (actualSyncPoint == this)
                    {
                        this.exitedAfterAcquiring = true;
                        entered = true;
                    }
                    else
                    {
                        if (timeout != Timeout.InfiniteTimeSpan)
                        {
                            try
                            {
                                var waitStart = DateTime.UtcNow;
                                FabricEvents.Events.SyncPointWait(
                                    "Start@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                    this.key.ToString(),
                                    ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));
                                var finished = await TimeoutHandler.WaitWithDelay(remainingTimeout, actualSyncPoint.syncEvent.Task);

                                if (!finished)
                                {
                                    FabricEvents.Events.SyncPointWait(
                                        "Timeout@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                        this.key.ToString(),
                                        ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));
                                    throw new TimeoutException();
                                }

                                FabricEvents.Events.SyncPointWait(
                                    "Finish@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                    this.key.ToString(),
                                    ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));
                                // go around and try entering again
                                var now = DateTime.UtcNow;

                                // this could throw a TimeoutException
                                remainingTimeout = TimeoutHandler.UpdateTimeout(remainingTimeout, waitStart, now);
                            }
                            catch (Exception e)
                            {
                                if (e is TimeoutException)
                                {
                                    FabricEvents.Events.SyncPointWait(
                                        "Timeout@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                        this.key.ToString(),
                                        ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));
                                    throw;
                                }

                                Diagnostics.ProcessExceptionAsError(
                                    "EnterAsync.TimeoutHandler.WaitWithDelay@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                    e,
                                    "Unexpected exception in EnterAsync RemainingTimeout: {0}",
                                    remainingTimeout);
                            }
                        }
                        else
                        {
                            FabricEvents.Events.SyncPointWait(
                                "Start@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                this.key.ToString(),
                                ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));
                            await actualSyncPoint.syncEvent.Task;
                            FabricEvents.Events.SyncPointWait(
                                "Finish@" + this.id + "::" + actualSyncPoint.id + "@" + this.traceType,
                                this.key.ToString(),
                                ((int) remainingTimeout.TotalMilliseconds).ToString(CultureInfo.InvariantCulture));
                        }
                    }
                }
            }
            catch (Exception e)
            {
                if (e is TimeoutException || e is FabricObjectClosedException || e is FabricNotPrimaryException)
                {
                    this.exitedAfterAcquiring = false;
                }
                else
                {
                    Diagnostics.Assert(false, "{0}@{1} Unexpected exception {2} in SyncPoint.EnterAsync", this.traceType, this.id, e);
                }

                throw;
            }
        }
    }
}