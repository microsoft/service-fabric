// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Threading.Tasks;

    /// <summary>
    /// Implementations of this interface store a collection of key-value pairs along with the version of each pair.
    /// The version is used for optimistic concurrency (similar to HTTPs Etag concept).
    /// For optimization, previously read key-values could also be cached.
    /// The version is always assumed to be increasing by 1.
    /// </summary>
    /// <remarks>Cancellation tokens are not used currently. Since this is an internal interface, we can change it if needed.</remarks>
    internal interface IVersionedPropertyStore
    {
        /// <summary>
        /// Gets the value and associated metadata of the specified key. To force fetching the latest data, specify useCachedValue = false.
        /// </summary>
        Task<IVersionedKeyValue> GetValueAsync(Guid activityId, string keyName, bool useCachedValue = true);

        /// <summary>
        /// Gets the value and associated metadata of the specified key.
        /// </summary>
        /// <remarks>
        /// The additional versionKeyName parameter exists just for backward compatibility.
        /// E.g. clients like <see cref="JobBlockingPolicyManager"/> already use a custom key for storing version info.
        /// </remarks>
        Task<IVersionedKeyValue> GetValueAsync(Guid activityId, string keyName, string versionKeyName, bool useCachedValue = true);

        /// <summary>
        /// Updates the value and associated metadata of the specific key.
        /// </summary>
        Task UpdateValueAsync(Guid activityId, string keyName, string value);

        /// <summary>
        /// Updates the value and associated metadata of the specific key.
        /// </summary>
        /// <remarks>
        /// The additional versionKeyName parameter exists just for backward compatibility.
        /// E.g. clients like <see cref="JobBlockingPolicyManager"/> already use a custom key for storing version info.
        /// </remarks>
        Task UpdateValueAsync(Guid activityId, string keyName, string versionKeyName, string value);

        /// <summary>
        /// Clears the cache forcing a refresh of the cache the next time data is accessed.
        /// </summary>
        void ClearCache();
    }
}