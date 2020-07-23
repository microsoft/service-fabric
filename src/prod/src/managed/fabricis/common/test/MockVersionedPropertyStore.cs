// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Collections.Generic;
    using Threading.Tasks;

    /// <summary>
    /// Similar to <see cref="VersionedPropertyStore"/> for mock purposes.
    /// </summary>
    internal sealed class MockVersionedPropertyStore : IVersionedPropertyStore
    {
        private const string VersionKeyFormat = "{0}_Version";

        private readonly Dictionary<string, IVersionedKeyValue> cachedProperties =
            new Dictionary<string, IVersionedKeyValue>(StringComparer.OrdinalIgnoreCase);

        private readonly object locker = new object();

        public Task<IVersionedKeyValue> GetValueAsync(Guid activityId, string keyName, bool useCachedValue = true)
        {
            string versionKeyName = VersionKeyFormat.ToString(keyName);
            return GetValueAsync(activityId, keyName, versionKeyName, useCachedValue);
        }

        public Task<IVersionedKeyValue> GetValueAsync(Guid activityId, string keyName, string versionKeyName, bool useCachedValue = true)
        {
            lock (locker)
            {
                if (cachedProperties.ContainsKey(keyName))
                {
                    var vkv = cachedProperties[keyName];
                    return Task.FromResult(vkv);
                }

                throw new FabricElementNotFoundException("keyName '{0}' not found".ToString(keyName));
            }
        }

        public async Task UpdateValueAsync(Guid activityId, string keyName, string value)
        {
            keyName.Validate("keyName");
            value.Validate("value");

            string versionKeyName = VersionKeyFormat.ToString(keyName);
            await UpdateValueAsync(activityId, keyName, versionKeyName, value).ConfigureAwait(false);
        }

        public Task UpdateValueAsync(Guid activityId, string keyName, string versionKeyName, string value)
        {
            keyName.Validate("keyName");
            versionKeyName.Validate("versionKeyName");
            value.Validate("value");

            lock (locker)
            {
                VersionedKeyValue vkv;

                if (cachedProperties.ContainsKey(keyName))
                {
                    vkv = new VersionedKeyValue(
                        keyName,
                        versionKeyName,
                        value,
                        cachedProperties[keyName].Version + 1);
                }
                else
                {
                    vkv = new VersionedKeyValue(keyName, versionKeyName, value, 0);
                }

                cachedProperties[keyName] = vkv;
            }

            return Task.FromResult(0);
        }

        public void ClearCache()
        {
            lock (locker)
            {
                cachedProperties.Clear();
            }
        }
    }
}