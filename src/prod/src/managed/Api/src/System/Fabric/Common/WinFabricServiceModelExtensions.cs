// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Linq;

    internal static class WinFabricServiceModelExtensions
    {
        public static bool IsStateful(this ServiceKind serviceKind)
        {
            return serviceKind == ServiceKind.Stateful;
        }

        public static bool IsStateful(this Service service)
        {
            return service.ServiceKind.IsStateful();
        }

        public static bool IsStateful(this ServiceDescription serviceDesc)
        {
            return serviceDesc.Kind == ServiceDescriptionKind.Stateful;
        }

        public static int TotalReplicasOrInstances(this ServiceDescription serviceDesc)
        {
            if (serviceDesc is StatefulServiceDescription)
            {
                return (serviceDesc as StatefulServiceDescription).TargetReplicaSetSize;
            }

            return (serviceDesc as StatelessServiceDescription).InstanceCount;
        }

        public static ServiceDescription GetClone(this ServiceDescription serviceDesc)
        {
            if (serviceDesc is StatefulServiceDescription)
            {
                return GetCloneStateful(serviceDesc as StatefulServiceDescription);
            }

            return GetCloneStateless(serviceDesc as StatelessServiceDescription);
        }

        public static StatefulServiceDescription GetCloneStateful(this StatefulServiceDescription other)
        {
            StatefulServiceDescription clone = new StatefulServiceDescription()
            {
                HasPersistedState = other.HasPersistedState,
                MinReplicaSetSize = other.MinReplicaSetSize,
                TargetReplicaSetSize = other.TargetReplicaSetSize,
            };

            clone.Metrics.AddRangeNullSafe(other.Metrics.Select(m => (m as StatefulServiceLoadMetricDescription).GetClone()));
            // Copy properties
            clone.CopyFrom(other);
            return clone;
        }

        public static StatelessServiceDescription GetCloneStateless(this StatelessServiceDescription other)
        {
            StatelessServiceDescription clone = new StatelessServiceDescription()
            {
                InstanceCount = other.InstanceCount
            };

            clone.Metrics.AddRangeNullSafe(other.Metrics.Select(m => (m as StatelessServiceLoadMetricDescription).GetClone()));
            // Copy properties
            clone.CopyFrom(other);
            return clone;
        }

        public static void CopyFrom(this ServiceDescription dest, ServiceDescription copySrc)
        {
            // Copy basic types
            dest.ServiceTypeName = copySrc.ServiceTypeName;
            dest.ApplicationName = copySrc.ApplicationName;
            dest.ServiceName = copySrc.ServiceName;
            dest.PlacementConstraints = copySrc.PlacementConstraints;
            dest.InitializationData = copySrc.InitializationData == null ? null : copySrc.InitializationData.ToArray();

            // Create copies of reference types
            dest.PartitionSchemeDescription = copySrc.PartitionSchemeDescription.GetClone();
            dest.Correlations.AddRangeNullSafe(copySrc.Correlations.Select(c => c.GetClone()));
            dest.PlacementPolicies.AddRangeNullSafe(copySrc.PlacementPolicies.Select(p => p.GetClone()));
        }

        public static ServiceCorrelationDescription GetClone(this ServiceCorrelationDescription other)
        {
            if (other == null) return null;
            return new ServiceCorrelationDescription()
            {
                Scheme = other.Scheme,
                ServiceName = other.ServiceName
            };
        }

        public static StatefulServiceLoadMetricDescription GetClone(this StatefulServiceLoadMetricDescription other)
        {
            if (other == null) return null;
            return new StatefulServiceLoadMetricDescription()
            {
                Name = other.Name,
                PrimaryDefaultLoad = other.PrimaryDefaultLoad,
                SecondaryDefaultLoad = other.SecondaryDefaultLoad,
                Weight = other.Weight
            };
        }

        public static StatelessServiceLoadMetricDescription GetClone(this StatelessServiceLoadMetricDescription other)
        {
            if (other == null) return null;
            return new StatelessServiceLoadMetricDescription()
            {
                Name = other.Name,
                DefaultLoad = other.DefaultLoad,
                Weight = other.Weight
            };
        }

        public static ServicePlacementPolicyDescription GetClone(this ServicePlacementPolicyDescription other)
        {
            if (other == null) return null;
            ServicePlacementPolicyDescription clone;

            if (other is ServicePlacementInvalidDomainPolicyDescription)
            {
                clone = new ServicePlacementInvalidDomainPolicyDescription()
                {
                    DomainName = (other as ServicePlacementInvalidDomainPolicyDescription).DomainName
                };
            }
            else if (other is ServicePlacementRequiredDomainPolicyDescription)
            {
                clone = new ServicePlacementRequiredDomainPolicyDescription()
                {
                    DomainName = (other as ServicePlacementRequiredDomainPolicyDescription).DomainName
                };
            }
            else if (other is ServicePlacementPreferPrimaryDomainPolicyDescription)
            {
                clone = new ServicePlacementPreferPrimaryDomainPolicyDescription()
                {
                    DomainName = (other as ServicePlacementPreferPrimaryDomainPolicyDescription).DomainName
                };
            }
            else if (other is ServicePlacementRequireDomainDistributionPolicyDescription)
            {
                clone = new ServicePlacementRequireDomainDistributionPolicyDescription();
            }
            else
            {
                clone = new ServicePlacementNonPartiallyPlaceServicePolicyDescription();
            }

            return clone;
        }

        public static PartitionSchemeDescription GetClone(this PartitionSchemeDescription partitionSchemeDesc)
        {
            if (partitionSchemeDesc is SingletonPartitionSchemeDescription)
            {
                return new SingletonPartitionSchemeDescription();
            }

            if (partitionSchemeDesc is NamedPartitionSchemeDescription)
            {
                var n1 = partitionSchemeDesc as NamedPartitionSchemeDescription;
                var n2 = new NamedPartitionSchemeDescription();

                foreach (var name in n1.PartitionNames)
                {
                    n2.PartitionNames.Add(name);
                }

                return n2;
            }

            var u1 = partitionSchemeDesc as UniformInt64RangePartitionSchemeDescription;
            var u2 = new UniformInt64RangePartitionSchemeDescription() { PartitionCount = u1.PartitionCount, LowKey = u1.LowKey, HighKey = u1.HighKey };
            return u2;
        }

        public static void AddNames(this NamedPartitionSchemeDescription partitionSchemeDesc, IEnumerable<string> names)
        {
            foreach (var name in names)
            {
                partitionSchemeDesc.PartitionNames.Add(name);
            }
        }

        public static int TotalPartitions(this PartitionSchemeDescription partitionSchemeDesc)
        {
            if (partitionSchemeDesc is SingletonPartitionSchemeDescription)
            {
                return 1;
            }

            if (partitionSchemeDesc is NamedPartitionSchemeDescription)
            {
                return (partitionSchemeDesc as NamedPartitionSchemeDescription).PartitionNames.Count;
            }

            return (partitionSchemeDesc as UniformInt64RangePartitionSchemeDescription).PartitionCount;
        }

        public static Guid PartitionId(this Partition partition)
        {
            return partition.PartitionInformation.Id;
        }

        public static ServicePartitionKind ServicePartitionKind(this Partition partition)
        {
            return partition.PartitionInformation.Kind;
        }

        // No Thow
        public static long LowKey(this ServicePartitionInformation partition)
        {
            return DefaultOnNullException<long>(
                () =>
                    (partition as Int64RangePartitionInformation).LowKey);
        }

        // No Thow
        public static long HighKey(this ServicePartitionInformation partition)
        {
            return DefaultOnNullException<long>(
                () =>
                    (partition as Int64RangePartitionInformation).HighKey);
        }

        // No Throw
        public static string Name(this ServicePartitionInformation partition)
        {
            return DefaultOnNullException<string>(
                () =>
                    (partition as NamedPartitionInformation).Name);
        }

        //No Throw
        public static long LowKey(this Partition partition)
        {
            return partition.PartitionInformation.LowKey();
        }

        // No Throw
        public static long HighKey(this Partition partition)
        {
            return partition.PartitionInformation.HighKey();
        }

        // No Throw
        public static string Name(this Partition partition)
        {
            return partition.PartitionInformation.Name();
        }


        // No Throw
        public static IList<ApplicationParameter> ToIList(this ApplicationParameterList appParams)
        {
            var result = new List<ApplicationParameter>();
            if (appParams == null)
            {
                return result;
            }

            foreach (var appParam in appParams)
            {
                result.Add(appParam);
            }

            return result;
        }

        // If source is null then it is ignored.
        // Can throw if dest is null.
        public static void AddRangeNullSafe<T>(this ICollection<T> dest, IEnumerable<T> sourceNullAsEmpty)
        {
            if (sourceNullAsEmpty == null) return;

            foreach (T item in sourceNullAsEmpty)
            {
                dest.Add(item);
            }
        }

        // Can throw on null argument.
        public static bool IsNodeUp(this Node node)
        {
            return node.NodeStatus == NodeStatus.Up ||
                node.NodeStatus == NodeStatus.Disabled ||
                node.NodeStatus == NodeStatus.Disabling ||
                node.NodeStatus == NodeStatus.Enabling;
        }

        public static T DefaultOnNullException<T>(Func<T> funcToRun)
        {
            T result = default(T);

            try
            {
                result = funcToRun();
            }
            catch (NullReferenceException)
            {
            }

            return result;
        }
    }
}