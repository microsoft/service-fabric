// ----------------------------------------------------------------------
//  <copyright file="PrimaryReplicaContextPersistentStoreWrapper.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore.Helpers;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;

    internal sealed class PrimaryReplicaContextPersistentStoreWrapper : IPrimaryReplicaContextPersistentStore
    {
        private readonly IStoreProvider storeProvider;
        private readonly CancellationToken cancellationToken;
        private readonly ILogger logger;
        private IPersistentStore<Guid, string> contextStore;

        public PrimaryReplicaContextPersistentStoreWrapper(
            ILogger logger,
            IStoreProvider storeProvider,
            CancellationToken cancellationToken)
        {
            this.storeProvider = storeProvider;
            this.cancellationToken = cancellationToken;
            this.logger = logger;
        }

        public async Task<PrimaryReplicaContext> GetPrimaryReplicaContextAsync(Guid partitionId)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            string contextAsString = await this.contextStore.GetEntityAsync(partitionId, this.cancellationToken).ConfigureAwait(false);

            var context = InsightsUtil.DeSerialize<PrimaryReplicaContext>(contextAsString);

            return context;
        }

        public async Task<bool> IsPartitionIdKnownAsync(Guid partitionId)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            return await this.contextStore.IsKeyPresentAsync(partitionId, this.cancellationToken).ConfigureAwait(false);
        }

        public async Task SavePrimaryReplicaContextAsync(PrimaryReplicaContext context)
        {
            await this.InitializeStoreAsync().ConfigureAwait(false);

            string serializedContext = InsightsUtil.Serialize(context);

            await this.contextStore.SetEntityAsync(context.PartitionId, serializedContext, this.cancellationToken).ConfigureAwait(false);
        }

        private async Task InitializeStoreAsync()
        {
            if (this.contextStore == null)
            {
                this.contextStore = await this.storeProvider.CreatePersistentStoreKeyGuidValueStringAsync(
                    PrimaryReplicaContextConstants.PrimaryReplicaContextStoreId,
                    PrimaryReplicaContextConstants.PrimaryReplicaContextPersistentStoreRetentionPolicy,
                    this.cancellationToken).ConfigureAwait(false);
            }
        }
    }
}