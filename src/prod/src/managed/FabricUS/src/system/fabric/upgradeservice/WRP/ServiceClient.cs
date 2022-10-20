// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.WRP.Model;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Newtonsoft.Json.Linq;

    public class ServiceClient : FabricClientApplicationWrapper
    {
        private readonly FabricClient fabricClient;

        public ServiceClient(FabricClient fabricClient)
        {
            fabricClient.ThrowIfNull(nameof(fabricClient));
            this.fabricClient = fabricClient;
        }

        protected ServiceClient()
        {
        }

        public override TraceType TraceType { get; protected set; } = new TraceType("ServiceClient");

        public override async Task<IFabricOperationResult> GetAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "GetAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ServiceOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var serviceOperationDescription = (ServiceOperationDescription)description;
            Service clusterService = (await this.fabricClient.QueryManager.GetServiceListAsync(
                serviceOperationDescription.ApplicationName,
                serviceOperationDescription.ServiceName,
                context.ContinuationToken,
                context.GetRemainingTimeOrThrow(),
                context.CancellationToken)).FirstOrDefault();

            var result = new FabricOperationResult()
            {
                OperationStatus = null,
                QueryResult = new ServiceClientFabricQueryResult(clusterService)
            };

            Trace.WriteInfo(
                TraceType,
                null == clusterService
                    ? "GetAsync: Service not found. Name: {0}."
                    : "GetAsync: Service exists. Name: {0}.",
                serviceOperationDescription.ServiceName);

            if (null == clusterService)
            {
                if (description.OperationType == OperationType.Delete)
                {
                    result.OperationStatus =
                        new ServiceOperationStatus(serviceOperationDescription)
                        {
                            Status = ResultStatus.Succeeded,
                            Progress = new JObject(),
                            ErrorDetails = new JObject()
                        };

                    return result;
                }

                return null;
            }

            var status = GetResultStatus(clusterService.ServiceStatus);
            var progress = new JObject();
            var errorDetails = new JObject(); //TODO: How to get the error details?

            result.OperationStatus = 
                new ServiceOperationStatus(serviceOperationDescription)
                {
                    Status = status,
                    Progress = progress,
                    ErrorDetails = errorDetails
                };

            return result;
        }

        public override async Task CreateAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "CreateAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ServiceOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var serviceOperationDescription = (ServiceOperationDescription)description;
            serviceOperationDescription.ApplicationName.ThrowIfNull(nameof(serviceOperationDescription.ApplicationName));
            serviceOperationDescription.ServiceName.ThrowIfNull(nameof(serviceOperationDescription.ServiceName));
            serviceOperationDescription.ServiceTypeName.ThrowIfNullOrWhiteSpace(nameof(serviceOperationDescription.ServiceTypeName));
            serviceOperationDescription.PartitionDescription.ThrowIfNull(nameof(serviceOperationDescription.PartitionDescription));
            Trace.WriteInfo(
                TraceType,
                "CreateAsync: Creating service. Name: {0}. Kind: {1}. Timeout: {2}",
                serviceOperationDescription.ServiceName,
                serviceOperationDescription.ServiceKind,
                context.OperationTimeout);
            await this.fabricClient.ServiceManager.CreateServiceAsync(
                this.CreateServiceDescription(serviceOperationDescription),
                context.OperationTimeout,
                context.CancellationToken);
            Trace.WriteInfo(TraceType, "CreateAsync: Create service call accepted");
        }

        public override async Task UpdateAsync(IOperationDescription description, IFabricOperationResult result, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "UpdateAsync called");
            description.ThrowIfNull(nameof(description));
            result.ThrowIfNull(nameof(result));
            result.OperationStatus.ThrowIfNull(nameof(result.OperationStatus));
            result.QueryResult.ThrowIfNull(nameof(result.QueryResult));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ServiceOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            if (!this.ValidObjectType<ServiceClientFabricQueryResult>(result.QueryResult, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var serviceOperationDescription = (ServiceOperationDescription)description;
            serviceOperationDescription.ServiceName.ThrowIfNull(nameof(serviceOperationDescription.ServiceName));
            Trace.WriteInfo(
                TraceType,
                "UpdateAsync: Updating service. Name: {0}. Timeout: {1}",
                serviceOperationDescription.ServiceName,
                context.OperationTimeout);
            await this.fabricClient.ServiceManager.UpdateServiceAsync(
                serviceOperationDescription.ServiceName,
                this.CreateServiceUpdateDescription(serviceOperationDescription), 
                context.OperationTimeout,
                context.CancellationToken);
            Trace.WriteInfo(TraceType, "UpdateAsync: Service update call accepted");
        }

        public override async Task DeleteAsync(IOperationDescription description, IOperationContext context)
        {
            Trace.WriteInfo(TraceType, "DeleteAsync called");
            description.ThrowIfNull(nameof(description));
            context.ThrowIfNull(nameof(context));

            string errorMessage;
            if (!this.ValidObjectType<ServiceOperationDescription>(description, out errorMessage))
            {
                throw new InvalidCastException(errorMessage);
            }

            var serviceOperationDescription = (ServiceOperationDescription)description;
            try
            {
                Trace.WriteInfo(
                    TraceType,
                    "DeleteAsync: Deleting service. Name: {0}. Timeout: {1}",
                    serviceOperationDescription.ServiceName,
                    context.OperationTimeout);
                await this.fabricClient.ServiceManager.DeleteServiceAsync(
                    new DeleteServiceDescription(serviceOperationDescription.ServiceName),
                    context.OperationTimeout,
                    context.CancellationToken);
                Trace.WriteInfo(TraceType, "DeleteAsync: Delete service call accepted");
            }
            catch (FabricServiceNotFoundException)
            {
                Trace.WriteInfo(
                    TraceType,
                    "DeleteAsync: Service not found. Name: {0}",
                    serviceOperationDescription.ServiceName);
            }
        }

        private ResultStatus GetResultStatus(ServiceStatus status)
        {
            switch (status)
            {
                case ServiceStatus.Active:
                    return ResultStatus.Succeeded;
                case ServiceStatus.Failed:
                    return ResultStatus.Failed;
                case ServiceStatus.Creating:
                case ServiceStatus.Deleting:
                case ServiceStatus.Upgrading:
                    return ResultStatus.InProgress;
            }

            Trace.WriteWarning(this.TraceType, "Invalid ServiceStatus: {0}", status);
            return ResultStatus.InProgress; // TODO: Should we create an Unknown/Invalid status?
        }

        private ServiceDescription CreateServiceDescription(ServiceOperationDescription serviceOperationDescription)
        {
            string errorMessage;
            ServiceDescription serviceDescription;
            switch (serviceOperationDescription.ServiceKind)
            {
                case ArmServiceKind.Stateful:
                    if (!this.ValidObjectType<StatefulServiceOperationDescription>(serviceOperationDescription, out errorMessage))
                    {
                        throw new InvalidCastException(errorMessage);
                    }

                    var statefulOperation = (StatefulServiceOperationDescription)serviceOperationDescription;
                    serviceDescription = new StatefulServiceDescription()
                    {
                        HasPersistedState = statefulOperation.HasPersistedState,
                        MinReplicaSetSize = statefulOperation.MinReplicaSetSize,
                        TargetReplicaSetSize = statefulOperation.TargetReplicaSetSize,
                        QuorumLossWaitDuration = statefulOperation.QuorumLossWaitDuration,
                        ReplicaRestartWaitDuration = statefulOperation.ReplicaRestartWaitDuration,
                        StandByReplicaKeepDuration = statefulOperation.StandByReplicaKeepDuration
                    };

                    break;
                case ArmServiceKind.Stateless:
                    if (!this.ValidObjectType<StatelessServiceOperationDescription>(serviceOperationDescription, out errorMessage))
                    {
                        throw new InvalidCastException(errorMessage);
                    }

                    var statelessOperation = (StatelessServiceOperationDescription)serviceOperationDescription;
                    serviceDescription = new StatelessServiceDescription()
                    {
                        InstanceCount = statelessOperation.InstanceCount
                    };

                    break;
                default:
                    throw new ArgumentOutOfRangeException(
                        nameof(serviceOperationDescription.ServiceKind),
                        serviceOperationDescription.ServiceKind,
                        $"{this.TraceType}: Unexpected ArmServiceKind");
            }

            serviceDescription.ApplicationName = serviceOperationDescription.ApplicationName;
            serviceDescription.ServiceName = serviceOperationDescription.ServiceName;

            if (serviceOperationDescription.DefaultMoveCost.HasValue)
            {
                serviceDescription.DefaultMoveCost =
                    (MoveCost)Enum.Parse(
                        typeof(MoveCost),
                        serviceOperationDescription.DefaultMoveCost.Value.ToString());
            }

            serviceDescription.PlacementConstraints = serviceOperationDescription.PlacementConstraints;
            serviceDescription.ServiceTypeName = serviceOperationDescription.ServiceTypeName;

            if (serviceOperationDescription.CorrelationScheme != null)
            {
                foreach (var scheme in serviceOperationDescription.CorrelationScheme)
                {
                    serviceDescription.Correlations.Add(this.GetServiceCorrelationDescription(scheme));
                }
            }

            if (serviceOperationDescription.ServiceLoadMetrics != null)
            {
                foreach (var metric in serviceOperationDescription.ServiceLoadMetrics)
                {
                    serviceDescription.Metrics.Add(this.GetServiceLoadMetricDescription(serviceOperationDescription.ServiceKind, metric));
                }
            }

            if (serviceOperationDescription.ServicePlacementPolicies != null)
            {
                foreach (var policy in serviceOperationDescription.ServicePlacementPolicies)
                {
                    serviceDescription.PlacementPolicies.Add(this.GetServicePlacementPolicyDescription(policy));
                }
            }

            PartitionSchemeDescription partitionSchemeDescription;
            switch (serviceOperationDescription.PartitionDescription.PartitionScheme)
            {
                case ArmPartitionScheme.Named:
                    if (!this.ValidObjectType<ArmNamedPartitionDescription>(serviceOperationDescription.PartitionDescription, out errorMessage))
                    {
                        throw new InvalidCastException(errorMessage);
                    }

                    var namedOperation = (ArmNamedPartitionDescription)serviceOperationDescription.PartitionDescription;
                    partitionSchemeDescription = new NamedPartitionSchemeDescription(namedOperation.Names);
                    break;
                case ArmPartitionScheme.Singleton:
                    partitionSchemeDescription = new SingletonPartitionSchemeDescription();
                    break;
                case ArmPartitionScheme.UniformInt64Range:
                    if (!this.ValidObjectType<ArmUniformInt64PartitionDescription>(serviceOperationDescription.PartitionDescription, out errorMessage))
                    {
                        throw new InvalidCastException(errorMessage);
                    }

                    var uniformOperation = (ArmUniformInt64PartitionDescription)serviceOperationDescription.PartitionDescription;
                    partitionSchemeDescription = new UniformInt64RangePartitionSchemeDescription()
                    {
                        HighKey = uniformOperation.HighKey,
                        LowKey = uniformOperation.LowKey,
                        PartitionCount = uniformOperation.Count
                    };

                    break;
                default:
                    throw new ArgumentOutOfRangeException(
                        nameof(serviceOperationDescription.PartitionDescription.PartitionScheme),
                        serviceOperationDescription.PartitionDescription.PartitionScheme,
                        $"{this.TraceType}: Unexpected ArmPartitionScheme");
            }

            serviceDescription.PartitionSchemeDescription = partitionSchemeDescription;

            return serviceDescription;
        }

        private ServiceUpdateDescription CreateServiceUpdateDescription(ServiceOperationDescription serviceOperationDescription)
        {
            string errorMessage;
            ServiceUpdateDescription serviceUpdateDescription;
            switch (serviceOperationDescription.ServiceKind)
            {
                case ArmServiceKind.Stateful:
                    if (!this.ValidObjectType<StatefulServiceOperationDescription>(serviceOperationDescription, out errorMessage))
                    {
                        throw new InvalidCastException(errorMessage);
                    }

                    var statefulOperation = (StatefulServiceOperationDescription)serviceOperationDescription;
                    serviceUpdateDescription = new StatefulServiceUpdateDescription()
                    {
                        MinReplicaSetSize = statefulOperation.MinReplicaSetSize,
                        QuorumLossWaitDuration = statefulOperation.QuorumLossWaitDuration,
                        ReplicaRestartWaitDuration = statefulOperation.ReplicaRestartWaitDuration,
                        StandByReplicaKeepDuration = statefulOperation.StandByReplicaKeepDuration,
                        TargetReplicaSetSize = statefulOperation.TargetReplicaSetSize
                    };

                    break;
                case ArmServiceKind.Stateless:
                    if (!this.ValidObjectType<StatelessServiceOperationDescription>(serviceOperationDescription, out errorMessage))
                    {
                        throw new InvalidCastException(errorMessage);
                    }

                    var statelessOperation = (StatelessServiceOperationDescription)serviceOperationDescription;
                    serviceUpdateDescription = new StatelessServiceUpdateDescription()
                    {
                        InstanceCount = statelessOperation.InstanceCount
                    };

                    break;
                default:
                    throw new ArgumentOutOfRangeException(
                        nameof(serviceOperationDescription.ServiceKind),
                        serviceOperationDescription.ServiceKind,
                        $"{this.TraceType}: Unexpected ArmServiceKind");
            }

            if (serviceOperationDescription.DefaultMoveCost.HasValue)
            {
                serviceUpdateDescription.DefaultMoveCost =
                    (MoveCost)Enum.Parse(
                        typeof(MoveCost),
                        serviceOperationDescription.DefaultMoveCost.Value.ToString());
            }

            serviceUpdateDescription.PlacementConstraints = serviceOperationDescription.PlacementConstraints;

            if (serviceOperationDescription.CorrelationScheme != null)
            {
                serviceUpdateDescription.Correlations = new List<ServiceCorrelationDescription>();
                foreach (var scheme in serviceOperationDescription.CorrelationScheme)
                {
                    serviceUpdateDescription.Correlations.Add(this.GetServiceCorrelationDescription(scheme));
                }
            }

            if (serviceOperationDescription.ServiceLoadMetrics != null)
            {
                serviceUpdateDescription.Metrics = new KeyedItemCollection<string, ServiceLoadMetricDescription>(n => n.Name);
                foreach (var metric in serviceOperationDescription.ServiceLoadMetrics)
                {
                    serviceUpdateDescription.Metrics.Add(this.GetServiceLoadMetricDescription(serviceOperationDescription.ServiceKind, metric));
                }
            }

            if (serviceOperationDescription.ServicePlacementPolicies != null)
            {
                serviceUpdateDescription.PlacementPolicies = new List<ServicePlacementPolicyDescription>();
                foreach (var policy in serviceOperationDescription.ServicePlacementPolicies)
                {
                    serviceUpdateDescription.PlacementPolicies.Add(this.GetServicePlacementPolicyDescription(policy));
                }
            }

            return serviceUpdateDescription;
        }

        private ServiceLoadMetricDescription GetServiceLoadMetricDescription(
            ArmServiceKind serviceKind, 
            ArmServiceLoadMetrics metric)
        {
            var weight = (ServiceLoadMetricWeight)Enum.Parse(
                        typeof(ServiceLoadMetricWeight),
                        metric.Weight.ToString());

            switch (serviceKind)
            {
                case ArmServiceKind.Stateful:
                    return new StatefulServiceLoadMetricDescription(metric.Name, 0, 0, weight);
                case ArmServiceKind.Stateless:
                    return new StatelessServiceLoadMetricDescription(metric.Name, 0, weight);
                default:
                    throw new ArgumentOutOfRangeException(
                        nameof(serviceKind),
                        serviceKind,
                        $"{this.TraceType}: Unexpected ArmServiceKind");
            }            
        }

        private ServiceCorrelationDescription GetServiceCorrelationDescription(ArmServiceCorrelationDescription scheme)
        {
            return new ServiceCorrelationDescription()
            {
                ServiceName = scheme.ServiceName,
                Scheme =
                    (ServiceCorrelationScheme) Enum.Parse(
                        typeof (ServiceCorrelationScheme),
                        scheme.Scheme.ToString())
            };
        }

        private ServicePlacementPolicyDescription GetServicePlacementPolicyDescription(ArmServicePlacementPolicyDescription policy)
        {
            string errorMessage;
            ServicePlacementPolicyDescription placementPolicy;
            switch (policy.Type)
            {
                case ArmServicePlacementPolicyType.InvalidDomain:
                    {
                        if (!this.ValidObjectType<ArmServicePlacementInvalidDomainPolicyDescription>(policy, out errorMessage))
                        {
                            throw new InvalidCastException(errorMessage);
                        }

                        var policyOperation = (ArmServicePlacementInvalidDomainPolicyDescription)policy;
                        placementPolicy = new ServicePlacementInvalidDomainPolicyDescription()
                        {
                            DomainName = policyOperation.DomainName
                        };

                        break;
                    }
                case ArmServicePlacementPolicyType.NonPartiallyPlaceService:
                    {
                        placementPolicy = new ServicePlacementNonPartiallyPlaceServicePolicyDescription();
                        break;
                    }
                case ArmServicePlacementPolicyType.PreferredPrimaryDomain:
                    {
                        if (!this.ValidObjectType<ArmServicePlacementPreferPrimaryDomainPolicyDescription>(policy, out errorMessage))
                        {
                            throw new InvalidCastException(errorMessage);
                        }

                        var policyOperation = (ArmServicePlacementPreferPrimaryDomainPolicyDescription)policy;
                        placementPolicy = new ServicePlacementPreferPrimaryDomainPolicyDescription()
                        {
                            DomainName = policyOperation.DomainName
                        };

                        break;
                    }
                case ArmServicePlacementPolicyType.RequiredDomain:
                    {
                        if (!this.ValidObjectType<ArmServicePlacementRequiredDomainPolicyDescription>(policy, out errorMessage))
                        {
                            throw new InvalidCastException(errorMessage);
                        }

                        var policyOperation = (ArmServicePlacementRequiredDomainPolicyDescription)policy;
                        placementPolicy = new ServicePlacementRequiredDomainPolicyDescription()
                        {
                            DomainName = policyOperation.DomainName
                        };

                        break;
                    }
                case ArmServicePlacementPolicyType.RequiredDomainDistribution:
                    {
                        placementPolicy = new ServicePlacementRequireDomainDistributionPolicyDescription();
                        break;
                    }
                default:
                    throw new ArgumentOutOfRangeException(
                        nameof(policy.Type),
                        policy.Type,
                        $"{this.TraceType}: Unexpected ArmServicePlacementPolicyType");
            }

            return placementPolicy;
        }
    }

    public class ServiceClientFabricQueryResult : IFabricQueryResult
    {
        public ServiceClientFabricQueryResult(Service service)
        {
            this.Service = service;
        }

        public Service Service { get; private set; }
    }
}