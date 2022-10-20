// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using DockerCompose;
    using Globalization;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Fabric;
    using System.Fabric.Management.ServiceModel;

    internal class ApplicationPackageGenerator
    {
        public Application application;
        public string applicationTypeName;
        public string applicationTypeVersion;
        public bool generateDnsNames;
        public bool useOpenNetworkConfig;
        public bool useLocalNatNetworkConfig;
        public string mountPointForSettings;
        GenerationConfig generationConfig;
        private List<CodePackageDebugParameters> debugParameters;

        const string DefaultReplicatorEndpoint = "ReplicatorEndpoint";

        public ApplicationPackageGenerator(
            string applicationTypeName,
            string applicationTypeVersion,
            Application application,
            bool useOpenNetworkConfig,
            bool useLocalNatNetworkConfig,
            string mountPointForSettings,
            bool generateDnsNames = false,
            GenerationConfig generationConfig = null)
        {
            this.applicationTypeName = applicationTypeName;
            this.applicationTypeVersion = applicationTypeVersion;
            this.application = application;
            this.generateDnsNames = generateDnsNames;
            this.generationConfig = generationConfig ?? new GenerationConfig();
            this.useOpenNetworkConfig = useOpenNetworkConfig;
            this.useLocalNatNetworkConfig = useLocalNatNetworkConfig;
            this.mountPointForSettings = mountPointForSettings;
        }

        public void Generate(
            out ApplicationManifestType applicationManifest,
            out IList<ServiceManifestType> serviceManifests,
            out Dictionary<string, Dictionary<string, SettingsType>> settingsType)
        {
            this.CreateManifests(out applicationManifest, out serviceManifests, out settingsType);
        }

        private void CreateManifests(
            out ApplicationManifestType applicationManifest,
            out IList<ServiceManifestType> serviceManifests,
            out Dictionary<string, Dictionary<string, SettingsType>> settingsType)
        {
            settingsType = new Dictionary<string, Dictionary<string, SettingsType>>();

            int serviceInstanceCount = application.services.Count;
            
            applicationManifest = new ApplicationManifestType()
            {
                ApplicationTypeName = this.applicationTypeName,
                ApplicationTypeVersion = this.applicationTypeVersion,
                ServiceManifestImport =
                    new ApplicationManifestTypeServiceManifestImport[serviceInstanceCount],
                DefaultServices = new DefaultServicesType()
                {
                    Items = new object[serviceInstanceCount]
                }
            };

            if (!string.IsNullOrEmpty(application.debugParams))
            {
                applicationManifest.Parameters = new ApplicationManifestTypeParameter[]
                {
                    new ApplicationManifestTypeParameter()
                    {
                        Name = "_WFDebugParams_",
                        DefaultValue = application.debugParams
                    }
                };

                debugParameters = CodePackageDebugParameters.CreateFrom(application.debugParams);
            }

            var index = 0;
            serviceManifests = new List<ServiceManifestType>();

            foreach (var service in this.application.services)
            {
                string serviceName = service.name;
                service.dnsName = this.GetServiceDnsName(this.application.name, serviceName);

                ServiceManifestType serviceManifest;
                DefaultServicesTypeService defaultServices;
                var serviceManifestImport = this.GetServiceManifestImport(
                    applicationManifest,
                    serviceName,
                    service,
                    out defaultServices,
                    out serviceManifest,
                    settingsType);

                applicationManifest.ServiceManifestImport[index] = serviceManifestImport;
                applicationManifest.DefaultServices.Items[index] = defaultServices;

                serviceManifests.Add(serviceManifest);
                ++index;
            }
        }

        private ApplicationManifestTypeServiceManifestImport GetServiceManifestImport(
            ApplicationManifestType applicationManifestType,
            string serviceName,
            Service service,
            out DefaultServicesTypeService defaultServiceEntry,
            out ServiceManifestType serviceManifest,
            Dictionary<string, Dictionary<string, SettingsType>> settingsTypeDictionary)
        {
            if (service == null)
            {
                throw new FabricApplicationException(String.Format("Service description is not specified"));
            }

            if (this.debugParameters != null)
            {
                foreach (var codePackage in service.codePackages)
                {
                    string codePackageName = codePackage.name;
                    CodePackageDebugParameters serviceDebugParams = this.debugParameters.Find(param => codePackageName.Equals(param.CodePackageName, StringComparison.InvariantCultureIgnoreCase));
                    service.SetDebugParameters(serviceDebugParams);
                }
            }
            bool isNATBindingUsed = IsNATBindingUsed(service);
            Dictionary<string, List<PortBindingType>> portBindingList;
            serviceManifest = this.CreateServiceManifest(
                serviceName,
                service,
                isNATBindingUsed,
                out portBindingList);

            var serviceManifestImport = new ApplicationManifestTypeServiceManifestImport()
            {
                ServiceManifestRef = new ServiceManifestRefType()
                {
                    ServiceManifestName = serviceManifest.Name,
                    ServiceManifestVersion = serviceManifest.Version
                },
            };

            // Includes SF Runtime access policy.
            int policyCount = 1;
            string containerIsolationLevel = GetContainerIsolationLevel(service);
#if DotNetCoreClrLinux
            // include ServicePackageContainerPolicy if isolationlevel is set to hyperv.
            if (containerIsolationLevel == "hyperv")
            {
                ++policyCount;
            }
#endif
            // Resource governance policies
            List<ResourceGovernancePolicyType> codepackageResourceGovernancePolicies;
            ServicePackageResourceGovernancePolicyType servicePackageResourceGovernancePolicy;
            this.GetResourceGovernancePolicies(
                service,
                out codepackageResourceGovernancePolicies,
                out servicePackageResourceGovernancePolicy);
            policyCount += codepackageResourceGovernancePolicies.Count;
            if (servicePackageResourceGovernancePolicy != null)
            {
                ++policyCount;
            }

            // Network Policies
            var networkPolicies = this.GetNetworkPolicies(
                service,
                serviceManifest,
                isNATBindingUsed
            );
            policyCount += networkPolicies.Count;

            // Container Host policies
            var containerHostPolicies = this.GetContainerHostPolicies(
                service,
                serviceManifest,
                portBindingList,
                isNATBindingUsed);
            policyCount += containerHostPolicies.Count;

            Dictionary<string, SettingsType> settingsTypesInService = new Dictionary<string, SettingsType>();
            // Config Package Policies
            List<ConfigPackagePoliciesType> configPackagePolicies = this.GetConfigPackagePolicies(
                service,
                serviceManifest,
                settingsTypesInService);
            if (settingsTypesInService.Count() > 0)
            {
                // Add ConfigPackage in ServiceManifest
                serviceManifest.ConfigPackage = new ConfigPackageType[settingsTypesInService.Count()];
                int configPackageCount = 0;
                foreach (var settingsType in settingsTypesInService)
                {
                    serviceManifest.ConfigPackage[configPackageCount++] = new ConfigPackageType()
                    {
                        Name = settingsType.Key,
                        Version = this.applicationTypeVersion
                    };
                }
                // Add settings to Dictionary
                settingsTypeDictionary.Add(serviceManifest.Name, settingsTypesInService);
            }

            policyCount += configPackagePolicies.Count;

            serviceManifestImport.Policies = new object[policyCount];

            int index = 0;
            serviceManifestImport.Policies[index] = new ServiceFabricRuntimeAccessPolicyType()
            {
                RemoveServiceFabricRuntimeAccess = (service.UsesReliableCollection()) ? false : this.generationConfig.RemoveServiceFabricRuntimeAccess,
                UseServiceFabricReplicatedStore = service.UsesReplicatedStoreVolume()
            };
            ++index;

#if DotNetCoreClrLinux
            //
            // On Linux the isolation policy and the portbindings are given at service package level when isolation level is hyperv.
            // TODO: Windows behavior and behavior for process isolation should change to match this once hosting changes are made to make this consistent
            // across windows and linux. Bug# 12703576.
            //

            if (containerIsolationLevel == "hyperv")
            {
                var servicePackageContainerPolicy = new ServicePackageContainerPolicyType();
                servicePackageContainerPolicy.Isolation = containerIsolationLevel;
                servicePackageContainerPolicy.Hostname = "ccvm1";

                int portBindingsCount = 0;
                foreach (var cpPortBindingList in portBindingList.Values)
                {
                    portBindingsCount += cpPortBindingList.Count;
                }

                if (portBindingsCount > 0)
                {
                    servicePackageContainerPolicy.Items = new PortBindingType[portBindingsCount];
                    int nextIndex = 0;
                    foreach (var cpPortBindingList in portBindingList.Values)
                    {
                        cpPortBindingList.ToArray().CopyTo(servicePackageContainerPolicy.Items, nextIndex);
                        nextIndex += cpPortBindingList.Count;
                    }
                }

                serviceManifestImport.Policies[index] = servicePackageContainerPolicy;
                ++index;
            }
#endif

            if (codepackageResourceGovernancePolicies.Count > 0)
            {
                codepackageResourceGovernancePolicies.ToArray().CopyTo(serviceManifestImport.Policies, index);
                index += codepackageResourceGovernancePolicies.Count;
            }

            if (servicePackageResourceGovernancePolicy != null)
            {
                serviceManifestImport.Policies[index] = servicePackageResourceGovernancePolicy;
                ++index;
            }

            if (containerHostPolicies.Count > 0)
            {
                containerHostPolicies.ToArray().CopyTo(serviceManifestImport.Policies, index);
                index += containerHostPolicies.Count;
            }

            if (networkPolicies.Count > 0)
            {
                networkPolicies.ToArray().CopyTo(serviceManifestImport.Policies, index);
                index += networkPolicies.Count;
            }

            if (configPackagePolicies.Count > 0)
            {
                configPackagePolicies.ToArray().CopyTo(serviceManifestImport.Policies, index);
                index += configPackagePolicies.Count;
            }

            defaultServiceEntry = this.GetDefaultServiceTypeEntry(
                serviceName,
                service,
                String.Format(CultureInfo.InvariantCulture, "{0}Type", serviceName));

            return serviceManifestImport;
        }

        private ServiceManifestType CreateServiceManifest(
            string serviceName,
            Service service,
            bool isNATBindingUsed,
            out Dictionary<string, List<PortBindingType>> portBindings)
        {
            var serviceManifestType = new ServiceManifestType()
            {
                Name = this.GetServicePackageName(serviceName),
                Version = this.applicationTypeVersion
            };

            serviceManifestType.ServiceTypes = new object[1];
            serviceManifestType.ServiceTypes[0] = this.GetContainerServiceType(serviceName, service);

            if (service.codePackages == null || service.codePackages.Count == 0)
            {
                throw new FabricApplicationException(string.Format("Code package can not be empty"));
            }

            // CodePackage
            serviceManifestType.CodePackage = new CodePackageType[service.codePackages.Count];

            portBindings = new Dictionary<string, List<PortBindingType>>();
            var endpointResourceList = new List<EndpointType>();

            int index = 0;
            foreach (var codePackage in service.codePackages)
            {
                string codePackageName = codePackage.name;

                codePackage.Validate();

                if (service.UsesReliableCollection())
                {
                    codePackage.environmentVariables.Add(new EnvironmentVariable()
                    {
                        name = "Fabric_UseReliableCollectionReplicationMode",
                        value = "true"
                    });
                }

                serviceManifestType.CodePackage[index] = this.GetCodePackage(service, codePackage);

                var endpointResources = this.GetEndpointResources(service, codePackage, isNATBindingUsed, portBindings);
                endpointResourceList.AddRange(endpointResources);

                ++index;
            }

            if (endpointResourceList.Count() > 0)
            {
                serviceManifestType.Resources = new ResourcesType
                {
                    Endpoints = endpointResourceList.ToArray()
                };
            }

            return serviceManifestType;
        }

        private bool IsNATBindingUsed(Service service)
        {
            if (service.networkRefs != null)
            {
                foreach (var networkRef in service.networkRefs)
                {
                    var name = networkRef.name;

                    // For the local one box scenario, open is not supported so we convert this to nat for testing.
                    if (!this.useOpenNetworkConfig)
                    {
                        if (name.Equals(StringConstants.OpenNetworkName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            name = StringConstants.NatNetworkName;
                        }
                    }
                    if (name.Equals(StringConstants.NatNetworkName, StringComparison.InvariantCultureIgnoreCase))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        private List<NetworkPoliciesType> GetNetworkPolicies(Service service, ServiceManifestType serviceManifest, bool isNATBindingUsed)
        {
            List<NetworkPoliciesType> networkPoliciesList = new List<NetworkPoliciesType>();
            List<ContainerNetworkPolicyType> containerNetworkPolicies = new List<ContainerNetworkPolicyType>();

            if (service.networkRefs != null)
            {
                foreach (var networkRef in service.networkRefs)
                {
                    var name = networkRef.name;

#if DotNetCoreClrLinux
                    // Isolated networks are not supported for reliable collections for mesh in release 6.4 on Linux Cluster.
                    // For reliable collections in mesh, assert that reliable service is using Open Network / NAT only.
                    if (service.UsesReliableCollection() || service.UsesReplicatedStoreVolume())
                    {
                        if (!name.Equals(StringConstants.OpenNetworkName, StringComparison.InvariantCultureIgnoreCase) && !name.Equals(StringConstants.NatNetworkName, StringComparison.InvariantCultureIgnoreCase))
                            throw new FabricComposeException("Local network is not supported for reliable collections and volumes for mesh on Linux Clusters.");
                    }
#endif

                    // For the local one box scenario, open is not supported so we convert this to nat for testing.
                    if (!this.useOpenNetworkConfig)
                    {
                        if (name.Equals(StringConstants.OpenNetworkName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            name = StringConstants.NatNetworkName;
                        }
                    }
                    // For the local one box scenario, isolated is converted to open for testing.
                    if (this.useLocalNatNetworkConfig)
                    {
                        if (!name.Equals(StringConstants.OpenNetworkName, StringComparison.InvariantCultureIgnoreCase) &&
                            !name.Equals(StringConstants.NatNetworkName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            name = StringConstants.OpenNetworkName;
                        }
                    }

                    ContainerNetworkPolicyType internalNetworkPolicy = new ContainerNetworkPolicyType()
                    {
                        NetworkRef = name
                    };

                    if (serviceManifest.Resources != null && serviceManifest.Resources.Endpoints != null)
                    {
                        var containerNetworkPolicyEndpointBindingType = new List<ContainerNetworkPolicyEndpointBindingType>();

                        // Use all endpoints specified if no endpointRef is explicitly stated.
                        if (networkRef.endpointRefs == null)
                        {
                            foreach (var currentEndpoint in serviceManifest.Resources.Endpoints)
                            {
                                ContainerNetworkPolicyEndpointBindingType endpointBinding = new ContainerNetworkPolicyEndpointBindingType()
                                {
                                    EndpointRef = currentEndpoint.Name
                                };
                                containerNetworkPolicyEndpointBindingType.Add(endpointBinding);
                            }
                        }
                        else
                        {
                            if (service.UsesReliableCollection())
                            {
                                EndpointRef replicatorEndpointRef = new EndpointRef();
                                replicatorEndpointRef.name = DefaultReplicatorEndpoint;
                                networkRef.endpointRefs.Add(replicatorEndpointRef);
                            }

                            foreach (var currentNetworkEndpoint in networkRef.endpointRefs)
                            {
                                foreach (var currentEndpoint in serviceManifest.Resources.Endpoints)
                                {
                                    if (currentEndpoint.Name == currentNetworkEndpoint.name)
                                    {
                                        ContainerNetworkPolicyEndpointBindingType endpointBinding = new ContainerNetworkPolicyEndpointBindingType()
                                        {
                                            EndpointRef = currentEndpoint.Name
                                        };
                                        containerNetworkPolicyEndpointBindingType.Add(endpointBinding);
                                    }
                                }
                            }
                        }
                        internalNetworkPolicy.Items = containerNetworkPolicyEndpointBindingType.ToArray();
                    }
                    containerNetworkPolicies.Add(internalNetworkPolicy);
                }

                NetworkPoliciesType networkPolicies = new NetworkPoliciesType();
                if (containerNetworkPolicies.Count != 0)
                {
                    networkPolicies.Items = new ContainerNetworkPolicyType[containerNetworkPolicies.Count];
                    containerNetworkPolicies.ToArray().CopyTo(networkPolicies.Items, 0);
                }

                networkPoliciesList.Add(networkPolicies);
            }

            return networkPoliciesList;
        }

        private string GetConfigPackageName(string codePackageName) { return string.Format("{0}Config", codePackageName); }

        private List<ConfigPackagePoliciesType> GetConfigPackagePolicies(
            Service service,
            ServiceManifestType serviceManifest,
            Dictionary<string, SettingsType> settingsTypesInService) // Key is ConfigPackageName
        {
            List<ConfigPackagePoliciesType> configPackagePolicies = new List<ConfigPackagePoliciesType>();
            foreach (SingleInstance.CodePackage codePackage in service.codePackages.Where(c => c.settings.Count != 0))
            {
                var settingsBySection = codePackage.settings.GroupBy(s => s.SectionName);

                var configPackagePoliciesType = new ConfigPackagePoliciesType()
                {
                    CodePackageRef = codePackage.name,
                    ConfigPackage = new ConfigPackageDescriptionType[settingsBySection.Count()]
                };
                configPackagePolicies.Add(configPackagePoliciesType);

                // Setting files and config package are per each code package
                SettingsType settingsType = new SettingsType()
                {
                    Section = new SettingsTypeSection[settingsBySection.Count()]
                };
                settingsTypesInService.Add(GetConfigPackageName(codePackage.name), settingsType);

                int settingsTypeCount = 0;
                foreach (var section in settingsBySection)
                {
                    ConfigPackageDescriptionType configPackage = new ConfigPackageDescriptionType()
                    {
                        Name = GetConfigPackageName(codePackage.name),
                        SectionName = section.Key,
                        MountPoint = section.Key.Equals(Setting.DefaultSectionName) ? this.mountPointForSettings : System.IO.Path.Combine(this.mountPointForSettings, section.Key)
                    };
                    SettingsTypeSection settingSection = new SettingsTypeSection()
                    {
                        Name = section.Key,
                        Parameter = new SettingsTypeSectionParameter[section.Count()]
                    };
                    configPackagePoliciesType.ConfigPackage[settingsTypeCount] = configPackage;
                    settingsType.Section[settingsTypeCount++] = settingSection;

                    int ParameterCount = 0;
                    foreach (Setting setting in section)
                    {
                        SettingsTypeSectionParameter parameter = new SettingsTypeSectionParameter()
                        {
                            Name = setting.ParameterName
                        };

                        if (TryMatchEncryption(ref setting.value))
                        {
                            parameter.IsEncrypted = true;
                            parameter.Type = "Encrypted";
                        }
                        else if (TryMatchSecretRef(ref setting.value))
                        {
                            parameter.Type = "SecretsStoreRef";
                        }
                        
                        parameter.Value = setting.value;

                        settingSection.Parameter[ParameterCount++] = parameter;
                    }
                }
            }

            return configPackagePolicies;
        }

        private List<ContainerHostPoliciesType> GetContainerHostPolicies(
            Service service,
            ServiceManifestType serviceManifest,
            Dictionary<string, List<PortBindingType>> portBindings,
            bool isNATBindingUsed)
        {
            List<ContainerHostPoliciesType> containerHostPolicies = new List<ContainerHostPoliciesType>();
            foreach (var codePackage in serviceManifest.CodePackage)
            {
                var containerHostPolicy = new ContainerHostPoliciesType()
                {
                    CodePackageRef = codePackage.Name,
                    ContainersRetentionCount = generationConfig.ContainersRetentionCount
                };

                //
                // On Linux the isolation policy and the portbindings are given at service package level when isolation level is hyperv.
                // TODO: Windows behavior should change to match this once hosting changes are made to make this consistent
                // across windows and linux. Bug# 12703576.
                //
                string containerIsolationLevel = GetContainerIsolationLevel(service);
                if (containerIsolationLevel != null)
                {
                    containerHostPolicy.Isolation = containerIsolationLevel;
                }

                // Each container in the CG can have different volume mounts and labels.
                var codePackageInput = service.codePackages.Find(elem => codePackage.Name == elem.name);
                var containerVolumeTypes = this.GetContainerVolumeTypes(codePackageInput);
                var containerLabelTypes = this.GetContainerLabelTypes(codePackageInput);

                var diagnosticsLabel = this.GetDiagnosticsContainerLabel(service, codePackageInput);
                if (diagnosticsLabel != null)
                {
                    containerLabelTypes.Add(diagnosticsLabel);
                }

                int containerHostPolicyCount = 0;

                if (codePackageInput.imageRegistryCredential != null &&
                    codePackageInput.imageRegistryCredential.IsCredentialsSpecified())
                {
                    ++containerHostPolicyCount;
                }

                if (containerVolumeTypes.Count != 0)
                {
                    containerHostPolicyCount += containerVolumeTypes.Count;
                }

                if (containerLabelTypes.Count != 0)
                {
                    containerHostPolicyCount += containerLabelTypes.Count;
                }

#if DotNetCoreClrLinux
                //
                // On Linux the isolation policy and the portbindings are given at service package level when isolation level is hyperv.
                // TODO: Windows behavior should change to match this once hosting changes are made to make this consistent
                // across windows and linux. Bug# 12703576.
                //
                if (containerIsolationLevel != "hyperv")
                {

#endif
                if (isNATBindingUsed)
                {
                    if (portBindings.ContainsKey(codePackage.Name) &&
                        portBindings[codePackage.Name].Count != 0)
                    {
                        containerHostPolicyCount += portBindings[codePackage.Name].Count;
                    }
                }

#if DotNetCoreClrLinux
                }
#endif
                if (containerHostPolicyCount != 0)
                {
                    containerHostPolicy.Items = new object[containerHostPolicyCount];
                    int nextIndex = 0;

                    if (containerVolumeTypes.Count != 0)
                    {
                        containerVolumeTypes.ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                        nextIndex += containerVolumeTypes.Count;
                    }

                    if (containerLabelTypes.Count != 0)
                    {
                        containerLabelTypes.ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                        nextIndex += containerLabelTypes.Count;
                    }

                    if (codePackageInput.imageRegistryCredential != null &&
                        codePackageInput.imageRegistryCredential.IsCredentialsSpecified())
                    {
                        string password = codePackageInput.imageRegistryCredential.password;
                        var repositoryCredentialsType = new RepositoryCredentialsType()
                        {
                            AccountName = codePackageInput.imageRegistryCredential.username,
                            PasswordEncrypted = false,
                            PasswordEncryptedSpecified = false
                        };

                        if (TryMatchEncryption(ref password))
                        {
                            repositoryCredentialsType.Type = "Encrypted";
                        }
                        else if (TryMatchSecretRef(ref password))
                        {
                            repositoryCredentialsType.Type = "SecretsStoreRef";
                        }

                        repositoryCredentialsType.Password = password;

                        containerHostPolicy.Items[nextIndex] = repositoryCredentialsType;
                        ++nextIndex;
                    }

#if DotNetCoreClrLinux
                    //
                    // On Linux the isolation policy and the portbindings are given at service package level for isolation level hyperv.
                    // TODO: Windows behavior should change to match this once hosting changes are made to make this consistent
                    // across windows and linux. Bug# 12703576.
                    //

                    if (containerIsolationLevel != "hyperv")
                    {

#endif
                    if (isNATBindingUsed)
                    {
                        if (portBindings.ContainsKey(codePackage.Name) &&
                        portBindings[codePackage.Name].Count != 0)
                        {
                            portBindings[codePackage.Name].ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                            nextIndex += portBindings[codePackage.Name].Count;
                        }
                    }

#if DotNetCoreClrLinux
                    }

#endif
                }

                containerHostPolicies.Add(containerHostPolicy);
            }

            return containerHostPolicies;
        }
        
        private void GetResourceGovernancePolicies(
            Service service,
            out List<ResourceGovernancePolicyType> codepackageResourceGovernancePolicies,
            out ServicePackageResourceGovernancePolicyType servicePackageResourceGovernancePolicy)
        {
            codepackageResourceGovernancePolicies = new List<ResourceGovernancePolicyType>();
            servicePackageResourceGovernancePolicy = null;

            //
            // As of 6.1, PLB treats reservations as limits. So if limits are specified, we use limits as the cap,
            // if not, we set the cap to reservation. This needs to change when PLB supports 'true' reservations.
            //
            double totalLimitCPU = 0;
            foreach (var codePackage in service.codePackages)
            {
                if (codePackage.resources.limits != null && codePackage.resources.limits.cpu != 0)
                {
                    totalLimitCPU += codePackage.resources.limits.cpu;
                }
                else
                {
                    totalLimitCPU += codePackage.resources.requests.cpu;
                }
            }

            if (totalLimitCPU != 0)
            {
                servicePackageResourceGovernancePolicy = new ServicePackageResourceGovernancePolicyType()
                {
                    CpuCores = ImageBuilderUtility.ConvertToString(totalLimitCPU)
                };
            }

            foreach (var codePackage in service.codePackages)
            {
                string codePackageName = codePackage.name;

                this.ValidateResources(codePackageName, codePackage.resources);

                ResourceGovernancePolicyType resourceGovernancePolicy = new ResourceGovernancePolicyType()
                {
                    CodePackageRef = codePackageName
                };

#if DotNetCoreClrLinux
                resourceGovernancePolicy.MemoryReservationInMB = this.GetMemoryInMBString(codePackage.resources.requests.memoryInGB);
#endif
                if (codePackage.resources.limits != null && codePackage.resources.limits.memoryInGB != 0)
                {
                    resourceGovernancePolicy.MemoryInMB = this.GetMemoryInMBString(codePackage.resources.limits.memoryInGB);
                }
                else
                {
                    resourceGovernancePolicy.MemoryInMB = this.GetMemoryInMBString(codePackage.resources.requests.memoryInGB);
                }

                this.UpdateCPUShares(codePackage, resourceGovernancePolicy, totalLimitCPU);

                codepackageResourceGovernancePolicies.Add(resourceGovernancePolicy);
            }
        }

        private IEnumerable<EndpointType> GetEndpointResources(
            Service service,
            CodePackage codePackage,
            bool isNATBindingUsed,
            Dictionary<string, List<PortBindingType>> portBindings)
        {
            var endpoints = new List<EndpointType>();

            portBindings[codePackage.name] = new List<PortBindingType>();

            //
            // If network type is 'open', we just publish all the ports that is specified for the individual containers.
            //
            foreach (var codePackageEndpoint in codePackage.endpoints)
            {
                // TODO: what is port is not specified?, private network should be treated same as open.
                
                // 1. Always use specified port for services not using reliable collections
                // 2. Use specied port for services using reliable collections only if not using NAT
                bool useSpecifiedPort = !service.UsesReliableCollection() || !isNATBindingUsed;
                var endpoint = new EndpointType()
                {
                    Name = codePackageEndpoint.name,
                    CodePackageRef = codePackage.name,
                    Port = useSpecifiedPort ? codePackageEndpoint.port : 0,
                    UriScheme = service.GetUriScheme(),
                    PortSpecified = useSpecifiedPort 
                };

                var portBinding = new PortBindingType()
                {
                    ContainerPort = codePackageEndpoint.port,
                    EndpointRef = endpoint.Name
                };
                portBindings[codePackage.name].Add(portBinding);

                endpoints.Add(endpoint);
            }

            if ((service.UsesReliableCollection() || service.UsesReplicatedStoreVolume()) &&
                endpoints.FirstOrDefault(endpoint => endpoint.Name.Equals(DefaultReplicatorEndpoint, StringComparison.CurrentCultureIgnoreCase)) == null)
            {
                // If ReliableCollectionsRef is specified, insert a ReplicatorEndpoint with dynamic host port assignment by default
                var endpoint = new EndpointType()
                {
                    Name = DefaultReplicatorEndpoint,
                    PortSpecified = false,
                    Type = EndpointTypeType.Internal
                };

                if (!isNATBindingUsed)
                {
                    endpoint.CodePackageRef = codePackage.name;
                }

                endpoints.Add(endpoint);

                //Adding a port binding for the ReplicatorEndpoint would result in passing it via "-p" to docker when activating the container,
                //resulting in docker attempting to use it, which would conflict with the Replicator already listening on that port.
                if (!service.UsesReplicatedStoreVolume())
                {
                    portBindings[codePackage.name].Add(new PortBindingType()
                    {
                        ContainerPort = 0,
                        EndpointRef = DefaultReplicatorEndpoint,
                    });
                }
            }

            return endpoints;
        }

        private List<ContainerVolumeType> GetContainerVolumeTypes(
            CodePackage codePackageInput)
        {
            var containerVolumeTypes = new List<ContainerVolumeType>();

            var volumeMounts = Enumerable.Concat<VolumeMount>(codePackageInput.volumeRefs, codePackageInput.volumes);
            foreach (var volumeMount in volumeMounts)
            {
                // Description is not provided if JSON file is used directly from SFCompose.exe
                if (volumeMount.VolumeDesc == null) { continue; }

                var containerVolumeType = new ContainerVolumeType
                {
                    Driver = volumeMount.VolumeDesc.PluginName,
                    Source = volumeMount.Name,
                    Destination = volumeMount.DestinationPath,
                    IsReadOnly = volumeMount.ReadOnly
                };

                containerVolumeType.Items = volumeMount.VolumeDesc.DriverOptions.ToArray();

                containerVolumeTypes.Add(containerVolumeType);
            }

            return containerVolumeTypes;
        }

        private List<ContainerLabelType> GetContainerLabelTypes(
            CodePackage codePackageInput)
        {
            var containerLabelTypes = new List<ContainerLabelType>();

            if (codePackageInput.labels == null)
            {
                return containerLabelTypes;
            }

            foreach (var label in codePackageInput.labels)
            {
                var containerLabelType = new ContainerLabelType
                {
                    Name = label.Name,
                    Value = label.Value,
                };

                containerLabelTypes.Add(containerLabelType);
            }

            return containerLabelTypes;
        }

        private ContainerLabelType GetDiagnosticsContainerLabel(Service service, CodePackage codePackageInput)
        {
            if (codePackageInput.diagnostics != null)
            {
                if (codePackageInput.diagnostics.enabled == false)
                {
                    return null;
                }

                if (codePackageInput.diagnostics.sinkRefs != null && codePackageInput.diagnostics.sinkRefs.Any())
                {
                    return this.GetDiagnosticsContainerLabelHelper(this.application.diagnostics, codePackageInput.diagnostics.sinkRefs);
                }
                else if (service.diagnostics != null && service.diagnostics.sinkRefs != null && service.diagnostics.sinkRefs.Any())
                {
                    return this.GetDiagnosticsContainerLabelHelper(this.application.diagnostics, service.diagnostics.sinkRefs);
                }
                else if (this.application.diagnostics != null && this.application.diagnostics.defaultSinkRefs != null && this.application.diagnostics.defaultSinkRefs.Any())
                {
                    return this.GetDiagnosticsContainerLabelHelper(this.application.diagnostics, this.application.diagnostics.defaultSinkRefs);
                }
                else
                {
                    throw new FabricComposeException("No default sink references specified but required.");
                }
            }
            else if (service.diagnostics != null)
            {
                if (service.diagnostics.enabled == false)
                {
                    return null;
                }

                if (service.diagnostics != null && service.diagnostics.sinkRefs != null && service.diagnostics.sinkRefs.Any())
                {
                    return this.GetDiagnosticsContainerLabelHelper(this.application.diagnostics, service.diagnostics.sinkRefs);
                }
                else if (this.application.diagnostics != null && this.application.diagnostics.defaultSinkRefs != null && this.application.diagnostics.defaultSinkRefs.Any())
                {
                    return this.GetDiagnosticsContainerLabelHelper(this.application.diagnostics, this.application.diagnostics.defaultSinkRefs);
                }
                else
                {
                    throw new FabricComposeException("No default sink references specified but required.");
                }
            }
            else if (this.application.diagnostics != null)
            {
                if (this.application.diagnostics.enabled == false)
                {
                    return null;
                }

                if (this.application.diagnostics != null && this.application.diagnostics.defaultSinkRefs != null && this.application.diagnostics.defaultSinkRefs.Any())
                {
                    return this.GetDiagnosticsContainerLabelHelper(this.application.diagnostics, this.application.diagnostics.defaultSinkRefs);
                }
                else
                {
                    throw new FabricComposeException("No default sink references specified but required.");
                }
            }

            return null;
        }

        private ContainerLabelType GetDiagnosticsContainerLabelHelper(DiagnosticsDescription diagnostics, List<string> references)
        {
            if (diagnostics == null)
            {
                throw new FabricComposeException("No diagnostics defined.");
            }

            if (references != null && references.Any())
            {
                var diagnosticsUsed = new List<DiagnosticsSinkBase>();
                var definedSinks = new Dictionary<string, DiagnosticsSinkBase>();

                foreach (var sink in diagnostics.sinks)
                {
                    try
                    {
                        definedSinks.Add(sink.name, sink);
                    }
                    catch (Exception e)
                    {
                        throw new FabricComposeException("Error with diagnostic sinks definition", e);
                    }
                }

                foreach (var reference in references)
                {
                    if (definedSinks.ContainsKey(reference))
                    {
                        diagnosticsUsed.Add(definedSinks[reference]);
                    }
                    else
                    {
                        throw new FabricComposeException(String.Format("No diagnostic sink with name {0} found.", reference));
                    }
                }

                Newtonsoft.Json.Linq.JArray sinksArray = new Newtonsoft.Json.Linq.JArray();
                foreach (var diagnostic in diagnosticsUsed)
                {
                    var diagnosticWrapper = new Newtonsoft.Json.Linq.JObject();
                    diagnosticWrapper.Add("kind", diagnostic.kind);
                    diagnosticWrapper.Add("properties", Newtonsoft.Json.JsonConvert.SerializeObject(diagnostic));
                    sinksArray.Add(diagnosticWrapper);
                }

                var diagnosticsUsedWrapper = new Newtonsoft.Json.Linq.JObject();
                diagnosticsUsedWrapper.Add("diagnostics", sinksArray);

                string value = Newtonsoft.Json.JsonConvert.SerializeObject(diagnosticsUsedWrapper);
                string key = "diagnosticsinfo";

                return new ContainerLabelType() { Name = key, Value = value };
            }
            else
            {
                throw new FabricComposeException("No sink references provided for diagnostics defined.");
            }
        }

        // Expected encryption string: [encrypted(ENCRYPTEDSTRING)]
        private bool TryMatchEncryption(ref string value)
        {
            Match match = Regex.Match(value, @"\[encrypted\((.+)\)\]");
            if (match.Success)
            {
                value = match.Groups[1].Value;
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Check if the value is following secret reference format (case-insensitive).
        /// Expected secret reference string: [reference('secrets/{secretName}/values/{secretVersion}')]
        /// The format follows the SFRP/ARM format.
        /// </summary>
        /// <param name="value">The value to check on. 
        /// If function returns true, the value will be replaced with the cluster resource reference ({secretName}:{secretValue}) to secret.</param>
        /// <returns>True if the value string follows the secret reference format. Otherwise, false.</returns>
        internal static bool TryMatchSecretRef(ref string value)
        {
            var match = Regex.Match(
                value,
                @"\[reference\(\'secrets\/(?<secretName>[a-zA-Z0-9_\-]+)\/values\/(?<secretVersion>[a-zA-Z0-9_\-\.]+)\'\)\]",
                RegexOptions.IgnoreCase);

            if (match.Success)
            {
                value = string.Format("{0}:{1}", match.Groups["secretName"].Value, match.Groups["secretVersion"].Value);
                return true;
            }
            else
            {
                return false;
            }
        }

        private CodePackageType GetCodePackage(
            Service service,
            CodePackage codePackage)
        {
            var containerCodePackage = new CodePackageType()
            {
                Name = codePackage.name,
                Version = this.applicationTypeVersion,
                EntryPoint = new EntryPointDescriptionType()
                {
                    Item = this.GetContainerHostEntryPointType(codePackage)
                }
            };

            if (codePackage.settings.Count > 0)
            {
                // Reserved environment variable for setting path
                codePackage.environmentVariables.Add(new EnvironmentVariable()
                {
                    name = "Fabric_SettingPath",
                    value = this.mountPointForSettings
                });
            }

            //
            // Account for the environment variables that we set here.
            //
            int count = 5;
            if (codePackage.environmentVariables.Count > 0)
            {
                count += codePackage.environmentVariables.Count;
            }

            containerCodePackage.EnvironmentVariables = new EnvironmentVariableType[count];
            containerCodePackage.EnvironmentVariables[0] = new EnvironmentVariableType() { Name = "Fabric_Id", Value = "@PartitionId@" };
            containerCodePackage.EnvironmentVariables[1] = new EnvironmentVariableType() { Name = "Fabric_ServiceName", Value = service.name };
            containerCodePackage.EnvironmentVariables[2] = new EnvironmentVariableType() { Name = "Fabric_ApplicationName", Value = this.application.name };
            containerCodePackage.EnvironmentVariables[3] = new EnvironmentVariableType() { Name = "Fabric_CodePackageName", Value = codePackage.name };
            containerCodePackage.EnvironmentVariables[4] = new EnvironmentVariableType() { Name = "Fabric_ServiceDnsName", Value = service.dnsName };

            //
            // Account for the environment variables that we set here.
            //
            var index = 5;
            foreach (var environmentVariable in codePackage.environmentVariables)
            {
                var codePackageEnv = new EnvironmentVariableType()
                {
                    Name = environmentVariable.name
                };

                // The input would be changed as ref here.
                if (TryMatchEncryption(ref environmentVariable.value))
                {
                    codePackageEnv.Type = EnvironmentVariableTypeType.Encrypted;
                }
                // TODO: #11905876 Integrate querying SecretStoreService in Hosting
                // TODO [dragosav]: look into aligning the type of the variable/setting type - env vars
                // use an enum (EnvVarTypeType) whereas settings use a string value.
                else if (TryMatchSecretRef(ref environmentVariable.value))
                {
                    codePackageEnv.Type = EnvironmentVariableTypeType.SecretsStoreRef;
                }

                codePackageEnv.Value = environmentVariable.value;
                containerCodePackage.EnvironmentVariables[index++] = codePackageEnv;
            }

            return containerCodePackage;
        }

        private ContainerHostEntryPointType GetContainerHostEntryPointType(
            CodePackage codePackage)
        {
            var imageName = codePackage.image;

            var registryCredential = codePackage.imageRegistryCredential;

            if (registryCredential != null &&
                registryCredential.IsServerNameSpecified() &&
                !imageName.StartsWith(registryCredential.server))
            {
                imageName = String.Format(CultureInfo.InvariantCulture, "{0}/{1}", registryCredential.server, imageName);
            }

            var entryPointType = new ContainerHostEntryPointType()
            {
                ImageName = imageName,
            };

            if (!String.IsNullOrEmpty(codePackage.entryPoint))
            {
                entryPointType.EntryPoint = codePackage.entryPoint;
            }

            string command = null;
            foreach (string cmd in codePackage.commands)
            {
                if (command != null)
                {
                    command = command + ",";
                }
                command = command + cmd;
            }

            if (command != null)
            {
                entryPointType.Commands = command;
            }

            return entryPointType;
        }

        private DefaultServicesTypeService GetDefaultServiceTypeEntry(
            string serviceName,
            Service service,
            string serviceTypeName)
        {
            var defaultServiceTypeEntries = new List<DefaultServicesTypeService>();

            List<ServiceTypeNamedPartitionPartition> partitionList = new List<ServiceTypeNamedPartitionPartition>();
            for (int index = 0; index < service.replicaCount; index++)
            {
                // Partition name is generated
                partitionList.Add(new ServiceTypeNamedPartitionPartition()
                {
                    Name = ImageBuilderUtility.ConvertToString(index)
                });
            }

            List<ScalingPolicyType> scalingPolicies = new List<ScalingPolicyType>();
            if (service.autoScalingPolicies != null) {
                foreach (var autoScalingPolicy in service.autoScalingPolicies)
                {
                    string metricName = null;
                    string lowerLoadThreshold = null;
                    string upperLoadThreshold = null;
                    if (String.Equals(autoScalingPolicy.trigger.metric.name, "cpu", StringComparison.OrdinalIgnoreCase)
                        && (  String.Equals(autoScalingPolicy.trigger.metric.kind, "Resource", StringComparison.OrdinalIgnoreCase)
                            || String.Equals(autoScalingPolicy.trigger.metric.kind, "0", StringComparison.OrdinalIgnoreCase)))
                    {
                        metricName = "servicefabric:/_CpuCores";
                        lowerLoadThreshold = autoScalingPolicy.trigger.lowerLoadThreshold;
                        upperLoadThreshold = autoScalingPolicy.trigger.upperLoadThreshold;
                    }
                    else if (String.Equals(autoScalingPolicy.trigger.metric.name, "memoryInGB", StringComparison.OrdinalIgnoreCase)
                        && (   String.Equals(autoScalingPolicy.trigger.metric.kind, "Resource", StringComparison.OrdinalIgnoreCase)
                            || String.Equals(autoScalingPolicy.trigger.metric.kind, "0", StringComparison.OrdinalIgnoreCase)))
                    {
                        metricName = "servicefabric:/_MemoryInMB";

                        double threshold = 0;
                        if(Double.TryParse(autoScalingPolicy.trigger.lowerLoadThreshold, out threshold))
                        {
                            threshold *= 1024;
                        }
                        lowerLoadThreshold = ImageBuilderUtility.ConvertToString(threshold);

                        threshold = 0;
                        if (Double.TryParse(autoScalingPolicy.trigger.upperLoadThreshold, out threshold))
                        {
                            threshold *= 1024;
                        }
                        upperLoadThreshold = ImageBuilderUtility.ConvertToString(threshold);
                    }
                    else
                    {
                        //currently only those 2 metrics are allowed for autoscaling
                        continue;
                    }
                    scalingPolicies.Add(new ScalingPolicyType()
                    {
                        AverageServiceLoadScalingTrigger = new ScalingPolicyTypeAverageServiceLoadScalingTrigger()
                        {
                            MetricName = metricName,
                            LowerLoadThreshold = lowerLoadThreshold,
                            UpperLoadThreshold = upperLoadThreshold,
                            ScaleIntervalInSeconds = autoScalingPolicy.trigger.scaleIntervalInSeconds,
                            UseOnlyPrimaryLoad = "false"
                        },

                        AddRemoveIncrementalNamedPartitionScalingMechanism = new ScalingPolicyTypeAddRemoveIncrementalNamedPartitionScalingMechanism()
                        {
                            MinPartitionCount = autoScalingPolicy.mechanism.minCount,
                            MaxPartitionCount = autoScalingPolicy.mechanism.maxCount,
                            ScaleIncrement = autoScalingPolicy.mechanism.scaleIncrement
                        }
                    });
                }
            }
            ServiceTypeNamedPartition partition = new ServiceTypeNamedPartition()
            {
                Partition = partitionList.ToArray()
            };

            //
            // TEMPORARY: This configuration should come from a settings object
            // that is part of the ReliableCollections/BlockStore configuration.
            //
            // TargetReplicaSetSize of FM is used to decide the replica count of the application.
            // TargetReplicaSetSize of FM is 1 for 1 node cluster and non 1 value for a multi node cluster.
            //

            string minReplicaSetSize = "1";
            string targetReplicaSetSize = "1";
            if (generationConfig.TargetReplicaCount != 1 && 
               (service.UsesReplicatedStoreVolume() || service.UsesReliableCollection()))
            {
                minReplicaSetSize = "2";
                targetReplicaSetSize = "3";
            }

            StatefulServiceType type;
            if(scalingPolicies.Count != 0)
            {
                type = new StatefulServiceType()
                {
                    MinReplicaSetSize = minReplicaSetSize,
                    TargetReplicaSetSize = targetReplicaSetSize,
                    ServiceTypeName = serviceTypeName,
                    NamedPartition = partition,
                    DefaultMoveCost = ServiceTypeDefaultMoveCost.High,
                    DefaultMoveCostSpecified = true,
                    ReplicaRestartWaitDurationSeconds = ImageBuilderUtility.ConvertToString(generationConfig.ReplicaRestartWaitDurationSeconds),
                    QuorumLossWaitDurationSeconds = ImageBuilderUtility.ConvertToString(generationConfig.QuorumLossWaitDurationSeconds),
                    ServiceScalingPolicies = scalingPolicies.ToArray()
                };
            }
            else
            {
                type = new StatefulServiceType()
                {
                    MinReplicaSetSize = minReplicaSetSize,
                    TargetReplicaSetSize = targetReplicaSetSize,
                    ServiceTypeName = serviceTypeName,
                    NamedPartition = partition,
                    DefaultMoveCost = ServiceTypeDefaultMoveCost.High,
                    DefaultMoveCostSpecified = true,
                    ReplicaRestartWaitDurationSeconds = ImageBuilderUtility.ConvertToString(generationConfig.ReplicaRestartWaitDurationSeconds),
                    QuorumLossWaitDurationSeconds = ImageBuilderUtility.ConvertToString(generationConfig.QuorumLossWaitDurationSeconds)
                };
            }
            var defaultServiceType = new DefaultServicesTypeService
            {
                Name = serviceName,
                ServiceDnsName = service.dnsName,
                ServicePackageActivationMode = "ExclusiveProcess",

                //
                // Specifying limits at the service package level is sufficient. Need
                // not specify default load metrics.
                //

                Item = type,
            };

            return defaultServiceType;
        }

        private ServiceTypeType GetContainerServiceType(string namePrefix, Service service)
        {
            return new StatefulServiceTypeType()
            {
                ServiceTypeName = String.Format(CultureInfo.InvariantCulture, "{0}Type", namePrefix),
                UseImplicitHost = !service.UsesReliableCollection(), // Stateful service should have UseImplicitHost=false
                HasPersistedState = service.HasPersistedState()
            };
        }

        private string GetServicePackageName(string serviceName)
        {
            return String.Format(CultureInfo.InvariantCulture, "{0}Pkg", serviceName);
        }

        private string GetMemoryInMBString(double memoryInGB)
        {
            var memoryInMB = Convert.ToInt64(memoryInGB * 1024);
            return ImageBuilderUtility.ConvertToString(memoryInMB);
        }

        private void UpdateCPUShares(
            CodePackage codePackage,
            ResourceGovernancePolicyType resourceGovernancePolicy,
            double totalCPULimit)
        {
            double containerCpuLimit = 0;
            if (totalCPULimit == 0)
            {
                return;
            }

            if (codePackage.resources.limits == null ||
                codePackage.resources.limits.cpu == 0)
            {
                containerCpuLimit = codePackage.resources.requests.cpu;
            }
            else
            {
                containerCpuLimit = codePackage.resources.limits.cpu;
            }

            var containerCPUPercent = Convert.ToUInt32(((containerCpuLimit / totalCPULimit) * 100));
            resourceGovernancePolicy.CpuShares = ImageBuilderUtility.ConvertToString(containerCPUPercent);
        }

        private string GetServiceDnsName(string applicationName, string serviceName)
        {
            if (!this.generateDnsNames)
            {
                return "";
            }

            var serviceDnsName = serviceName;

            //
            // If the dns service suffix is not already a fully qualified domain name, create the name under application's domain to avoid
            // dns name conflicts with services in other applications.
            //
            if (!serviceName.Contains(".") && !String.IsNullOrEmpty(applicationName))
            {
                //
                // Each of the path segment in the application name becomes a domain label in the service Dns Name, With the first path segment
                // becoming the top level domain label.
                //
                var appNameUri = new Uri("fabric:/" + applicationName);
                var appNameSegments = appNameUri.AbsolutePath.Split('/');
                for (int i = appNameSegments.Length - 1; i >= 0; --i)
                {
                    // Each segment max length is 63
                    if (appNameSegments[i].Length > 63)
                    {
                        throw new FabricComposeException(
                            String.Format(
                                "Service DNS name cannot be generated. Segment '{0}' of application name '{1}' exceeds max domain label size.",
                                appNameSegments[i],
                                appNameUri));
                    }

                    if (!String.IsNullOrEmpty(appNameSegments[i]))
                    {
                        serviceDnsName += "." + appNameSegments[i];
                    }
                }
            }

            //
            // Total length of the domain name cannot be > 255 characters - RFC 1035
            //
            if (serviceDnsName.Length > 255)
            {
                throw new FabricComposeException(
                    String.Format(
                        "Service DNS name cannot be generated. The dns name '{0}' exceeds max dns name length.",
                        serviceDnsName));
            }

            return serviceDnsName;
        }

        private void ValidateResources(string codePackageName, Resources resources)
        {
            if (resources.requests == null ||
                resources.requests.cpu == 0 ||
                resources.requests.memoryInGB == 0)
            {
                throw new FabricApplicationException(String.Format("Resource requests not specified for code package {0}", codePackageName));
            }
        }

        private string GetContainerIsolationLevel(Service service)
        {
#if DotNetCoreClrLinux
            //  RDBug 14037954:[Mesh][Linux] Stateful services are not supported with Clear/Kata containers
            //  Isolation mode "hyperv" is not supported today on linux as we do not have support for Clear/Kata containers.
            if (service.UsesReliableCollection() || service.UsesReplicatedStoreVolume())
            {
                return "process";
            }
#endif
            return generationConfig.IsolationLevel;
        }
    }
}
