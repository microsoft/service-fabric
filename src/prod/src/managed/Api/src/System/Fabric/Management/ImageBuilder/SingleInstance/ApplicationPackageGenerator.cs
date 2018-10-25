// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using DockerCompose;
    using Globalization;
    using System.Collections.Generic;
    using System.Linq;
    using System.Fabric.Management.ServiceModel;

    internal class ApplicationPackageGenerator
    {
        public Application application;
        public string applicationTypeName;
        public string applicationTypeVersion;
        public bool generateDnsNames;
        public bool useOpenNetworkConfig;
        GenerationConfig generationConfig;

        const string DefaultReplicatorEndpoint = "ReplicatorEndpoint";

        public ApplicationPackageGenerator(
            string applicationTypeName,
            string applicationTypeVersion,
            Application application,
            bool useOpenNetworkConfig,
            bool generateDnsNames = false,
            GenerationConfig generationConfig = null)
        {
            this.applicationTypeName = applicationTypeName;
            this.applicationTypeVersion = applicationTypeVersion;
            this.application = application;
            this.generateDnsNames = generateDnsNames;
            this.generationConfig = generationConfig ?? new GenerationConfig();
            this.useOpenNetworkConfig = useOpenNetworkConfig;
        }

        public void Generate(
            out ApplicationManifestType applicationManifest,
            out IList<ServiceManifestType> serviceManifests)
        {
            this.CreateManifests(out applicationManifest, out serviceManifests);
        }

        private void CreateManifests(
            out ApplicationManifestType applicationManifest,
            out IList<ServiceManifestType> serviceManifests)
        {
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
            }

            var index = 0;
            serviceManifests = new List<ServiceManifestType>();
            foreach (var service in this.application.services)
            {
                string serviceName = service.name;

                ServiceManifestType serviceManifest;
                DefaultServicesTypeService defaultServices;
                var serviceManifestImport = this.GetServiceManifestImport(
                    applicationManifest,
                    serviceName,
                    service,
                    out defaultServices,
                    out serviceManifest);

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
            out ServiceManifestType serviceManifest)
        {
            if (service == null)
            {
                throw new FabricApplicationException(String.Format("Service description is not specified"));
            }

            Dictionary<string, List<PortBindingType>> portBindingList;
            serviceManifest = this.CreateServiceManifest(
                serviceName,
                service,
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

#if DotNetCoreClrLinux
            // include ServicePackageContainerPolicy if isolationlevel is set to hyperv.
            if (generationConfig.IsolationLevel == "hyperv")
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

            // Container Host policies
            var containerHostPolicies = this.GetContainerHostPolicies(
                service,
                serviceManifest,
                portBindingList);
            policyCount += containerHostPolicies.Count;

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

            if (generationConfig.IsolationLevel == "hyperv")
            {
                var servicePackageContainerPolicy = new ServicePackageContainerPolicyType();
                servicePackageContainerPolicy.Isolation = generationConfig.IsolationLevel;
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

            defaultServiceEntry = this.GetDefaultServiceTypeEntry(
                serviceName,
                service,
                String.Format(CultureInfo.InvariantCulture, "{0}Type", serviceName));

            return serviceManifestImport;
        }

        private ServiceManifestType CreateServiceManifest(
            string serviceName,
            Service service,
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

                serviceManifestType.CodePackage[index] = this.GetCodePackage(codePackage, codePackage.imageRegistryCredential);

                var endpointResources = this.GetEndpointResources(service, codePackage, portBindings);
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

        private List<ContainerHostPoliciesType> GetContainerHostPolicies(
            Service service,
            ServiceManifestType serviceManifest,
            Dictionary<string, List<PortBindingType>> portBindings)
        {
            List<ContainerHostPoliciesType> containerHostPolicies = new List<ContainerHostPoliciesType>();
            foreach (var codePackage in serviceManifest.CodePackage)
            {
                var containerHostPolicy = new ContainerHostPoliciesType()
                {
                    CodePackageRef = codePackage.Name,
                    ContainersRetentionCount = generationConfig.ContainersRetentionCount
                };

#if !DotNetCoreClrLinux
                //
                // On Linux the isolation policy and the portbindings are given at service package level when isolation level is hyperv.
                // TODO: Windows behavior should change to match this once hosting changes are made to make this consistent
                // across windows and linux. Bug# 12703576.
                //
                if (generationConfig.IsolationLevel != null)
                {
                    containerHostPolicy.Isolation = generationConfig.IsolationLevel;
                }
#endif

                // This is not code package specific - for CG, all code package share the same network configuration.
                var containerNetworkTypes = this.GetContainerNetworkTypes();

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

                if (containerNetworkTypes.Count != 0)
                {
                    containerHostPolicyCount += containerNetworkTypes.Count;
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
                if (generationConfig.IsolationLevel != "hyperv")
                {

# endif
                if (portBindings.ContainsKey(codePackage.Name) &&
                    portBindings[codePackage.Name].Count != 0)
                {
                    containerHostPolicyCount += portBindings[codePackage.Name].Count;
                }

#if DotNetCoreClrLinux
                }
# endif
                if (containerHostPolicyCount != 0)
                {
                    containerHostPolicy.Items = new object[containerHostPolicyCount];
                    int nextIndex = 0;
                    if (containerNetworkTypes.Count != 0)
                    {
                        containerNetworkTypes.ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                        nextIndex += containerNetworkTypes.Count;
                    }

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
                        var repositoryCredentialsType = new RepositoryCredentialsType()
                        {
                            AccountName = codePackageInput.imageRegistryCredential.username,
                            Password = codePackageInput.imageRegistryCredential.password,
                            PasswordEncrypted = false,
                            PasswordEncryptedSpecified = false
                        };

                        containerHostPolicy.Items[nextIndex] = repositoryCredentialsType;
                        ++nextIndex;
                    }

#if DotNetCoreClrLinux
                    //
                    // On Linux the isolation policy and the portbindings are given at service package level for isolation level hyperv.
                    // TODO: Windows behavior should change to match this once hosting changes are made to make this consistent
                    // across windows and linux. Bug# 12703576.
                    //

                    if (generationConfig.IsolationLevel != "hyperv")
                    {

#endif
                    if (portBindings.ContainsKey(codePackage.Name) &&
                        portBindings[codePackage.Name].Count != 0)
                    {
                        portBindings[codePackage.Name].ToArray().CopyTo(containerHostPolicy.Items, nextIndex);
                        nextIndex += portBindings[codePackage.Name].Count;
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
                    CpuCores = totalLimitCPU.ToString()
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
                var endpoint = new EndpointType()
                {
                    Name = codePackageEndpoint.name,
                    CodePackageRef = codePackage.name,
                    Port = service.UsesReliableCollection() ? 0 : codePackageEndpoint.port,
                    PortSpecified = !service.UsesReliableCollection(),
                    // RDBug 12797515:Allow user to specify Protocol to be used for Service Enpoints
                    // Current workaround is to always use "http" protocol for stateful services.
                    UriScheme = service.UsesReliableCollection() ? "http" : ""
                };

                if (!this.useOpenNetworkConfig)
                {
                    var portBinding = new PortBindingType()
                    {
                        ContainerPort = codePackageEndpoint.port,
                        EndpointRef = endpoint.Name
                    };
                    portBindings[codePackage.name].Add(portBinding);
                }

                endpoints.Add(endpoint);
            }

            if ((service.UsesReliableCollection() || service.UsesReplicatedStoreVolume()) && 
                endpoints.FirstOrDefault(endpoint => endpoint.Name.Equals(DefaultReplicatorEndpoint, StringComparison.CurrentCultureIgnoreCase)) == null)
            {
                // If ReliableCollectionsRef is specified, insert a ReplicatorEndpoint with dynamic host port assignment by default
                endpoints.Add(new EndpointType() 
                {
                    Name = DefaultReplicatorEndpoint,
                    PortSpecified = false,
                    Type = EndpointTypeType.Internal
                });

                if (!this.useOpenNetworkConfig)
                {
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

        private CodePackageType GetCodePackage(
            CodePackage codePackage,
            ImageRegistryCredential registryCredentials)
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

            int count = 1;
            if (codePackage.environmentVariables != null && codePackage.environmentVariables.Count > 0)
            {
                count += codePackage.environmentVariables.Count;
            }

            containerCodePackage.EnvironmentVariables = new EnvironmentVariableType[count];
            containerCodePackage.EnvironmentVariables[0] = new EnvironmentVariableType() { Name = "Fabric_Id", Value = "@PartitionId@"};


            if (codePackage.environmentVariables != null)
            {
                var index = 1;
                foreach (var environmentVariable in codePackage.environmentVariables)
                {
                    var codePackageEnv = new EnvironmentVariableType();

                    codePackageEnv.Name = environmentVariable.name;
                    codePackageEnv.Value = environmentVariable.value;
                    containerCodePackage.EnvironmentVariables[index++] = codePackageEnv;
                }
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
                    Name = index.ToString()
                });
            }

            ServiceTypeNamedPartition partition = new ServiceTypeNamedPartition()
            {
                Partition = partitionList.ToArray()
            };

            //
            // TEMPORARY: This configuration should come from a settings object
            // that is part of the ReliableCollections/BlockStore configuration.
            //
            string minReplicaSetSize = "1";
            string targetReplicaSetSize = "1";
            if (service.UsesReplicatedStoreVolume() ||
                service.UsesReliableCollection())
            {
                minReplicaSetSize = "2";
                targetReplicaSetSize = "3";
            }

            var defaultServiceType = new DefaultServicesTypeService
            {
                Name = serviceName,
                ServiceDnsName = this.GetServiceDnsName(this.application.name, serviceName),
                ServicePackageActivationMode = "ExclusiveProcess",

                //
                // Specifying limits at the service package level is sufficient. Need
                // not specify default load metrics.
                //

                Item = new StatefulServiceType()
                {
                    MinReplicaSetSize = minReplicaSetSize,
                    TargetReplicaSetSize = targetReplicaSetSize,
                    ServiceTypeName = serviceTypeName,
                    NamedPartition = partition,
                    DefaultMoveCost = ServiceTypeDefaultMoveCost.High,
                    DefaultMoveCostSpecified = true,
                    ReplicaRestartWaitDurationSeconds = generationConfig.ReplicaRestartWaitDurationSeconds.ToString(),
                    QuorumLossWaitDurationSeconds = generationConfig.QuorumLossWaitDurationSeconds.ToString()
                },
            };

            return defaultServiceType;
        }

        private List<ContainerNetworkConfigType> GetContainerNetworkTypes()
        {
            var containerNetworkTypes = new List<ContainerNetworkConfigType>();

            // In isolated mode, always use private network.
            //if (!generationConfig.IsolationLevel.Equals("hyperv", StringComparison.CurrentCultureIgnoreCase) && this.useOpenNetworkConfig)

            // Otherwise, check useOpenNetworkConfig to use NAT(scalemin) vs openNetwork
            if (this.useOpenNetworkConfig)
            {
                var containerNetworkType = new ContainerNetworkConfigType()
                {
                    NetworkType = DockerComposeConstants.OpenExternalNetworkServiceManifestValue
                };

                containerNetworkTypes.Add(containerNetworkType);
            }

            return containerNetworkTypes;
        }

        private ServiceTypeType GetContainerServiceType(string namePrefix, Service service)
        {
            return new StatefulServiceTypeType()
            {
                ServiceTypeName = String.Format(CultureInfo.InvariantCulture, "{0}Type", namePrefix),
                UseImplicitHost = !service.UsesReliableCollection(), // Stateful service should have UseImplicitHost=false
                HasPersistedState = true,
            };
        }

        private string GetServicePackageName(string serviceName)
        {
            return String.Format(CultureInfo.InvariantCulture, "{0}Pkg", serviceName);
        }

        private string GetMemoryInMBString(double memoryInGB)
        {
            var memoryInMB = Convert.ToInt64(memoryInGB*1024);
            return memoryInMB.ToString();
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

            var containerCPUPercent = Convert.ToUInt32(((containerCpuLimit / totalCPULimit)*100));
            resourceGovernancePolicy.CpuShares = containerCPUPercent.ToString();
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
                var appNameUri = new Uri("fabric:/"+applicationName);
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
    }
}
