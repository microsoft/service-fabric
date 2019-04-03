// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.common.Tracing.Test
{
    using System.Fabric.Common.Tracing;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides unit tests for <see cref="AgeBasedCache{TKey,TValue}"/>.
    /// </summary>
    [TestClass]
    public class AgeBasedCacheTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestCacheMaxBoundScenario()
        {
            AgeBasedCache<int, int> cache = AgeBasedCache<int, int>.DefaultInstance;

            int i = 0;
            for (; i < AgeBasedCache<int, int>.MaxCacheCount; ++i)
            {
                Assert.IsTrue(
                    cache.TryAddOrUpdate(i, i, TimeSpan.FromSeconds(AgeBasedCache<int, int>.DefaultCleanupDurationInSec + 10), null),
                    "Cache Add expected to Pass but failed");
            }

            Assert.IsTrue(cache.Count == AgeBasedCache<int, int>.MaxCacheCount, "Expected Count != Actual Count");
            Assert.IsFalse(cache.TryAddOrUpdate(i, i, TimeSpan.FromSeconds(10), null), "Cache Add expected to fail but Passed");
            cache.Clear();
            Assert.IsTrue(cache.TryAddOrUpdate(i, i, TimeSpan.FromSeconds(10), null), "Cache Add expected to Pass but Failed");

            // Cleanup
            cache.Clear();
        }

        /// <remarks>
        /// Disabling this test till a timer provider can be added in the cache.
        /// </remarks>
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestExpiredItemsSweeped()
        {
            TimeSpan sweepTimer = TimeSpan.FromMilliseconds(800);
            bool timerInactivated = false;
            AgeBasedCache<int, int> cache = AgeBasedCache<int, int>.CreateInstanceForTestingOnly(sweepTimer, TimeSpan.MaxValue, () => timerInactivated = true);
            cache.Clear();

            int i = 0;
            int cleanedUpItems = 0;
            for (; i < AgeBasedCache<int, int>.MaxCacheCount; ++i)
            {
                Assert.IsTrue(cache.TryAddOrUpdate(i, i, TimeSpan.FromMilliseconds(100), (item => cleanedUpItems++)), "Cache Add expected to Pass but failed");
            }

            // Wait for sweep to happen
            Threading.Thread.Sleep(sweepTimer + sweepTimer);

            Assert.IsTrue(cleanedUpItems == i, string.Format("Actual cleaned up Count: {0}. Expected {1}", cleanedUpItems, i));
            Assert.IsTrue(cache.Count == 0);
            Assert.IsFalse(timerInactivated, "Timer shouldn't have been inactivated");
            Assert.IsTrue(cache.TryAddOrUpdate(i, i, sweepTimer, null), "Cache Add expected to Pass but failed");
            Assert.IsTrue(cache.Count == 1);

            // Wait for sweep to happen
            Threading.Thread.Sleep(sweepTimer + sweepTimer);

            // Make sure item sweeped
            Assert.IsTrue(cache.Count == 0);
        }

        // Disabling this test for now. Using a Real timer is flaky and needs to be replaced by a mock timer.
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("vipind")]
        public void TestCacheSweepsAfterTimerInactivity()
        {
            TimeSpan sweepTimer = TimeSpan.FromMilliseconds(800);
            bool timerInactivated = false;
            AgeBasedCache<int, int> cache = AgeBasedCache<int, int>.CreateInstanceForTestingOnly(sweepTimer, sweepTimer, () => timerInactivated = true);

            // start with clean slate
            cache.Clear();

            int cleanedUpItems = 0;
            Assert.IsTrue(cache.TryAddOrUpdate(1, 1, TimeSpan.FromMilliseconds(100), (item => cleanedUpItems++)), "Cache Add expected to Pass but failed");

            // Wait for sweep to happen
            Threading.Thread.Sleep(sweepTimer + sweepTimer);

            Assert.IsTrue(cleanedUpItems == 1, string.Format("Actual cleaned up Count: {0}. Expected 1", cleanedUpItems));
            Assert.IsTrue(cache.Count == 0, "We expect 0 items in cache");
            Assert.IsTrue(timerInactivated, "Time is supposed to be inactivated");

            // Add another Item.
            Assert.IsTrue(cache.TryAddOrUpdate(2, 2, sweepTimer, null), "Cache Add expected to Pass but failed");

            // Wait for sweep to happen
            Threading.Thread.Sleep(sweepTimer + sweepTimer);

            // Make sure timer get started again and item get sweeped
            Assert.IsTrue(cache.Count == 0);
        }
    }
}