// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Collections.Generic;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading.Tasks;

    [TestClass]
    public class VersionedPropertyStoreTest
    {
        private static readonly TraceType traceType = new TraceType(typeof(VersionedPropertyStoreTest).Name);

        private static readonly Uri defaultStoreName = new Uri("Fabric:/__FabricSystem_App4294967295/test");

        /// <summary>
        /// We need this to set custom values of max attempts and delta back off for the unit tests to finish quickly.
        /// </summary>
        private readonly IRetryPolicyFactory retryPolicyFactory = new LinearRetryPolicyFactory(
            traceType,
            500,
            3,
            AzureHelper.IsRetriableException);

        private int workflow1Counter;

        private int workflow2Counter;

        private int workflow3Counter;

        [TestMethod]
        public async Task GetValueAsyncTest1Async()
        {
            var propertyManager = new MockPropertyManagerWrapper
            {
                SubmitPropertyBatchAsyncFunc = Workflow1
            };

            Guid activityId = Guid.NewGuid();

            IVersionedPropertyStore versionedPropertyStore = await VersionedPropertyStore.CreateAsync(
                activityId, traceType, defaultStoreName, propertyManager, retryPolicyFactory).ConfigureAwait(false);

            IVersionedKeyValue vkv = await versionedPropertyStore.GetValueAsync(activityId, "A").ConfigureAwait(false);
            VerifyVersionedKeyValue(vkv, "Apple", 2, 1, workflow1Counter);

            vkv = await versionedPropertyStore.GetValueAsync(activityId, "A").ConfigureAwait(false);
            // Verifying that remote store is not hit now and data is fetched from cache
            VerifyVersionedKeyValue(vkv, "Apple", 2, 1, workflow1Counter);
        }

        [TestMethod]
        public async Task UpdateValueAsyncTest1Async()
        {
            var propertyManager = new MockPropertyManagerWrapper
            {
                SubmitPropertyBatchAsyncFunc = Workflow2
            };

            Guid activityId = Guid.NewGuid();

            IVersionedPropertyStore versionedPropertyStore = await VersionedPropertyStore.CreateAsync(
                activityId, traceType, defaultStoreName, propertyManager, retryPolicyFactory).ConfigureAwait(false);

            await versionedPropertyStore.UpdateValueAsync(activityId, "A", "Apple").ConfigureAwait(false);
            IVersionedKeyValue vkv = await versionedPropertyStore.GetValueAsync(activityId, "A");
            // Cache is updated correctly after UpdateValueAsync so hitting remote store is not needed
            VerifyVersionedKeyValue(vkv, "Apple", 0, 1, workflow2Counter);

            // now, get from the remote store. (workflow2 is modified to send appropriate return values)
            vkv = await versionedPropertyStore.GetValueAsync(activityId, "A", false);
            VerifyVersionedKeyValue(vkv, "Antelope", 25, 2, workflow2Counter);

            vkv = await versionedPropertyStore.GetValueAsync(activityId, "A");
            // verify cache is updated correctly
            VerifyVersionedKeyValue(vkv, "Antelope", 25, 2, workflow2Counter);
        }

        [TestMethod]
        public async Task ClearCacheTest1Async()
        {
            var propertyManager = new MockPropertyManagerWrapper
            {
                SubmitPropertyBatchAsyncFunc = Workflow3
            };

            Guid activityId = Guid.NewGuid();

            IVersionedPropertyStore versionedPropertyStore = await VersionedPropertyStore.CreateAsync(
                activityId, traceType, defaultStoreName, propertyManager, retryPolicyFactory).ConfigureAwait(false);

            IVersionedKeyValue vkv = await versionedPropertyStore.GetValueAsync(activityId, "A").ConfigureAwait(false);
            VerifyVersionedKeyValue(vkv, "Apple", 2, 1, workflow3Counter);

            IVersionedKeyValue vkv2 = await versionedPropertyStore.GetValueAsync(activityId, "A").ConfigureAwait(false);
            // verify that the counter hasn't been incremented since the actual property manager hasn't been invoked
            // but the value has been obtained from the cache
            VerifyVersionedKeyValue(vkv2, "Apple", 2, 1, workflow3Counter);

            versionedPropertyStore.ClearCache();

            IVersionedKeyValue vkv3 = await versionedPropertyStore.GetValueAsync(activityId, "A").ConfigureAwait(false);
            // verify that the counter has been incremented since the actual property manager has been invoked
            // since the value has been cleared from the cache
            VerifyVersionedKeyValue(vkv3, "Apple", 2, 2, workflow3Counter);

            Assert.AreEqual(workflow3Counter, 2);
        }

        private static void VerifyVersionedKeyValue(IVersionedKeyValue vkv, string value, long version, int expectedCounterValue, int actualCounterValue)
        {
            Assert.IsNotNull(vkv);
            Assert.AreEqual(vkv.Value, value);
            Assert.AreEqual(vkv.Version, version);
            Assert.AreEqual(expectedCounterValue, actualCounterValue);
        }

        private MockPropertyBatchResultWrapper Workflow1(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper
            {
                FailedOperationIndex = -1,
            };

            result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)2 });
            result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = "Apple" });

            traceType.ConsoleWriteLine("Setting FailedOperationIndex = -1 to simulate success from SubmitPropertyBatchAsync for a Get");

            workflow1Counter++;

            return result;
        }

        private MockPropertyBatchResultWrapper Workflow2(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper
            {
                FailedOperationIndex = -1,
            };

            if (workflow2Counter == 0)
            {
                // there are 6 operations that are batched up inside UpdatePropertiesAsync
                // If we succeed, we get the values of indices 2 and 4 to update the cache.
                // Simulating this behavior
                result.NamedProperties.Add(new MockNamedPropertyWrapper());
                result.NamedProperties.Add(new MockNamedPropertyWrapper());
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)0 });
                result.NamedProperties.Add(new MockNamedPropertyWrapper());
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = "Apple" });
                result.NamedProperties.Add(new MockNamedPropertyWrapper());

                traceType.ConsoleWriteLine("Setting FailedOperationIndex = -1 to simulate success from SubmitPropertyBatchAsync for an Update");
            }
            else
            {
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)25 });
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = "Antelope" });

                traceType.ConsoleWriteLine("Setting FailedOperationIndex = -1 to simulate success from SubmitPropertyBatchAsync for a Get");
            }

            workflow2Counter++;

            return result;
        }

        private MockPropertyBatchResultWrapper Workflow3(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper
            {
                FailedOperationIndex = -1,
            };

            result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)2 });
            result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = "Apple" });

            traceType.ConsoleWriteLine("Setting FailedOperationIndex = -1 to simulate success from SubmitPropertyBatchAsync for a Get");

            workflow3Counter++;

            return result;
        }
    }
}