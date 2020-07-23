// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Globalization;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// A store used primarily as a facade over the Naming Store.
    /// <see cref="IVersionedPropertyStore"/>.
    /// This store also caches the previously read key-value property to minimize trips to the Naming Store.
    /// This entry in the cache is refreshed after an update.
    /// </summary>
    internal sealed class VersionedPropertyStore : IVersionedPropertyStore
    {
        /// <summary>
        /// The format to be used for storing the version key in cases where it is not provided.
        /// E.g. if key is: JobBlockingPolicy, then the key which tracks the version of this is: JobBlockingPolicy_Version
        /// </summary>
        private const string VersionKeyFormat = "{0}_Version";

        private readonly TraceType traceType;

        private readonly IPropertyManagerWrapper propertyManager;

        private readonly IRetryPolicyFactory retryPolicyFactory;

        private readonly Uri storeName;

        /// <summary>
        /// Stores the cached properties. These are refreshed 
        /// a. whenever an update happens (UpdateValueAsync)
        /// b. read is called with an option to not use cached value (GetValueAsync with useCachedValue = false)
        /// </summary>
        private readonly Dictionary<string, IVersionedKeyValue> cachedProperties = 
            new Dictionary<string, IVersionedKeyValue>(StringComparer.OrdinalIgnoreCase);

        private readonly object locker = new object();

        private VersionedPropertyStore(
            TraceType traceType,
            Uri storeName, 
            IPropertyManagerWrapper propertyManager, 
            IRetryPolicyFactory retryPolicyFactory)
        {
            this.traceType = traceType;
            this.storeName = storeName;
            this.propertyManager = propertyManager;
            this.retryPolicyFactory = retryPolicyFactory;
        }

        #region public

        public static async Task<IVersionedPropertyStore> CreateAsync(
            Guid activityId,
            TraceType traceType,
            Uri storeName,
            IPropertyManagerWrapper propertyManager,
            IRetryPolicyFactory retryPolicyFactory)
        {
            traceType.Validate("traceType");
            storeName.Validate("storeName");
            propertyManager.Validate("propertyManager");
            retryPolicyFactory.Validate("retryPolicyFactory");

            var store = new VersionedPropertyStore(traceType, storeName, propertyManager, retryPolicyFactory);

            await store.InitializeStoreNameAsync(activityId).ConfigureAwait(false);

            return store;
        }

        public Task<IVersionedKeyValue> GetValueAsync(Guid activityId, string keyName, bool useCachedValue = true)
        {
            keyName.Validate("keyName");

            string versionKeyName = VersionKeyFormat.ToString(keyName);
            return GetValueAsync(activityId, keyName, versionKeyName, useCachedValue);
        }

        public async Task<IVersionedKeyValue> GetValueAsync(Guid activityId, string keyName, string versionKeyName, bool useCachedValue = true)
        {
            keyName.Validate("keyName");
            versionKeyName.Validate("versionKeyName");

            if (useCachedValue)
            {
                lock (locker)
                {
                    if (cachedProperties.ContainsKey(keyName))
                    {
                        var vkv = cachedProperties[keyName];
                        return vkv;
                    }
                }
            }

            traceType.WriteInfo(
                "Getting value from remote store{0}. keyName: {1}, versionKeyName: {2}",
                useCachedValue ? " since it doesn't exist in cache" : string.Empty,
                keyName,
                versionKeyName);

            var versionedKeyValue = await ReadPropertiesFromRemoteStoreAsync(activityId, keyName, versionKeyName).ConfigureAwait(false);

            lock (locker)
            {
                versionedKeyValue = UpdateCache(activityId, versionedKeyValue);
            }

            return versionedKeyValue;
        }

        public async Task UpdateValueAsync(Guid activityId, string keyName, string value)
        {
            keyName.Validate("keyName");
            value.Validate("value");

            string versionKeyName = VersionKeyFormat.ToString(keyName);
            await UpdateValueAsync(activityId, keyName, versionKeyName, value).ConfigureAwait(false);
        }

        public async Task UpdateValueAsync(Guid activityId, string keyName, string versionKeyName, string value)
        {
            keyName.Validate("keyName");
            versionKeyName.Validate("versionKeyName");
            value.Validate("value");

            traceType.WriteInfo(
                "Starting VersionedPropertyStore.UpdateValueAsync for store name '{0}'. Key: {1}, VersionKey: {2}, Value: {3}, ActivityId: {4}",
                storeName, keyName, versionKeyName, value, activityId);

            Int64 oldVersion, newVersion;

            lock (locker)
            {
                oldVersion = cachedProperties.ContainsKey(keyName) ? cachedProperties[keyName].Version : -1;
                newVersion = oldVersion + 1;
            }

            // Publish the new values to the naming service, including retrieving the new values
            // to update the local cache
            var propertyOperations = new List<PropertyBatchOperation>
            {
                // operation index
                new CheckValuePropertyOperation(versionKeyName, oldVersion), // 0
                new PutPropertyOperation(versionKeyName, newVersion),        // 1
                new GetPropertyOperation(versionKeyName, true),              // 2
                new PutPropertyOperation(keyName, value),                    // 3
                new GetPropertyOperation(keyName, true)                      // 4
            };

            try
            {
                IPropertyBatchResultWrapper result = await retryPolicyFactory.Create().ExecuteAsync(
                    async token => await propertyManager.SubmitPropertyBatchAsync(
                        storeName,
                        propertyOperations).ConfigureAwait(false),
                    "SubmitPropertyBatchAsync",
                    CancellationToken.None).ConfigureAwait(false);

                traceType.WriteInfo(
                    "SubmitPropertyBatchAsync returned result.FailedOperationIndex: {0}, result.FailedOperationException: {1}, ActivityId: {2}",
                    result.FailedOperationIndex,
                    result.FailedOperationException != null ? result.FailedOperationException.ToString() : "<null>",
                    activityId);

                switch (result.FailedOperationIndex)
                {
                    case -1:
                        lock (locker)
                        {
                            // Note; these indexes are tied to the batch operation array above. If you adjust
                            // the set of operations, need to update these offsets accordingly
                            Int64 version = result.GetProperty(2).GetValue<Int64>();
                            string newValue = result.GetProperty(4).GetValue<string>();

                            var updatedVkv = UpdateCache(activityId, new VersionedKeyValue(keyName, versionKeyName, newValue, version));

                            string message =
                                "UpdateValueAsync successful for store name '{0}'. New cached value: {1}, ActivityId: {2}"
                                    .ToString(storeName, updatedVkv, activityId);

                            traceType.WriteInfo(message);
                        }
                        break;

                    case 0:
                    {
                        // optimistic concurrency check failed (someone else has updated the version in the remote store)
                        string message =
                            ("UpdateValueAsync policy failed for store name '{0}' due to version mismatch. This is because of a possible write conflict. Please retry. " +
                             "Key: {1}, VersionKey: {2}, ActivityId: {3}").ToString(
                                 storeName,
                                 keyName,
                                 versionKeyName,
                                 activityId);
                        traceType.WriteInfo(message);

                        // get the latest version so that the cache is updated
                        await GetValueAsync(activityId, keyName, versionKeyName, false).ConfigureAwait(false);

                        throw new FabricException(message, FabricErrorCode.WriteConflict);
                    }

                    default:
                    {
                        string message =
                            "UpdateValueAsync policy failed for store name '{0}'. " +
                            "Key: {1}, VersionKey: {2}, ActivityId: {3}".ToString(
                                storeName,
                                keyName,
                                versionKeyName,
                                activityId);
                        var ex = result.FailedOperationException ?? new FabricException(message, FabricErrorCode.Unknown);
                        throw ex;
                    }
                }
            }
            catch (Exception ex)
            {
                string message =
                    "Error while updating key for store name '{0}'. Key: {1}, VersionKey: {2}, Value: {3}, ActivityId: {4}, Error: {5}"
                        .ToString(storeName, keyName, versionKeyName, value, activityId, ex);
                traceType.WriteInfo(message);
                throw;
            }
        }

        public void ClearCache()
        {
            lock (locker)
            {
                cachedProperties.Clear();                
            }
        }

        #endregion public

        #region private methods

        /// <summary>
        /// Creates the store name if it doesn't exist.
        /// </summary>
        private async Task InitializeStoreNameAsync(Guid activityId)
        {
            try
            {
                await retryPolicyFactory.Create().ExecuteAsync(
                    async token =>
                    {
                        bool nameExists = await propertyManager.NameExistsAsync(storeName).ConfigureAwait(false);

                        if (nameExists)
                        {
                            traceType.WriteInfo(
                                "Store name '{0}' already exists. ActivityId: {1}",
                                storeName, activityId);
                            return true;
                        }

                        await propertyManager.CreateNameAsync(storeName).ConfigureAwait(false);
                        traceType.WriteInfo("Successfully created store name '{0}'. ActivityId: {1}", storeName, activityId);

                        return true;
                    },
                    "InitializeStoreNameAsync",
                    CancellationToken.None).ConfigureAwait(false);
            }
            catch (FabricElementAlreadyExistsException)
            {
                traceType.WriteInfo("Store name '{0}' already exists. ActivityId: {1}", storeName, activityId);
            }
            catch (AggregateException ae)
            {
                ae.Flatten().Handle(e => e is FabricElementAlreadyExistsException);
                traceType.WriteInfo("Store name '{0}' already exists. ActivityId: {1}", storeName, activityId);
            }
        }

        private async Task<IVersionedKeyValue>  ReadPropertiesFromRemoteStoreAsync(Guid activityId, string keyName, string versionKeyName)
        {
            traceType.WriteInfo("Starting ReadPropertiesFromRemoteStoreAsync for store name '{0}'. ActivityId: {1}", storeName, activityId);

            var propertyOperations = new List<PropertyBatchOperation>
            {
                                                                // operation index
                new GetPropertyOperation(versionKeyName, true), // 0
                new GetPropertyOperation(keyName, true),        // 1
            };

            try
            {
                IPropertyBatchResultWrapper result = await retryPolicyFactory.Create().ExecuteAsync(
                    async token =>
                        await propertyManager.SubmitPropertyBatchAsync(
                            storeName,
                            propertyOperations).ConfigureAwait(false),
                    "SubmitPropertyBatchAsync",
                    CancellationToken.None).ConfigureAwait(false);

                traceType.WriteInfo(
                    "SubmitPropertyBatchAsync returned result.FailedOperationIndex: {0}, result.FailedOperationException: {1}, ActivityId: {2}",
                    result.FailedOperationIndex,
                    result.FailedOperationException != null ? result.FailedOperationException.ToString() : "<null>",
                    activityId);

                if (result.FailedOperationIndex == -1)
                {
                    traceType.WriteInfo(
                        "ReadPropertiesFromRemoteStoreAsync succeeded for store name '{0}'. Key: {1}, Version Key: {2}, ActivityId: {3}",
                        storeName,
                        keyName,
                        versionKeyName,
                        activityId);

                    // operation succeeded
                    var version = result.GetProperty(0).GetValue<Int64>();
                    var value = result.GetProperty(1).GetValue<string>();

                    return new VersionedKeyValue(keyName, versionKeyName, value, version);
                }

                string message = string.Format(
                    CultureInfo.InvariantCulture,
                    "ReadPropertiesFromRemoteStoreAsync failed for store name '{0}'. Key: {1}, VersionKey: {2}, ActivityId: {3}, Error: {4}",
                    storeName,
                    keyName,
                    versionKeyName,
                    activityId,
                    result.FailedOperationException);

                throw result.FailedOperationException ?? new FabricException(message, FabricErrorCode.Unknown);
            }
            catch (Exception ex)
            {
                string message =
                    "Error while reading key for store name '{0}'. Key: {1}, VersionKey: {2}, ActivityId: {3}, Error: {4}"
                        .ToString(storeName, keyName, versionKeyName, activityId, ex);
                traceType.WriteInfo(message);
                throw;
            }                       
        }

        private IVersionedKeyValue UpdateCache(Guid activityId, IVersionedKeyValue vkv)
        {
            lock (locker)
            {
                if (cachedProperties.ContainsKey(vkv.Key))
                {
                    var existingVkv = cachedProperties[vkv.Key];

                    if (existingVkv.Version >= vkv.Version)
                    {
                        traceType.WriteWarning(
                            "UpdateCache ignored for store name '{0}'. Current local cache data is newer than data to be updated. Current local cache data: {1}{2}Data to be updated: {3}{4}ActivityId: {5}",
                            storeName,
                            existingVkv,
                            Environment.NewLine,
                            vkv,
                            Environment.NewLine,
                            activityId);

                        return existingVkv;
                    }
                }

                cachedProperties[vkv.Key] = vkv;
                return vkv;
            }    
        }

        #endregion private methods
    }
}