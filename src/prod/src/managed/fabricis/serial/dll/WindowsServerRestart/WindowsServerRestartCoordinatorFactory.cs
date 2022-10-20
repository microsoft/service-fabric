// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;

    internal sealed class WindowsServerRestartCoordinatorFactory : IInfrastructureCoordinatorFactory
    {
        private readonly IConfigStore configStore;
        private readonly string configSectionName;
        private readonly Guid partitionId;
        private readonly long replicaId;

        public WindowsServerRestartCoordinatorFactory(
            IConfigStore configStore, 
            string configSectionName,
            Guid partitionId,
            long replicaId)
        {
            configStore.ThrowIfNull("configStore");
            configSectionName.ThrowIfNullOrWhiteSpace("configSectionName");                        

            this.configStore = configStore;
            this.configSectionName = configSectionName;            
            this.partitionId = partitionId;
            this.replicaId = replicaId;
        }

        public IInfrastructureCoordinator Create()
        {
            return new WindowsServerRestartCoordinator(configStore, configSectionName, partitionId, replicaId);
        }    
    }
}