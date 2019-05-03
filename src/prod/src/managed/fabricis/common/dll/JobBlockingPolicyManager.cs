// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Threading.Tasks;

    /// <summary>
    /// This class allows a 'back door entry' to adjust the job blocking policy of the infrastructure service
    /// on the fly. i.e. without a cluster manifest upgrade or an app upgrade.
    /// Job blocking policy is used by the InfrastructureCoordinator to decide which state transitions to allow.
    /// </summary>
    internal sealed class JobBlockingPolicyManager : IJobBlockingPolicyManager
    {
        /// <summary>
        /// The name of the key which stores the job blocking policy in the underlying <see cref="IVersionedPropertyStore"/>.
        /// </summary>        
        public const string PolicyPropertyName = "JobBlockingPolicy";

        /// <summary>
        /// The name of the version key which stores the job blocking policy's version in the underlying <see cref="IVersionedPropertyStore"/>.
        /// </summary>
        public const string PolicyVersionName = "PolicyVersion";

        private readonly IVersionedPropertyStore versionedPropertyStore;
        
        private JobBlockingPolicyManager(IVersionedPropertyStore versionedPropertyStore)
        {
            this.versionedPropertyStore = versionedPropertyStore;
        }

        #region public methods

        /// <summary>
        /// Creates <see cref="JobBlockingPolicyManager"/>. 
        /// Specify <see cref="oldVersionedPropertyStore"/> when IS is used in a multi-tenant cluster (E.g. RP cluster) 
        /// where each IS service communicates with one FC. In that case, we want individual job blocking policies for each IS service.
        /// If this parameter is <c>null</c>, then the same job blocking policy is shared across all IS services in a multi-tenant cluster.
        /// </summary>
        public static async Task<IJobBlockingPolicyManager> CreateAsync(
            TraceType traceType,
            IVersionedPropertyStore versionedPropertyStore,
            IVersionedPropertyStore oldVersionedPropertyStore = null)
        {
            traceType.Validate("traceType");
            versionedPropertyStore.Validate("versionedPropertyStore");

            traceType.WriteInfo("Starting creation of {0}", typeof(JobBlockingPolicyManager).Name);

            var manager = new JobBlockingPolicyManager(versionedPropertyStore);

            var activityId = Guid.NewGuid();

            var versionedKeyValue = await ReadPropertyAsync(traceType, versionedPropertyStore, activityId).ConfigureAwait(false);
            if (versionedKeyValue == null)
            {
                string defaultPolicyValue = JobBlockingPolicy.BlockNone.ToString();

                if (oldVersionedPropertyStore != null)
                {
                    var oldVersionedKeyValue = await ReadPropertyAsync(traceType, oldVersionedPropertyStore, activityId).ConfigureAwait(false);

                    if (oldVersionedKeyValue != null)
                    {
                        defaultPolicyValue = oldVersionedKeyValue.Value;
                    }

                    traceType.WriteInfo(
                        "{0} properties '{1}={2}' and '{3}' {4} newer versioned property store.",
                        oldVersionedKeyValue != null ? "Migrating" : "Creating",
                        PolicyPropertyName, defaultPolicyValue, PolicyVersionName,
                        oldVersionedKeyValue != null ? "to" : "in");
                }

                try
                {
                    await versionedPropertyStore.UpdateValueAsync(
                        activityId,
                        PolicyPropertyName,
                        PolicyVersionName,
                        defaultPolicyValue).ConfigureAwait(false);
                }
                catch (FabricException ex)
                {
                    if (ex.ErrorCode != FabricErrorCode.WriteConflict)
                    {
                        traceType.WriteError("Properties '{0}' and '{1}' could not be updated. Exception: {2}",
                            PolicyPropertyName, PolicyVersionName, ex);
                        throw;
                    }
                }
            }

            traceType.WriteInfo("Successfully created {0}", typeof(JobBlockingPolicyManager).Name);
            return manager;
        }

        public JobBlockingPolicy Policy
        {
            get
            {
                return GetPolicyAsync().GetAwaiter().GetResult();
            }
        }

        public async Task<JobBlockingPolicy> GetPolicyAsync()
        {
            var versionedKeyValue = await versionedPropertyStore.GetValueAsync(Guid.NewGuid(), PolicyPropertyName, PolicyVersionName).ConfigureAwait(false);
            var policy = (JobBlockingPolicy)Enum.Parse(typeof(JobBlockingPolicy), versionedKeyValue.Value);
            return policy;
        }

        public async Task UpdatePolicyAsync(JobBlockingPolicy newBlockingPolicy)
        {
            // no write-conflict handling here. we just throw the exception for callers to handle
            await versionedPropertyStore
                .UpdateValueAsync(Guid.NewGuid(), PolicyPropertyName, PolicyVersionName, newBlockingPolicy.ToString())
                .ConfigureAwait(false);
        }

        public void ClearCache()
        {
            versionedPropertyStore.ClearCache();
        }

        #endregion

        private static async Task<IVersionedKeyValue> ReadPropertyAsync(
            TraceType traceType,
            IVersionedPropertyStore versionedPropertyStore,
            Guid activityId)
        {
            try
            {
                var versionedKeyValue = await versionedPropertyStore
                    .GetValueAsync(activityId, PolicyPropertyName, PolicyVersionName, false)
                    .ConfigureAwait(false);

                return versionedKeyValue;
            }
            catch (FabricElementNotFoundException ex)
            {
                traceType.WriteInfo(
                    "Properties '{0}' and '{1}' not found. These need to be created. Exception: {2}",
                    PolicyPropertyName,
                    PolicyVersionName,
                    ex);
            }

            return null;
        }
    }
}