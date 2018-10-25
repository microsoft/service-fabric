// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using System;

    internal class ContainerPackageGenerator
    {
        private readonly ComposeApplicationTypeDescription composeApplicationTypeDescription;
        private readonly ContainerRepositoryCredentials repositoryCredentials;
        private bool generateDnsNames;
        private string applicationName;

        public ContainerPackageGenerator(
            ComposeApplicationTypeDescription applicationTypeDescription,
            ContainerRepositoryCredentials repositoryCredentials,
            string applicationName,
            bool generateDnsNames)
        {
            this.composeApplicationTypeDescription = applicationTypeDescription;
            this.repositoryCredentials = repositoryCredentials;
            this.generateDnsNames = generateDnsNames;
            this.applicationName = applicationName;
        }

        public void Generate(
            out ApplicationManifestType applicationManifest,
            out IList<ServiceManifestType> serviceManifests)
        {
            this.CreateManifests(out applicationManifest, out serviceManifests);
        }

        private void CreateManifests(out ApplicationManifestType applicationManifest,
            out IList<ServiceManifestType> serviceManifests)
        {
            applicationManifest = new ApplicationManifestType()
            {
                ApplicationTypeName = this.composeApplicationTypeDescription.ApplicationTypeName,
                ApplicationTypeVersion = this.composeApplicationTypeDescription.ApplicationTypeVersion,
                ServiceManifestImport =
                    new ApplicationManifestTypeServiceManifestImport[
                        this.composeApplicationTypeDescription.ServiceTypeDescriptions.Count],
                DefaultServices = new DefaultServicesType()
                {
                    Items = new object[this.composeApplicationTypeDescription.ServiceTypeDescriptions.Count]
                }
            };

            var index = 0;
            serviceManifests = new List<ServiceManifestType>();
            foreach (var servicetype in this.composeApplicationTypeDescription.ServiceTypeDescriptions)
            {
                ServiceManifestType serviceManifest;
                DefaultServicesTypeService defaultService;
                var serviceManifestImport = this.GetServiceManifestImport(
                    applicationManifest,
                    servicetype.Key,
                    servicetype.Value,
                    out defaultService,
                    out serviceManifest);

                applicationManifest.ServiceManifestImport[index] = serviceManifestImport;
                applicationManifest.DefaultServices.Items[index] = defaultService;

                serviceManifests.Add(serviceManifest);
                ++index;
            }
        }

        private ApplicationManifestTypeServiceManifestImport GetServiceManifestImport(
            ApplicationManifestType applicationManifestType,
            string serviceNamePrefix,
            ComposeServiceTypeDescription serviceTypeDescription,
            out DefaultServicesTypeService defaultServiceEntry,
            out ServiceManifestType serviceManifest)
        {
            IList<PortBindingType> portBindingList;
            serviceManifest = this.CreateServiceManifest(
                serviceNamePrefix,
                applicationManifestType.ApplicationTypeVersion,
                serviceTypeDescription,
                out portBindingList);

            var serviceManifestImport = new ApplicationManifestTypeServiceManifestImport()
            {
                ServiceManifestRef = new ServiceManifestRefType()
                {
                    ServiceManifestName = serviceManifest.Name,
                    ServiceManifestVersion = serviceManifest.Version
                },
            };

            // Environment variables
            if (serviceTypeDescription.EnvironmentVariables.Count > 0)
            {
                serviceManifestImport.EnvironmentOverrides = new EnvironmentOverridesType[1];

                // one codepackage                
                serviceManifestImport.EnvironmentOverrides[0] = this.GetEnvironmentOverrides(serviceTypeDescription,
                    serviceManifest);
            }

            var servicePackageResourceGovernance = this.GetServicePackageResourceGovernancePolicy(serviceTypeDescription);
            var resourceGovernancePolicies = this.GetResourceGovernancePolicies(serviceTypeDescription, serviceManifest);

            // Resource governance policy for CP, policy for SP and ContainerHost policy
            serviceManifestImport.Policies = new object[resourceGovernancePolicies.Count + 2];

            int index = 0;
            if (resourceGovernancePolicies.Count != 0)
            {
                resourceGovernancePolicies.ToArray().CopyTo(serviceManifestImport.Policies, index);
                index += resourceGovernancePolicies.Count;
            }

            // Resource governance on SP level
            serviceManifestImport.Policies[index] = servicePackageResourceGovernance;
            ++index;

            // Container Host policies
            serviceManifestImport.Policies[index] = this.GetContainerHostPolicy(serviceTypeDescription, serviceManifest,
                portBindingList);

            defaultServiceEntry = this.GetDefaultServiceTypeEntry(serviceNamePrefix, serviceTypeDescription);

            return serviceManifestImport;
        }

        private EnvironmentOverridesType GetEnvironmentOverrides(
            ComposeServiceTypeDescription containerServiceTypeDescription, ServiceManifestType serviceManifest)
        {
            var envOverrides = new EnvironmentOverridesType
            {
                CodePackageRef = serviceManifest.CodePackage[0].Name,
                EnvironmentVariable =
                    new EnvironmentVariableOverrideType[containerServiceTypeDescription.EnvironmentVariables.Count]
            };
            var index = 0;
            foreach (var environment in containerServiceTypeDescription.EnvironmentVariables)
            {
                var envVariableType = new EnvironmentVariableOverrideType
                {
                    Name = environment.Key,
                    Value = environment.Value
                };

                envOverrides.EnvironmentVariable[index] = envVariableType;
                ++index;
            }

            return envOverrides;
        }

        private ContainerHostPoliciesType GetContainerHostPolicy(
            ComposeServiceTypeDescription containerServiceTypeDescription,
            ServiceManifestType serviceManifest,
            IList<PortBindingType> portBindingList)
        {
            var containerHostPolicy = new ContainerHostPoliciesType()
            {
                CodePackageRef = serviceManifest.CodePackage[0].Name,
            };

            if (!string.IsNullOrEmpty(containerServiceTypeDescription.Isolation))
            {
                containerHostPolicy.Isolation = containerServiceTypeDescription.Isolation;
            }

            var containerVolumeTypes = this.GetContainerVolumeTypes(containerServiceTypeDescription);
            var containerNetworkTypes = this.GetContainerNetworkTypes(containerServiceTypeDescription);

            var containerHostPolicies = 0;
            if (this.repositoryCredentials.IsCredentialsSpecified())
            {
                containerHostPolicies = 1;
            }

            if (containerServiceTypeDescription.LoggingOptions.IsSpecified())
            {
                ++containerHostPolicies;
            }

            containerHostPolicies += portBindingList.Count + containerVolumeTypes.Count + containerNetworkTypes.Count;
            if (containerHostPolicies != 0)
            {
                containerHostPolicy.Items = new object[containerHostPolicies];
                var nextIndex = 0;
                if (this.repositoryCredentials.IsCredentialsSpecified())
                {
                    var repositoryCredentialsType = new RepositoryCredentialsType()
                    {
                        AccountName = this.repositoryCredentials.RepositoryUserName,
                        Password = this.repositoryCredentials.RepositoryPassword,
                        PasswordEncrypted = this.repositoryCredentials.IsPasswordEncrypted,
                        PasswordEncryptedSpecified = this.repositoryCredentials.IsPasswordEncrypted
                    };

                    containerHostPolicy.Items[0] = repositoryCredentialsType;
                    ++nextIndex;
                }

                if (containerServiceTypeDescription.LoggingOptions.IsSpecified())
                {
                    var composeLoggingOptions = containerServiceTypeDescription.LoggingOptions;
                    var loggingOptions = new ContainerLoggingDriverType()
                    {
                        Driver = composeLoggingOptions.DriverName,
                    };

                    if (composeLoggingOptions.DriverOptions.Count != 0)
                    {
                        var driverOptions = new List<DriverOptionType>();
                        foreach (var item in composeLoggingOptions.DriverOptions)
                        {
                            var driverOption = new DriverOptionType()
                            {
                                Name = item.Key,
                                Value = item.Value
                            };
                            driverOptions.Add(driverOption);
                        }
                        loggingOptions.Items = new DriverOptionType[composeLoggingOptions.DriverOptions.Count];
                        driverOptions.ToArray().CopyTo(loggingOptions.Items, 0);
                    }

                    containerHostPolicy.Items[nextIndex] = loggingOptions;
                    ++nextIndex;
                }

                if (portBindingList.Count != 0)
                {
                    portBindingList.ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                    nextIndex += portBindingList.Count;
                }

                if (containerVolumeTypes.Count != 0)
                {
                    containerVolumeTypes.ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                    nextIndex += containerVolumeTypes.Count;
                }

                if (containerNetworkTypes.Count != 0)
                {
                    containerNetworkTypes.ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                    nextIndex += containerNetworkTypes.Count;
                }
            }

            return containerHostPolicy;
        }

        private DefaultServicesTypeService GetDefaultServiceTypeEntry(
            string namePrefix,
            ComposeServiceTypeDescription serviceTypeDescription)
        {
            //
            // In order to avoid dns name collisions between services of 2 different applications, service dns names be subdomains
            // under the application name.
            //
            var defaultServiceType = new DefaultServicesTypeService
            {
                Name = namePrefix,
                ServiceDnsName = this.generateDnsNames ? DockerComposeUtils.GetServiceDnsName(this.applicationName, namePrefix) : "",
                ServicePackageActivationMode = "ExclusiveProcess",
                Item = new StatelessServiceType()
                {
                    InstanceCount = serviceTypeDescription.InstanceCount.ToString(),
                    ServiceTypeName = DockerComposeUtils.GetServiceTypeName(namePrefix),
                    SingletonPartition = new ServiceTypeSingletonPartition(),
                },
            };

            if (serviceTypeDescription.PlacementConstraints.Count != 0)
            {
                defaultServiceType.Item.PlacementConstraints =
                    $"({this.MergePlacementConstraints(serviceTypeDescription.PlacementConstraints)})";
            }

            return defaultServiceType;
        }

        private List<ResourceGovernancePolicyType> GetResourceGovernancePolicies(
            ComposeServiceTypeDescription containerServiceTypeDescription,
            ServiceManifestType serviceManifest)
        {
            List<ResourceGovernancePolicyType> resourceGovernancePolicies = new List<ResourceGovernancePolicyType>();
            if (containerServiceTypeDescription.ResourceGovernance != null &&
                containerServiceTypeDescription.ResourceGovernance.IsSpecified())
            {
                // There is a single code package in the container service type.
                ResourceGovernancePolicyType resourceGovernancePolicy = new ResourceGovernancePolicyType();
                var containerResourceGovernance = containerServiceTypeDescription.ResourceGovernance;
                resourceGovernancePolicy.CodePackageRef = serviceManifest.CodePackage[0].Name;
                if (containerResourceGovernance.LimitCpuShares.Length != 0)
                {
                    resourceGovernancePolicy.CpuShares = containerResourceGovernance.LimitCpuShares;
                }

                if (containerResourceGovernance.LimitMemoryInMb.Length != 0)
                {
                    resourceGovernancePolicy.MemoryInMB = containerResourceGovernance.LimitMemoryInMb;
                }

                if (containerResourceGovernance.LimitMemorySwapInMb.Length != 0)
                {
                    resourceGovernancePolicy.MemorySwapInMB = containerResourceGovernance.LimitMemorySwapInMb;
                }

                if (containerResourceGovernance.ReservationMemoryInMb.Length != 0)
                {
                    resourceGovernancePolicy.MemoryReservationInMB = containerResourceGovernance.ReservationMemoryInMb;
                }

                resourceGovernancePolicies.Add(resourceGovernancePolicy);
            }

            return resourceGovernancePolicies;
        }

        private ServicePackageResourceGovernancePolicyType GetServicePackageResourceGovernancePolicy(
            ComposeServiceTypeDescription containerServiceTypeDescription)
        {
            ServicePackageResourceGovernancePolicyType servicePackageResourceGovernance = new ServicePackageResourceGovernancePolicyType();
            if (containerServiceTypeDescription.ResourceGovernance != null &&
                containerServiceTypeDescription.ResourceGovernance.IsSpecified())
            {
                var containerResourceGovernance = containerServiceTypeDescription.ResourceGovernance;
                if (containerResourceGovernance.LimitCpus.Length != 0)
                {
                    servicePackageResourceGovernance.CpuCores = containerResourceGovernance.LimitCpus;
                }
            }
            return servicePackageResourceGovernance;
        }

        private List<ContainerNetworkConfigType> GetContainerNetworkTypes(
            ComposeServiceTypeDescription containerServiceTypeDescription)
        {
            var containerNetworkTypes = new List<ContainerNetworkConfigType>();
            
            //
            // Process any default networks specified at the application level
            //
            foreach (var network in this.composeApplicationTypeDescription.NetworkDescriptions)
            {
                if (!network.Value.IsDefaultNetwork)
                {
                    continue;
                }
                var containerNetworkType = new ContainerNetworkConfigType();
                if (network.Value.ExternalNetworkName == DockerComposeConstants.OpenExternalNetworkValue)
                {
                    containerNetworkType.NetworkType = DockerComposeConstants.OpenExternalNetworkServiceManifestValue;
                }
                else
                {
                    // default is NAT.
                    containerNetworkType.NetworkType = DockerComposeConstants.NatExternalNetworkServiceManifestValue;
                }

                containerNetworkTypes.Add(containerNetworkType);
            }

            return containerNetworkTypes;
        }

        private List<ContainerVolumeType> GetContainerVolumeTypes(
            ComposeServiceTypeDescription containerServiceTypeDescription)
        {
            var containerVolumeTypes = new List<ContainerVolumeType>();
            foreach (var volumeMapping in containerServiceTypeDescription.VolumeMappings)
            {
                var containerVolumeType = new ContainerVolumeType
                {
                    Source = volumeMapping.HostLocation,
                    Destination = volumeMapping.ContainerLocation,
                    IsReadOnly = volumeMapping.ReadOnly
                };

                if (this.composeApplicationTypeDescription.VolumeDescriptions.ContainsKey(volumeMapping.HostLocation))
                {
                    var volumeDriver =
                        this.composeApplicationTypeDescription.VolumeDescriptions[volumeMapping.HostLocation];

                    containerVolumeType.Driver = volumeDriver.DriverName;

                    if (volumeDriver.DriverOptions.Count != 0)
                    {
                        var driverOptions = new List<DriverOptionType>();
                        foreach (var item in volumeDriver.DriverOptions)
                        {
                            var driverOption = new DriverOptionType
                            {
                                Name = item.Key,
                                Value = item.Value
                            };
                            driverOptions.Add(driverOption);
                        }

                        containerVolumeType.Items = new DriverOptionType[driverOptions.Count];
                        driverOptions.ToArray().CopyTo(containerVolumeType.Items, 0);
                    }
                }
                containerVolumeTypes.Add(containerVolumeType);
            }

            return containerVolumeTypes;
        }

        private ServiceManifestType CreateServiceManifest(
            string namePrefix,
            string version,
            ComposeServiceTypeDescription serviceTypeDescription, 
            out IList<PortBindingType> portBindings)
        {
            var serviceManifestType = new ServiceManifestType()
            {
                Name = DockerComposeUtils.GetServicePackageName(namePrefix),
                Version = version
            };

            // Service Type
            serviceManifestType.ServiceTypes = new object[1];
            serviceManifestType.ServiceTypes[0] = this.GetContainerServiceType(namePrefix);

            // CodePackage
            serviceManifestType.CodePackage = new CodePackageType[1];
            serviceManifestType.CodePackage[0] = this.GetCodePackage(namePrefix, serviceTypeDescription);

            var endpointResources = this.GetEndpointResources(namePrefix, serviceTypeDescription, out portBindings);
            if (endpointResources.Count() > 0)
            {
                serviceManifestType.Resources = new ResourcesType
                {
                    Endpoints = endpointResources.ToArray()
                };
            }

            return serviceManifestType;
        }

        private ServiceTypeType GetContainerServiceType(string namePrefix)
        {
            return new StatelessServiceTypeType()
            {
                ServiceTypeName = DockerComposeUtils.GetServiceTypeName(namePrefix),
                UseImplicitHost = true
            };
        }

        private IEnumerable<EndpointType> GetEndpointResources(string namePrefix,
            ComposeServiceTypeDescription serviceTypeDescription, out IList<PortBindingType> portBindings)
        {
            var endpoints = new List<EndpointType>();
            portBindings = new List<PortBindingType>();

            var i = 0;
            foreach (var portMapping in serviceTypeDescription.Ports)
            {
                var endpoint = new EndpointType()
                {
                    Name = DockerComposeUtils.GetEndpointName(namePrefix, i),
                };

                if (!string.IsNullOrEmpty(portMapping.HostPort))
                {
                    endpoint.Port = Int32.Parse(portMapping.HostPort);
                    endpoint.PortSpecified = true;
                }

                endpoint.Protocol = EndpointTypeProtocol.tcp;
                if (portMapping.Protocol != null && portMapping.Protocol.StartsWith(DockerComposeConstants.HttpProtocol))
                {
                    endpoint.UriScheme = portMapping.Protocol;
                }

                if (!string.IsNullOrEmpty(portMapping.ContainerPort))
                {
                    var portBinding = new PortBindingType()
                    {
                        ContainerPort = Int32.Parse(portMapping.ContainerPort),
                        EndpointRef = endpoint.Name
                    };
                    portBindings.Add(portBinding);
                }

                endpoints.Add(endpoint);
                ++i;
            }

            return endpoints;
        }

        private CodePackageType GetCodePackage(string namePrefix, ComposeServiceTypeDescription serviceTypeDescription)
        {
            var containerCodePackage = new CodePackageType()
            {
                Name = DockerComposeUtils.GetCodePackageName(namePrefix),
                Version = serviceTypeDescription.TypeVersion,
                EnvironmentVariables = new EnvironmentVariableType[serviceTypeDescription.EnvironmentVariables.Count],
                EntryPoint = new EntryPointDescriptionType()
                {
                    Item = this.GetContainerHostEntryPointType(serviceTypeDescription)
                }
            };

            var index = 0;
            foreach (var environmentVariable in serviceTypeDescription.EnvironmentVariables)
            {
                var codePackageEnv = new EnvironmentVariableType();

                codePackageEnv.Name = environmentVariable.Key;
                codePackageEnv.Value = string.Empty;
                containerCodePackage.EnvironmentVariables[index] = codePackageEnv;
                ++index;
            }

            return containerCodePackage;
        }

        private ContainerHostEntryPointType GetContainerHostEntryPointType(
            ComposeServiceTypeDescription serviceTypeDescription)
        {
            var entryPointType = new ContainerHostEntryPointType()
            {
                ImageName = serviceTypeDescription.ImageName,
            };

            if (serviceTypeDescription.EntryPointOverride != null)
            {
                entryPointType.Commands = serviceTypeDescription.EntryPointOverride;
            }

            if (serviceTypeDescription.Commands != null)
            {
                if (entryPointType.Commands != null)
                {
                    entryPointType.Commands = entryPointType.Commands + ", ";
                }
                entryPointType.Commands = entryPointType.Commands + serviceTypeDescription.Commands;
            }

            return entryPointType;
        }

        private string MergePlacementConstraints(IList<string> placementConstraints)
        {
            string mergedValue = "";
            foreach (var item in placementConstraints)
            {
                if (mergedValue.Length != 0)
                {
                    mergedValue += @"&&";
                }
                mergedValue += $"({item})";
            }

            return mergedValue;
        }
    }
}