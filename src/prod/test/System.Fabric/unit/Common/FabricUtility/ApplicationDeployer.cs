// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Description;
    using System.IO;
    using System.Linq;
    using System.Xml;

    using ServiceModel = System.Fabric.Management.ServiceModel;
    using System.Xml.Serialization;
    using System.Fabric.Management.ServiceModel;

    /// <summary>
    /// Contains the information required for a service
    /// </summary>
    public class ServiceDeployer
    {
        public const string DefaultCodePackageName = "my_code_package";

        /// <summary>
        /// The additional binaries to be included with the code package
        /// </summary>
        public List<string> CodePackageFiles { get; set; }

        /// <summary>
        /// The additional binaries to be included with the config package
        /// </summary>
        public List<string> ConfigPackageFiles { get; set; }

        /// <summary>
        /// The additional binaries to be included with the data package
        /// </summary>
        public List<string> DataPackageFiles { get; set; }

        /// <summary>
        /// The configuration settings to be provided to the service
        /// These will be available in the ServiceActivationParameters.ConfigurationPackage
        /// It is of the format (section, parameter, value)
        /// </summary>
        public List<Tuple<string, string, string>> ConfigurationSettings { get; set; }

        /// <summary>
        /// The winfab name of the service (only for default services), if it's empty, service name will be same to service type name
        /// </summary>
        public string ServiceName { get; set; }

        /// <summary>
        /// The winfab name of the service type
        /// </summary>
        public string ServiceTypeName { get; set; }

        /// <summary>
        /// The winfab name of the service group type
        /// </summary>
        public string ServiceGroupTypeName { get; set; }

        /// <summary>
        /// The CLR type that implements one of the following:
        /// IStatelessServiceFactory, IStatefulServiceFactory, IStatelessServiceInstance, IStatefulServiceReplica
        /// </summary>
        public Type ServiceTypeImplementation { get; set; }
        
        /// <summary>
        /// The version of the service (defaults to 1.0)
        /// </summary>
        public string Version { get; set; }

        /// <summary>
        /// The extensions
        /// </summary>
        public List<Tuple<string, string>> Extensions { get; set;}

        /// <summary>
        /// The instance count to set for this service (defaults to 1)
        /// </summary>
        public int InstanceCount { get; set; }

        /// <summary>
        /// The partition (defaults to SingletonPartition)
        /// </summary>
        public PartitionSchemeDescription Partition { get; set; }

        /// <summary>
        /// The min replica set size (defaults to 1)
        /// </summary>
        public int MinReplicaSetSize { get; set; }

        /// <summary>
        /// The targetreplicasetsize (defaults to 1)
        /// </summary>
        public int TargetReplicaSetSize { get; set; }

        /// <summary>
        /// The loadmetrics (defaults to null)
        /// </summary>
        public List<ServiceLoadMetricDescription> LoadMetrics { get; set; }

        /// <summary>
        /// The placementconstraints (defaults to null)
        /// </summary>
        public string PlacementConstraints { get; set; }

        /// <summary>
        /// The service correlations (defaults to null)
        /// </summary>
        public List<ServiceCorrelationDescription> ServiceCorrelations { get; set; }

        /// <summary>
        /// The service placement policy (defaults to null)
        /// </summary>
        public List<ServicePlacementPolicyDescription> ServicePlacementPolicies { get; set; }

        /// <summary>
        /// The service scaling policy (defaults to null)
        /// </summary>
        public List<ScalingPolicyDescription> ScalingPolicies { get; set; }

        /// <summary>
        /// Default move cost for the service
        /// </summary>
        public MoveCost? DefaultMoveCost;

        /// <summary>
        /// The name of the code package for this service
        /// </summary>
        public string CodePackageName { get; set; }

        /// <summary>
        /// The name of the config package for this service
        /// Set this to null if no config package
        /// </summary>
        public string ConfigPackageName { get; set; }

        /// <summary>
        /// The name for the data package for this service
        /// Set this to null if no data package
        /// </summary>
        public string DataPackageName { get; set; }

        /// <summary>
        /// The endpoints to create
        /// </summary>
        public List<Tuple<string, EndpointProtocol>> Endpoints { get; private set; }

        public bool HasPersistedState { get; set; }

        public bool IsStateless
        {
            get
            {
                if (typeof(IStatelessServiceFactory).IsAssignableFrom(this.ServiceTypeImplementation))
                {
                    return true;
                }
                else if (typeof(IStatelessServiceInstance).IsAssignableFrom(this.ServiceTypeImplementation))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
        
        public ServiceDeployer(string codePackageName)
        {
            this.CodePackageFiles = new List<string>();
            this.ConfigPackageFiles = new List<string>();
            this.DataPackageFiles = new List<string>();
            this.Extensions = new List<Tuple<string, string>>();
            this.ConfigurationSettings = new List<Tuple<string, string, string>>();
            this.InstanceCount = 1;
            this.Partition = new SingletonPartitionSchemeDescription();
            this.MinReplicaSetSize = 1;
            this.TargetReplicaSetSize = 1;
            this.LoadMetrics = null;
            this.PlacementConstraints = null;
            this.ServiceCorrelations = null;
            this.ServicePlacementPolicies = null;
            this.Version = "1.0";
            this.CodePackageName = codePackageName;
            this.ConfigPackageName = null;
            this.DataPackageName = null;
            this.Endpoints = new List<Tuple<string, EndpointProtocol>>();
            this.HasPersistedState = false;
            this.ScalingPolicies = null;
        }

        public ServiceDeployer() : this(ServiceDeployer.DefaultCodePackageName) { }

        /// <summary>
        /// Adds all the required elements to the manifest
        /// <param name="deploymentFolder">Must be the folder where the service manifest will be placed</param>
        /// </summary>        
        public void Generate(ServiceModel.ServiceManifestType manifest, string deploymentFolder)
        {
            Debug.Assert(!string.IsNullOrEmpty(this.ServiceTypeName));
            Debug.Assert(this.ServiceTypeImplementation != null);
            Debug.Assert(this.CodePackageFiles.Count != 0);
            Debug.Assert(this.CodePackageName != null);

            bool implementsRequiredInterface =
                typeof(IStatelessServiceFactory).IsAssignableFrom(this.ServiceTypeImplementation) ||
                typeof(IStatelessServiceInstance).IsAssignableFrom(this.ServiceTypeImplementation) ||
                typeof(IStatefulServiceFactory).IsAssignableFrom(this.ServiceTypeImplementation) ||
                typeof(IStatefulServiceReplica).IsAssignableFrom(this.ServiceTypeImplementation);

            Debug.Assert(implementsRequiredInterface);

            if (this.ConfigurationSettings.Count != 0 && this.ConfigPackageName == null)
            {
                throw new ArgumentException("Configuration settings provided without a config package name");
            }

            if ((this.ConfigPackageFiles.Count != 0 && this.ConfigPackageName == null) || (this.DataPackageFiles.Count != 0 && this.DataPackageName == null))
            {
                throw new ArgumentException("Config/Data package had files but no package name");
            }

            ServiceModel.ServiceTypeType serviceTypeDescription = null;
            if (this.IsStateless)
            {
                serviceTypeDescription = new ServiceModel.StatelessServiceTypeType
                {
                    ServiceTypeName = this.ServiceTypeName,
                    LoadMetrics = GenerateStatelessLoadMetrics(),
                    PlacementConstraints = this.PlacementConstraints
                };
            }
            else
            {
                serviceTypeDescription = new ServiceModel.StatefulServiceTypeType
                {
                    ServiceTypeName = this.ServiceTypeName,
                    HasPersistedState = this.HasPersistedState,
                    LoadMetrics = GenerateStatefulLoadMetrics(),
                    PlacementConstraints = this.PlacementConstraints
                };
            }

            XmlDocument dummyDocument = new XmlDocument() { XmlResolver = null };
            var extensionValue = dummyDocument.CreateElement("DefaultEntryPoint", "http://schemas.microsoft.com/2011/01/fabric/servicetypeextension");
            extensionValue.InnerText = DefaultEntryPoint.CreateExtensionValueFromTypeInformation(this.CodePackageName, this.ServiceTypeImplementation);

            serviceTypeDescription.Extensions = new ServiceModel.ExtensionsTypeExtension[]
            {
                new ServiceModel.ExtensionsTypeExtension
                {
                    Name = DefaultEntryPoint.ServiceImplementationTypeExtensionName,
                    Any = extensionValue
                }
            };

            manifest.ServiceTypes = ServiceDeployer.CreateOrAppend<object>(manifest.ServiceTypes, (object)serviceTypeDescription);

            if (this.ServiceGroupTypeName != null)
            {
                ServiceModel.ServiceGroupTypeType serviceGroupTypeDescription = null;
                if (this.IsStateless)
                {
                    serviceGroupTypeDescription = new ServiceModel.StatelessServiceGroupTypeType
                    {
                        ServiceGroupTypeName = this.ServiceGroupTypeName,
                        ServiceGroupMembers = new ServiceGroupTypeMember[2] {
                            new ServiceGroupTypeMember { ServiceTypeName = this.ServiceTypeName },
                            new ServiceGroupTypeMember { ServiceTypeName = this.ServiceTypeName }
                        }
                    };
                }
                else
                {
                    serviceGroupTypeDescription = new ServiceModel.StatefulServiceGroupTypeType
                    {
                        ServiceGroupTypeName = this.ServiceGroupTypeName,
                        ServiceGroupMembers = new ServiceGroupTypeMember[2] {
                            new ServiceGroupTypeMember { ServiceTypeName = this.ServiceTypeName },
                            new ServiceGroupTypeMember { ServiceTypeName = this.ServiceTypeName }
                        }
                    };
                }

                manifest.ServiceTypes = ServiceDeployer.CreateOrAppend<object>(manifest.ServiceTypes, (object)serviceGroupTypeDescription);
            }

            // deploy the packages

            // code package
            // add "system.fabric.test.dll" as a required file
            // add "system.fabric.test.host.dll" as a required file
            this.CodePackageFiles.Add(typeof(ApplicationDeployer).Assembly.Location);

            this.CodePackageFiles.Add(GetFileInAssemblyLocation("System.Fabric.Test.Host.exe"));
            this.DeployPackage(deploymentFolder, this.CodePackageName, this.CodePackageFiles);
            manifest.CodePackage = ServiceDeployer.CreateOrAppend(manifest.CodePackage, this.GenerateCodePackageDescription());

            // config package
            if (this.ConfigPackageName != null)
            {
                this.DeployPackage(deploymentFolder, this.ConfigPackageName, this.ConfigPackageFiles);
                manifest.ConfigPackage = ServiceDeployer.CreateOrAppend(manifest.ConfigPackage, this.GenerateConfigPackageDescription());
                
                this.WriteConfigurationPackage(deploymentFolder);
            }

            // data package
            if (this.DataPackageName != null)
            {
                this.DeployPackage(deploymentFolder, this.DataPackageName, this.DataPackageFiles);
                manifest.DataPackage = ServiceDeployer.CreateOrAppend(manifest.DataPackage, this.GenerateDataPackageDescription());
            }

            manifest.Resources = manifest.Resources ?? new ServiceModel.ResourcesType();
            manifest.Resources.Endpoints = ServiceDeployer.CreateOrAppend(manifest.Resources.Endpoints, this.GenerateEndpoints());
        }

        public ServiceModel.ServiceType GenerateServiceDescription()
        {
            ServiceModel.ServiceType description = null;
            if (this.IsStateless)
            {
                description = new ServiceModel.StatelessServiceType
                {
                    ServiceTypeName = this.ServiceTypeName,
                    InstanceCount = this.InstanceCount.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    LoadMetrics = GenerateStatelessLoadMetrics(),
                    PlacementConstraints = this.PlacementConstraints,
                    ServiceCorrelations = GenerateServiceCorrelations(),
                    ServicePlacementPolicies = GenerateServicePlacementPolicies(),
                    ServiceScalingPolicies = GenerateScalingPolicy()
                };
                if (DefaultMoveCost.HasValue)
                {
                    description.DefaultMoveCostSpecified = true;
                    description.DefaultMoveCost = (ServiceTypeDefaultMoveCost)DefaultMoveCost.Value;
                }
            }
            else
            {
                description = new ServiceModel.StatefulServiceType
                {
                    ServiceTypeName = this.ServiceTypeName,
                    MinReplicaSetSize = this.MinReplicaSetSize.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    TargetReplicaSetSize = this.TargetReplicaSetSize.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    LoadMetrics = GenerateStatefulLoadMetrics(),
                    PlacementConstraints = this.PlacementConstraints,
                    ServiceCorrelations = GenerateServiceCorrelations(),
                    ServicePlacementPolicies = GenerateServicePlacementPolicies()
                };
                if (DefaultMoveCost.HasValue)
                {
                    description.DefaultMoveCostSpecified = true;
                    description.DefaultMoveCost = (ServiceTypeDefaultMoveCost)DefaultMoveCost.Value;
                }
            }

            SetPartitionDescription(description, this.Partition);

            return description;
        }

        private string GetFileInAssemblyLocation(string fileName)
        {
            var filePath = Path.Combine(Path.GetDirectoryName(typeof (ApplicationDeployer).Assembly.Location), fileName);
            if (!File.Exists(filePath))
            {
                throw new InvalidOperationException(string.Format("File {0} not found", filePath));
            }

            return filePath;
        }

        private static TItem[] CreateOrAppend<TItem>(TItem[] existingItems, params TItem[] newItems)
        {
            if (existingItems == null)
            {
                if (newItems == null || newItems.Length == 0)
                {
                    return null;
                }
                return newItems;
            }
            else
            {
                Debug.Assert(newItems != null);
                if (newItems.Length == 0)
                {
                    return existingItems;
                }
                return existingItems.Concat(newItems).ToArray();
            }
        }

        private ServiceModel.LoadMetricType[] GenerateStatefulLoadMetrics()
        {
            return this.LoadMetrics == null ? null : this.LoadMetrics.Select(lm =>
                    new ServiceModel.LoadMetricType
                    {
                        Name = lm.Name,
                        Weight = GetLoadMetricTypeWeight(lm.Weight),
                        PrimaryDefaultLoad = (uint)(lm as StatefulServiceLoadMetricDescription).PrimaryDefaultLoad,
                        SecondaryDefaultLoad = (uint)(lm as StatefulServiceLoadMetricDescription).SecondaryDefaultLoad,
                        WeightSpecified = true
                    }
                ).ToArray();
        }

        private ServiceModel.LoadMetricType[] GenerateStatelessLoadMetrics()
        {
            return this.LoadMetrics == null ? null : this.LoadMetrics.Select(lm =>
                    new ServiceModel.LoadMetricType
                    {
                        Name = lm.Name,
                        Weight = GetLoadMetricTypeWeight(lm.Weight),
                        DefaultLoad = (uint)(lm as StatelessServiceLoadMetricDescription).DefaultLoad,
                        WeightSpecified = true
                    }
                ).ToArray();
        }

        private ServiceModel.ScalingPolicyType[] GenerateScalingPolicy()
        {
            if (this.ScalingPolicies != null)
            {
                var allPolicies = new ScalingPolicyType[this.ScalingPolicies.Count];
                for (int i = 0; i < this.ScalingPolicies.Count; ++i)
                {
                    var scalingPolicy = this.ScalingPolicies[i];
                    var scaling = new ScalingPolicyType();
                    if (scalingPolicy.ScalingMechanism.Kind == ScalingMechanismKind.AddRemoveIncrementalNamedPartition)
                    {
                        var mechanism = new ServiceModel.ScalingPolicyTypeAddRemoveIncrementalNamedPartitionScalingMechanism();
                        mechanism.ScaleIncrement = ((AddRemoveIncrementalNamedPartitionScalingMechanism)scalingPolicy.ScalingMechanism).ScaleIncrement.ToString();
                        mechanism.MinPartitionCount = ((AddRemoveIncrementalNamedPartitionScalingMechanism)scalingPolicy.ScalingMechanism).MinPartitionCount.ToString();
                        mechanism.MaxPartitionCount = ((AddRemoveIncrementalNamedPartitionScalingMechanism)scalingPolicy.ScalingMechanism).MaxPartitionCount.ToString();
                        scaling.AddRemoveIncrementalNamedPartitionScalingMechanism = mechanism;
                    }
                    else if (scalingPolicy.ScalingMechanism.Kind == ScalingMechanismKind.ScalePartitionInstanceCount)
                    {
                        var mechanism = new ServiceModel.ScalingPolicyTypeInstanceCountScalingMechanism();
                        mechanism.ScaleIncrement = ((PartitionInstanceCountScaleMechanism)scalingPolicy.ScalingMechanism).ScaleIncrement.ToString();
                        mechanism.MinInstanceCount = ((PartitionInstanceCountScaleMechanism)scalingPolicy.ScalingMechanism).MinInstanceCount.ToString();
                        mechanism.MaxInstanceCount = ((PartitionInstanceCountScaleMechanism)scalingPolicy.ScalingMechanism).MaxInstanceCount.ToString();
                        scaling.InstanceCountScalingMechanism = mechanism;
                    }
                    if (scalingPolicy.ScalingTrigger.Kind == ScalingTriggerKind.AveragePartitionLoadTrigger)
                    {
                        var trigger = new ServiceModel.ScalingPolicyTypeAveragePartitionLoadScalingTrigger();
                        trigger.ScaleIntervalInSeconds = ((AveragePartitionLoadScalingTrigger)scalingPolicy.ScalingTrigger).ScaleInterval.Value.TotalSeconds.ToString();
                        trigger.MetricName = ((AveragePartitionLoadScalingTrigger)scalingPolicy.ScalingTrigger).MetricName;
                        trigger.LowerLoadThreshold = ((AveragePartitionLoadScalingTrigger)scalingPolicy.ScalingTrigger).LowerLoadThreshold.ToString();
                        trigger.UpperLoadThreshold = ((AveragePartitionLoadScalingTrigger)scalingPolicy.ScalingTrigger).UpperLoadThreshold.ToString();
                        scaling.AveragePartitionLoadScalingTrigger = trigger;
                    }
                    else if (scalingPolicy.ScalingTrigger.Kind == ScalingTriggerKind.AverageServiceLoadTrigger)
                    {
                        var trigger = new ServiceModel.ScalingPolicyTypeAverageServiceLoadScalingTrigger();
                        trigger.ScaleIntervalInSeconds = ((AverageServiceLoadScalingTrigger)scalingPolicy.ScalingTrigger).ScaleInterval.Value.TotalSeconds.ToString();
                        trigger.MetricName = ((AverageServiceLoadScalingTrigger)scalingPolicy.ScalingTrigger).MetricName;
                        trigger.LowerLoadThreshold = ((AverageServiceLoadScalingTrigger)scalingPolicy.ScalingTrigger).LowerLoadThreshold.ToString();
                        trigger.UpperLoadThreshold = ((AverageServiceLoadScalingTrigger)scalingPolicy.ScalingTrigger).UpperLoadThreshold.ToString();
                        trigger.UseOnlyPrimaryLoad = ((AverageServiceLoadScalingTrigger)scalingPolicy.ScalingTrigger).UseOnlyPrimaryLoad.ToString();
                        scaling.AverageServiceLoadScalingTrigger = trigger;
                    }
                    allPolicies[i] = scaling;
                }
                return allPolicies;
            }
            else
            {
                return null;    
            }
        }

        private static ServiceModel.LoadMetricTypeWeight GetLoadMetricTypeWeight(ServiceLoadMetricWeight weight)
        {
            switch (weight)
            {
                case ServiceLoadMetricWeight.High:
                    return ServiceModel.LoadMetricTypeWeight.High;
                case ServiceLoadMetricWeight.Medium:
                    return ServiceModel.LoadMetricTypeWeight.Medium;
                case ServiceLoadMetricWeight.Low:
                    return ServiceModel.LoadMetricTypeWeight.Low;
                case ServiceLoadMetricWeight.Zero:
                    return ServiceModel.LoadMetricTypeWeight.Zero;
            }

            throw new ArgumentException("loadMetricTypeWeight");
        }

        private ServiceModel.ServiceTypeServiceCorrelation[] GenerateServiceCorrelations()
        {
            return this.ServiceCorrelations == null ? null : this.ServiceCorrelations.Select(c =>
                    new ServiceModel.ServiceTypeServiceCorrelation
                    {
                        ServiceName = c.ServiceName.ToString(),
                        Scheme = GetServiceCorrelationScheme(c.Scheme)
                    }
                ).ToArray();
        }

        private static ServiceModel.ServiceTypeServiceCorrelationScheme GetServiceCorrelationScheme(ServiceCorrelationScheme scheme)
        {
            switch (scheme)
            {
                case ServiceCorrelationScheme.Affinity:
                    return ServiceModel.ServiceTypeServiceCorrelationScheme.Affinity;
                case ServiceCorrelationScheme.AlignedAffinity:
                    return ServiceModel.ServiceTypeServiceCorrelationScheme.AlignedAffinity;
                case ServiceCorrelationScheme.NonAlignedAffinity:
                    return ServiceModel.ServiceTypeServiceCorrelationScheme.NonAlignedAffinity;
            }
            throw new ArgumentException("serviceCorrelationScheme");
        }

        private ServiceModel.ServiceTypeServicePlacementPolicy[] GenerateServicePlacementPolicies()
        {
            return this.ServicePlacementPolicies == null ? null : this.ServicePlacementPolicies.Select(p =>
                new ServiceModel.ServiceTypeServicePlacementPolicy
                {
                    DomainName = GetServicePlacementPolicyDomainName(p),
                    Type = GetServicePlacementPolicyType(p.Type)
                }
            ).ToArray();
        }

        private static string GetServicePlacementPolicyDomainName(ServicePlacementPolicyDescription policy)
        {
            if (policy is ServicePlacementInvalidDomainPolicyDescription)
            {
                return (policy as ServicePlacementInvalidDomainPolicyDescription).DomainName;
            }
            else if (policy is ServicePlacementRequiredDomainPolicyDescription)
            {
                return (policy as ServicePlacementRequiredDomainPolicyDescription).DomainName;
            }
            else if (policy is ServicePlacementPreferPrimaryDomainPolicyDescription)
            {
                return (policy as ServicePlacementPreferPrimaryDomainPolicyDescription).DomainName;
            }
            else if (policy is ServicePlacementRequireDomainDistributionPolicyDescription)
            {
                return string.Empty;
            }
            else if (policy is ServicePlacementNonPartiallyPlaceServicePolicyDescription)
            {
                return string.Empty;
            }
            else
            {
                throw new ArgumentException("servicePlacementPolicy");
            }
        }

        private static ServiceModel.ServiceTypeServicePlacementPolicyType GetServicePlacementPolicyType(ServicePlacementPolicyType type)
        {
            switch (type)
            {
                case ServicePlacementPolicyType.InvalidDomain:
                    return ServiceModel.ServiceTypeServicePlacementPolicyType.InvalidDomain;
                case ServicePlacementPolicyType.PreferPrimaryDomain:
                    return ServiceModel.ServiceTypeServicePlacementPolicyType.PreferredPrimaryDomain;
                case ServicePlacementPolicyType.RequireDomain:
                    return ServiceModel.ServiceTypeServicePlacementPolicyType.RequiredDomain;
                case ServicePlacementPolicyType.RequireDomainDistribution:
                    return ServiceModel.ServiceTypeServicePlacementPolicyType.RequiredDomainDistribution;
                case ServicePlacementPolicyType.NonPartiallyPlaceService:
                    return ServiceModel.ServiceTypeServicePlacementPolicyType.NonPartiallyPlace;
            }

            throw new ArgumentException("servicePlacementPolicy");
        }

        private ServiceModel.EndpointType[] GenerateEndpoints()
        {
            return this.Endpoints
                .Select(ep =>
                    new ServiceModel.EndpointType
                    {
                        Name = ep.Item1,
                        Type = ServiceModel.EndpointTypeType.Input,
                        Protocol = ServiceDeployer.GetServiceModelProtocol(ep.Item2),
                        CertificateRef = string.Empty
                    }
                )
                .ToArray();
        }

        private static ServiceModel.EndpointTypeProtocol GetServiceModelProtocol(EndpointProtocol endpointProtocol)
        {
            switch (endpointProtocol)
            {
                case EndpointProtocol.Http:
                    return ServiceModel.EndpointTypeProtocol.http;
                case EndpointProtocol.Https:
                    return ServiceModel.EndpointTypeProtocol.https;
                case EndpointProtocol.Tcp:
                    return ServiceModel.EndpointTypeProtocol.tcp;
            }

            throw new ArgumentException("endpointProtocol");
        }

        private ServiceModel.DataPackageType GenerateDataPackageDescription()
        {
            ServiceModel.DataPackageType dataPackage = new ServiceModel.DataPackageType
            {
                Name = this.DataPackageName,
                Version = this.Version
            };
            return dataPackage;
        }

        private void WriteConfigurationPackage(string deploymentFolder)
        {
            ServiceModel.SettingsType settings = new ServiceModel.SettingsType();

            var sections = new List<ServiceModel.SettingsTypeSection>();
            foreach (var group in this.ConfigurationSettings.GroupBy(t => t.Item1))
            {
                var section = new ServiceModel.SettingsTypeSection
                {
                    Name = group.Key
                };

                section.Parameter = group
                    .Select(item =>
                    new ServiceModel.SettingsTypeSectionParameter
                    {
                        Name = item.Item2,
                        Value = item.Item3
                    })
                    .ToArray();

                sections.Add(section);
            }

            settings.Section = sections.Count == 0 ? null : sections.ToArray();

            // create the settings.xml file
            const string settingsFileName = @"Settings.xml";
            string settingsFilePath = Path.Combine(deploymentFolder, this.ConfigPackageName, settingsFileName);

            using (XmlWriter writer = XmlWriter.Create(settingsFilePath, new XmlWriterSettings { Indent = true }))
            {
                var serializer = new XmlSerializer(typeof(ServiceModel.SettingsType));
                serializer.Serialize(writer, settings);
                writer.Flush();
            }
        }

        private ServiceModel.ConfigPackageType GenerateConfigPackageDescription()
        {
            ServiceModel.ConfigPackageType configPackage = new ServiceModel.ConfigPackageType
            {
                Name = this.ConfigPackageName,
                Version = this.Version
            };
            return configPackage;
        }

        private ServiceModel.CodePackageType GenerateCodePackageDescription()
        {
            ServiceModel.CodePackageType codePackage = new ServiceModel.CodePackageType
            {
                Name = this.CodePackageName,
                EntryPoint = new ServiceModel.EntryPointDescriptionType
                {
                    Item = new ServiceModel.EntryPointDescriptionTypeExeHost
                    {
                        // See System.Fabric.Test.Host for the argument format
                        Arguments = string.Format("managed \"{0}\" \"{1}\"", Path.GetDirectoryName(this.GetType().Assembly.Location), typeof(DefaultEntryPoint).AssemblyQualifiedName),
                        Program = "System.Fabric.Test.Host.exe"
                    }
                },
                IsShared = false,
                Version = this.Version,
            };
            return codePackage;
        }

        private void DeployPackage(string serviceDeploymentFolder, string packageName, IEnumerable<string> files) 
        {
            string packageDeploymentFolder = Path.Combine(serviceDeploymentFolder, packageName);
            FileUtility.CopyFilesToDirectory(files, packageDeploymentFolder);
        }

        private static void SetPartitionDescription(ServiceModel.ServiceType description, PartitionSchemeDescription partitionDescription)
        {
            if (partitionDescription is SingletonPartitionSchemeDescription)
            {
                description.SingletonPartition = new ServiceModel.ServiceTypeSingletonPartition();
            }
            else if (partitionDescription is UniformInt64RangePartitionSchemeDescription)
            {
                var uniform = (UniformInt64RangePartitionSchemeDescription)partitionDescription;
                description.UniformInt64Partition = new ServiceModel.ServiceTypeUniformInt64Partition
                {
                    HighKey = uniform.HighKey.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    LowKey = uniform.LowKey.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    PartitionCount = uniform.PartitionCount.ToString(System.Globalization.CultureInfo.InvariantCulture)
                };
            }
            else if (partitionDescription is NamedPartitionSchemeDescription)
            {
                var named = (NamedPartitionSchemeDescription)partitionDescription;
                description.NamedPartition = new ServiceModel.ServiceTypeNamedPartition
                {
                    Partition = named.PartitionNames.Select(n => new ServiceModel.ServiceTypeNamedPartitionPartition { Name = n }).ToArray()
                };
            }
        }
    }

    /// <summary>
    /// Helper class used to generate the basic package that can be uploaded to image store for a service
    /// </summary>
    public class ApplicationDeployer
    {
        private const string UserName = "e2e_test_user";
        private string serviceManifestName;

        /// <summary>
        /// The application Name
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// The version - defaults to 1.0
        /// </summary>
        public string Version { get; set; }

        /// <summary>
        /// Application Parameters
        /// </summary>
        public IDictionary<string, string> Parameters { get; set; }
        
        public List<ServiceDeployer> Services { get; set; }

        public string ServiceManifestName
        {
            get
            {
                if (string.IsNullOrEmpty(serviceManifestName))
                {
                    this.serviceManifestName = this.Name + "ServicePackage";
                }

                return this.serviceManifestName;
            }
        }

        public ApplicationDeployer()
        {
            this.Services = new List<ServiceDeployer>();
            this.Version = "1.0";
            this.Parameters = new Dictionary<string,string>();
        }

        public void Deploy(string deploymentRootPath)
        {
            string serviceManifestFolder = Path.Combine(deploymentRootPath, this.ServiceManifestName);
            FileUtility.CreateDirectoryIfNotExists(serviceManifestFolder);

            LogHelper.Log("Service manifest being created at {0}", serviceManifestFolder);

            ServiceModel.ServiceManifestType serviceManifest = new ServiceModel.ServiceManifestType
            {
                Name = this.ServiceManifestName,
                Version = this.Version,
                Description = "Some description"
            };

            this.Services.ForEach((sd) => sd.Generate(serviceManifest, serviceManifestFolder));
            
            ServiceModel.ApplicationManifestType appManifest = new ServiceModel.ApplicationManifestType
            {
                ApplicationTypeName = this.Name,
                ApplicationTypeVersion = this.Version,
                Parameters = this.Parameters.Select
                    (
                        item => new ServiceModel.ApplicationManifestTypeParameter() 
                                { 
                                    Name = item.Key, 
                                    DefaultValue = item.Value 
                                }
                    ).ToArray()
            };

            appManifest.ServiceTemplates = this.Services
                .Select(s => s.GenerateServiceDescription())
                .ToArray();

            appManifest.DefaultServices = new ServiceModel.DefaultServicesType();
            appManifest.DefaultServices.Items = this.Services
                .Select(s =>
                new ServiceModel.DefaultServicesTypeService
                {
                    Name = string.IsNullOrEmpty(s.ServiceName) ? s.ServiceTypeName : s.ServiceName,
                    Item = s.GenerateServiceDescription()
                })
                .ToArray();

            var serviceImports = new ServiceModel.ApplicationManifestTypeServiceManifestImport
            {
                ServiceManifestRef = new ServiceModel.ServiceManifestRefType
                {
                    ServiceManifestName = this.ServiceManifestName,
                    ServiceManifestVersion = this.Version
                }
            };

            appManifest.ServiceManifestImport = new ServiceModel.ApplicationManifestTypeServiceManifestImport[] { serviceImports };

            using (FileStream fs = new FileStream(Path.Combine(serviceManifestFolder, "ServiceManifest.xml"), FileMode.Create))
            {
                XmlWriter xw = ApplicationDeployer.CreateWriter(fs);

                XmlSerializer serializer = new XmlSerializer(typeof(ServiceModel.ServiceManifestType));
                serializer.Serialize(xw, serviceManifest);
                
                xw.Close();
            }

            using (FileStream fs = new FileStream(Path.Combine(deploymentRootPath, "ApplicationManifest.xml"), FileMode.Create))
            {
                XmlWriter xw = ApplicationDeployer.CreateWriter(fs);

                XmlSerializer serializer = new XmlSerializer(typeof(ServiceModel.ApplicationManifestType));
                serializer.Serialize(xw, appManifest);

                xw.Close();
            }
        }

        private static XmlWriter CreateWriter(Stream s)
        {
            XmlWriterSettings settings = new XmlWriterSettings()
            {
                Indent = true,
            };

            XmlWriter xw = XmlTextWriter.Create(s, settings);
            return xw;
        }
    }
}