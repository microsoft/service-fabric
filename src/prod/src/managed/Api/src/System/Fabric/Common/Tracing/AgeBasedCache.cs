// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;

    internal sealed class AgeBasedCache<TKey, TValue> : ICache<TKey, TValue>, IDisposable where TKey : IEquatable<TKey>
    {
        /// <summary>
        /// This is the max count of items that could be there in the cache at any point in time.
        /// It's a simple protection against the unlikely bug that would cause the cache size
        /// to continue to grow. This way, if we ever reach this size, we stop accepting any adds.
        /// We came up with this number after extensive testing in LRCs where 450 was found to be at 99.8th
        /// percentile.
        /// </summary>
        public const int MaxCacheCount = 450;

        /// <summary>
        /// The cleanup routine runs every this interval to remove any expired entries.
        /// </summary>
        public const int DefaultCleanupDurationInSec = 2;

        /// <summary>
        /// This is minor optimization in cases where store is empty and there hasn't been
        /// any add/update from the user for this duration, we go ahead clean up the timer.
        /// </summary>
        public const int MaxNoActivityLifeSpanForTimerInSec = 90;

        /// <summary>
        /// Single Instance of the Cache.
        /// </summary>
        private static volatile AgeBasedCache<TKey, TValue> singleInstance;

        private static object syncRoot = new object();

        private TimeSpan noActivityLifeSpanForTimer;

        private bool disposed;

        /// <summary>
        /// Last time there was an Add or update to the cache from user.
        /// </summary>
        private DateTimeOffset lastTimeOfAddOrUpdateToCache;

        private class CacheValue<TCacheValue>
        {
            public CacheValue(TCacheValue value, DateTimeOffset expiryTime, Action<TValue> onExpireCallback)
            {
                this.AbsoluteExpiryTime = expiryTime;
                this.Value = value;
                this.OnExpireCallback = onExpireCallback;
            }

            /// <summary>
            /// The callback that's invoked when this entry expires
            /// </summary>
            public Action<TValue> OnExpireCallback { get; private set; }

            /// <summary>
            /// The expiry time.
            /// </summary>
            public DateTimeOffset AbsoluteExpiryTime { get; private set; }

            /// <summary>
            /// Actual value
            /// </summary>
            public TCacheValue Value { get; private set; }

            /// <summary>
            /// Update the value and its expiry time
            /// </summary>
            /// <param name="value"></param>
            /// <param name="expiryTime"></param>
            public void Update(TCacheValue value, DateTimeOffset expiryTime)
            {
                this.AbsoluteExpiryTime = expiryTime;
                this.Value = value;
            }
        }

        /// <summary>
        /// This is where we cache the values. Based on results from testing, it was decided to keep it simple
        /// and not maintain a separate link list of values.
        /// </summary>
        private IDictionary<TKey, CacheValue<TValue>> store;

        /// <summary>
        /// The timer that runs periodically and clears up expired values.
        /// </summary>
        private Timer clearCacheTimer;

        private ReaderWriterLockSlim rwLock;

        private TimeSpan cacheCleanDuration;

        /// <summary>
        /// Callback when timer is inactivated. *Only used for Unit Testing*
        /// </summary>
        private Action onTimerInactivated;

        private AgeBasedCache()
        {
            this.store = new Dictionary<TKey, CacheValue<TValue>>();
            this.rwLock = new ReaderWriterLockSlim();
            this.cacheCleanDuration = TimeSpan.FromSeconds(DefaultCleanupDurationInSec);
            this.noActivityLifeSpanForTimer = TimeSpan.FromSeconds(MaxNoActivityLifeSpanForTimerInSec);
            this.onTimerInactivated = null;
        }

        // Only Used for constructing cache for Unit testing.
        private AgeBasedCache(TimeSpan durationBetweenCacheCleans, TimeSpan maxTimerInactivity, Action timerInactivated)
        {
            this.store = new Dictionary<TKey, CacheValue<TValue>>();
            this.rwLock = new ReaderWriterLockSlim();
            this.cacheCleanDuration = durationBetweenCacheCleans;
            this.noActivityLifeSpanForTimer = maxTimerInactivity;
            this.onTimerInactivated = timerInactivated;
        }

        public static AgeBasedCache<TKey, TValue> DefaultInstance
        {
            get
            {
                if (singleInstance == null)
                {
                    lock (syncRoot)
                    {
                        if (singleInstance != null)
                        {
                            return singleInstance;
                        }

                        singleInstance = new AgeBasedCache<TKey, TValue>();
                    }
                }

                return singleInstance;
            }
        }

        /// <summary>
        /// Only meant for Testing so that unit test can have better control over how cache is created and how it behaves.
        /// </summary>
        /// <param name="durationBetweenCacheCleans"></param>
        /// <param name="maxTimerInactivity"></param>
        /// <param name="timerInactivated"></param>
        /// <returns></returns>
        internal static AgeBasedCache<TKey, TValue> CreateInstanceForTestingOnly(TimeSpan durationBetweenCacheCleans, TimeSpan maxTimerInactivity, Action timerInactivated)
        {
            return new AgeBasedCache<TKey, TValue>(durationBetweenCacheCleans, maxTimerInactivity, timerInactivated);
        }

        /// <inheritdoc />
        public bool TryAddOrUpdate(TKey key, TValue value, TimeSpan itemLife, Action<TValue> onExpireCallback)
        {
            this.rwLock.EnterWriteLock();

            try
            {
                // Technically we can take a write upgradeable lock, check this condition
                // and then request write lock if required. However this is unnecessary
                // since there are very few reads happening anyway on this class. So we
                // are not really unblocking anybody by taking a read lock. So sticking
                // with acquiring write lock here.
                if (this.store.Count >= MaxCacheCount)
                {
                    return false;
                }

                DateTimeOffset timeNow = DateTimeOffset.UtcNow;
                if (this.clearCacheTimer == null)
                {
                    this.clearCacheTimer = new Timer(this.RemoveExpiredValues, null, this.cacheCleanDuration, this.cacheCleanDuration);
                }

                CacheValue<TValue> existingValue;
                if (this.store.TryGetValue(key, out existingValue))
                {
                    existingValue.Update(value, timeNow + itemLife);
                }
                else
                {
                    this.store[key] = new CacheValue<TValue>(value, timeNow + itemLife, onExpireCallback);
                }

                // Note the time of last Add/Update activity.
                this.lastTimeOfAddOrUpdateToCache = timeNow;
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }

            return true;
        }

        /// <inheritdoc />
        public void RemoveIfPresent(TKey key)
        {
            this.rwLock.EnterWriteLock();

            try
            {
                this.RemoveUnlocked(key);
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }
        }

        /// <inheritdoc />
        public long Count
        {
            get
            {
                this.rwLock.EnterReadLock();

                try
                {
                    return this.store.Count;
                }
                finally
                {
                    this.rwLock.ExitReadLock();
                }
            }
        }

        /// <inheritdoc />
        public void Clear()
        {
            this.rwLock.EnterWriteLock();

            try
            {
                this.store.Clear();
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }
        }

        private void RemoveExpiredValues(object state)
        {
            this.rwLock.EnterWriteLock();

            List<CacheValue<TValue>> allCleanedUpValues = new List<CacheValue<TValue>>();
            try
            {
                DateTimeOffset currentSweepTime = DateTimeOffset.UtcNow;
                foreach (KeyValuePair<TKey, CacheValue<TValue>> s in this.store.Where(kv => currentSweepTime >= kv.Value.AbsoluteExpiryTime).ToList())
                {
                    this.RemoveUnlocked(s.Key);
                    allCleanedUpValues.Add(s.Value);
                }

                // Cleanup the Timer if required.
                if (!this.store.Any() && (currentSweepTime - this.lastTimeOfAddOrUpdateToCache) > this.noActivityLifeSpanForTimer)
                {
                    this.clearCacheTimer?.Dispose();

                    this.clearCacheTimer = null;

                    this.onTimerInactivated?.Invoke();
                }
            }
            finally
            {
                this.rwLock.ExitWriteLock();
            }

            // The callbacks are invoked outside of the lock.
            foreach (CacheValue<TValue> oneCleanedUpValue in allCleanedUpValues)
            {
                oneCleanedUpValue.OnExpireCallback?.Invoke(oneCleanedUpValue.Value);
            }
        }

        // Should be called while the write lock is held.
        private void RemoveUnlocked(TKey key)
        {
            this.store.Remove(key);
        }

        /// <summary>
        /// <para>
        /// Destructor of this cache.
        /// </para>
        /// </summary>
        ~AgeBasedCache()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (this.disposed)
            {
                return;
            }

            if (disposing)
            {
                this.clearCacheTimer?.Dispose();
                this.clearCacheTimer = null;

                this.rwLock?.Dispose();
                this.rwLock = null;
            }

            this.disposed = true;
        }
    }
}