// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Store
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// StructuredStore Provider
    /// </summary>
    public interface IStoreProvider
    {
        /// <summary>
        /// Get persistent store for <see cref="string"/>
        /// </summary>
        /// <param name="storeIdentifier"></param>
        /// <param name="retentionPolicy"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IPersistentStore<Guid, string>> CreatePersistentStoreKeyGuidValueStringAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token);

        /// <summary>
        /// Get persistent store for <see cref="DateTime"/>
        /// </summary>
        /// <param name="storeIdentifier"></param>
        /// <param name="retentionPolicy"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IPersistentStore<Guid, DateTime>> CreatePersistentStoreKeyGuidValueDateTimeAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token);

        /// <summary>
        /// Get persistent store for <see cref="string"/>
        /// </summary>
        /// <param name="storeIdentifier"></param>
        /// <param name="retentionPolicy"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IPersistentStore<string, string>> CreatePersistentStoreForStringsAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token);

        /// <summary>
        /// Get persistent store for <see cref="DateTime"/>
        /// </summary>
        /// <param name="storeIdentifier"></param>
        /// <param name="retentionPolicy"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IPersistentStore<string, DateTime>> CreatePersistentStoreForTimeAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token);

        /// <summary>
        /// Get persistent store for <see cref="int"/>
        /// </summary>
        /// <param name="storeIdentifier"></param>
        /// <param name="retentionPolicy"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IPersistentStore<string, int>> CreatePersistentStoreForIntAsync(
            string storeIdentifier,
            DataRetentionPolicy retentionPolicy,
            CancellationToken token);
    }
}