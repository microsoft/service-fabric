// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Linq;
    using System.Text;

    using System.Fabric.Common;

    internal class ApplicationEntity
    {
        private const string TraceType = "ApplicationEntity";

        public ApplicationEntity(Application application, ClusterStateSnapshot clusterSnapshot)
        {
            this.Application = application;
            this.ServiceList = new List<ServiceEntity>();
            this.CodePackages = new List<CodePackageEntity>();
            this.ClusterSnapshot = clusterSnapshot;
        }

        public ClusterStateSnapshot ClusterSnapshot { get; internal set; }

        public Application Application { get; internal set; }

        // Not null and contains non-null entries.
        public IList<ServiceEntity> ServiceList { get; private set; }

        // Not null and contains non-null entries.
        public IList<CodePackageEntity> CodePackages { get; private set; }

        // Create ServiceEntity from Query.Service and add it to this node.
        public ServiceEntity AddService(Service service)
        {
            ServiceEntity serviceEntity = new ServiceEntity(service, this);
            this.ServiceList.Add(serviceEntity);
            return serviceEntity;
        }

        public CodePackageEntity AddCodePackage(DeployedCodePackage codePackage, string nodeName, DeployedServicePackageHealth health)
        {
            var codePackageTraceStr = new StringBuilder();
            codePackageTraceStr.Append(codePackage.CodePackageName);
            codePackageTraceStr.Append("_");
            codePackageTraceStr.Append(codePackage.CodePackageVersion);
            codePackageTraceStr.Append("_");
            codePackageTraceStr.Append(codePackage.ServiceManifestName);

            if (string.IsNullOrWhiteSpace(codePackage.ServicePackageActivationId))
            {
                codePackageTraceStr.Append("_");
                codePackageTraceStr.Append(codePackage.ServicePackageActivationId);
            }
            
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Inside of AddCodePackage: app={0}, cp={1}, nodename={2}",
                this.Application.ApplicationName.OriginalString,
                codePackageTraceStr.ToString(),
                nodeName);

            CodePackageEntity codePackageEntity = new CodePackageEntity(codePackage, nodeName, this, health);
            this.CodePackages.Add(codePackageEntity);
            return codePackageEntity;
        }

        public CodePackageEntity GetCodePackagEntityForReplica(ReplicaEntity replicaEntity)
        {
            CodePackageEntity codePackageEntity = null;
            var codePackagesOnNode = this.CodePackages.Where(cp => cp.NodeName == replicaEntity.Replica.NodeName);
            foreach (var codePackage in codePackagesOnNode)
            {
                var partitionEntity = codePackage.DeployedPartitions.FirstOrDefault(p => (p.Guid == replicaEntity.ParentPartitionEntity.Guid));
                if (partitionEntity != null)
                {
                    codePackageEntity = codePackage;
                }
            }

            return codePackageEntity;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendLine(string.Format("   Start for App: {0}", this.Application.ApplicationName));

            foreach (var svc in this.ServiceList)
            {
                sb.AppendLine(svc.ToString());
            }

            foreach (var cp in this.CodePackages)
            {
                sb.AppendLine(string.Format("    Start for Code Package: {0} for App {1}", cp.CodePackageResult.CodePackageName, this.Application.ApplicationName));
                sb.AppendLine(cp.ToString());

                foreach (var dp in cp.DeployedPartitions)
                {
                    sb.AppendLine(dp.ToString());
                }

                sb.AppendLine(string.Format("    End for Code Package: {0} for App {1}", cp.CodePackageResult.CodePackageName, this.Application.ApplicationName));
            }

            sb.AppendLine(string.Format("   End for App: {0}", this.Application.ApplicationName));

            return sb.ToString();
        }
    }
}