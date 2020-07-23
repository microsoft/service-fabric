// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the abstract health data consumer.
    /// The HealthDataService will invoke this interface when it has health data available to process.
    /// This is an abstract class and not an interface so that it allows adding new methods in the future without breaking build.
    /// </summary>
    internal abstract class HealthDataConsumer
    {
        public abstract Task ProcessClusterHealthAsync(ClusterEntity cluster);

        public abstract Task ProcessNodeHealthAsync(NodeEntity nodeHealth);

        public abstract Task ProcessApplicationHealthAsync(ApplicationEntity application);

        public abstract Task ProcessServiceHealthAsync(ServiceEntity service);

        public abstract Task ProcessPartitionHealthAsync(PartitionEntity partition);

        public abstract Task ProcessReplicaHealthAsync(ReplicaEntity replica);

        public abstract Task ProcessDeployedApplicationHealthAsync(DeployedApplicationEntity deployedApplication);

        public abstract Task ProcessDeployedServicePackageHealthAsync(DeployedServicePackageEntity deployedServicePackage);
    }
}