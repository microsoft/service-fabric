// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Helpers
{
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.Common.Util;

    internal static class StoreExtensions
    {
        public static async Task<IEnumerable<TOutput>> RetreiveAllObjectFromStringStoreAsync<TOutput, TKey>(
            this IPersistentStore<TKey, string> store,
            CancellationToken token)
        {
            var inMemoryState = await store.GetAllValuesAsync(token).ConfigureAwait(false);

            return inMemoryState.Select(one => InsightsUtil.DeSerialize<TOutput>(one.Value)).ToList();
        }

        public static async Task<TOutput> CreateOrRetriveRetreiveObjectFromStoreAsync<TOutput, TKey>(
            this IPersistentStore<TKey, string> store,
            TKey key,
            TOutput defaultVal,
            CancellationToken token)
        {
            await store.SetIfNotPresentAsync(key, InsightsUtil.Serialize(defaultVal), token).ConfigureAwait(false);
            var deserialized = await store.GetEntityAsync(key, token).ConfigureAwait(false);
            return InsightsUtil.DeSerialize<TOutput>(deserialized);
        }

        /// <summary>
        /// Save an Object as string
        /// </summary>
        /// <typeparam name="TKey"></typeparam>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="store"></param>
        /// <param name="key"></param>
        /// <param name="objectToBeStored"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public static Task SerializeAndSaveObjectInStoreAsync<TKey, TValue>(
            this IPersistentStore<TKey, string> store,
            TKey key,
            TValue objectToBeStored,
            CancellationToken token)
        {
            Assert.IsNotNull(key, "key != null");
            Assert.IsFalse(objectToBeStored == null, "objectToBeStored != null");

            return store.SetEntityAsync(key, InsightsUtil.Serialize(objectToBeStored), token);
        }
    }
}