// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;

    internal class ApplicationInstanceBuilder
    {
        private static readonly string TraceType = "ApplicationInstanceBuilder";
        public static readonly string DebugParameter = "_WFDebugParams_";

        private ApplicationTypeContext applicationTypeContext;
        private IDictionary<string, string> parameters;
        List<CodePackageDebugParameters> debugParameters;
        private ImageStoreWrapper ImageStoreWrapper;
        TimeoutHelper timeoutHelper;

        public ApplicationInstanceBuilder(ApplicationTypeContext applicationTypeContext, IDictionary<string, string> parameters, ImageStoreWrapper imageStoreWrapper, TimeoutHelper timeoutHelper )
        {
            this.applicationTypeContext = applicationTypeContext;
            this.parameters = parameters;
            if (this.parameters.ContainsKey(ApplicationInstanceBuilder.DebugParameter))
            {
                debugParameters = CodePackageDebugParameters.CreateFrom(this.parameters[ApplicationInstanceBuilder.DebugParameter]);
            }
            this.ImageStoreWrapper = imageStoreWrapper;
            this.timeoutHelper = timeoutHelper;
        }

        public ApplicationPackageType BuildApplicationPackage(string applicationId, Uri nameUri, string rolloutVersion)
        {
            ApplicationManifestType applicationManifestType = this.applicationTypeContext.ApplicationManifest;

            ApplicationPackageType applicationPackageType = new ApplicationPackageType();
            applicationPackageType.ApplicationTypeName = applicationManifestType.ApplicationTypeName;            
            applicationPackageType.NameUri = nameUri.OriginalString;
            applicationPackageType.ApplicationId = applicationId;

            applicationPackageType.DigestedEnvironment = new EnvironmentType();            

            if (applicationManifestType.Policies != null)
            {
                applicationPackageType.DigestedEnvironment.Policies = applicationManifestType.Policies;
            }
            else
            {
                applicationPackageType.DigestedEnvironment.Policies = new ApplicationPoliciesType();
            }

            if (applicationManifestType.Principals != null)
            {
                applicationPackageType.DigestedEnvironment.Principals = applicationManifestType.Principals;
            }
            else
            {
                applicationPackageType.DigestedEnvironment.Principals = new SecurityPrincipalsType();
            }

            if (applicationManifestType.Diagnostics != null)
            {
                applicationPackageType.DigestedEnvironment.Diagnostics = applicationManifestType.Diagnostics;
            }
            else
            {
                applicationPackageType.DigestedEnvironment.Diagnostics = new DiagnosticsType();
            }
            
            applicationPackageType.DigestedCertificates = new ApplicationPackageTypeDigestedCertificates();
            if (applicationManifestType.Certificates != null)
            {
                applicationPackageType.DigestedCertificates.SecretsCertificate = applicationManifestType.Certificates.SecretsCertificate;
                applicationPackageType.DigestedCertificates.EndpointCertificate =
                    applicationManifestType.Certificates.EndpointCertificate;
            }

            this.ApplyParameters(applicationPackageType.DigestedEnvironment);
            this.ApplyParameters(applicationPackageType.DigestedCertificates);

            using (MemoryStream stream = new MemoryStream())
            {
                ImageBuilderUtility.WriteXml<ApplicationPackageType>(stream, applicationPackageType);
                stream.Seek(0, SeekOrigin.Begin);
                applicationPackageType.ContentChecksum = ChecksumUtility.ComputeHash(stream);
            }            

            // The RolloutVersion is applied to the ApplicationPackage after computing the ContentChecksum.
            // This is done so that if a ApplicationPackage is created with identical data but with different 
            // RolloutVersion (as in the case of upgrade rollback), the ContentChecksum value will be the same.
            // Hosting uses this when upgrading the Application.
            PopulateApplicationPackageWithRolloutVersion(applicationPackageType, rolloutVersion);

            return applicationPackageType;
        }

        public ApplicationInstanceType BuildApplicationInstanceType(string applicationId, Uri nameUri, int applicationInstanceVersion, string applicatioPackageRolloutVersion)
        {
            ApplicationManifestType applicationManifestType = this.applicationTypeContext.ApplicationManifest;

            ApplicationInstanceType applicationInstanceType = new ApplicationInstanceType();
            applicationInstanceType.ApplicationId = applicationId;
            applicationInstanceType.ApplicationTypeName = applicationManifestType.ApplicationTypeName;
            applicationInstanceType.ApplicationTypeVersion = applicationManifestType.ApplicationTypeVersion;
            applicationInstanceType.NameUri = nameUri.OriginalString;
            applicationInstanceType.Version = applicationInstanceVersion;

            applicationInstanceType.ApplicationPackageRef = new ApplicationInstanceTypeApplicationPackageRef() { RolloutVersion = applicatioPackageRolloutVersion };

            applicationInstanceType.ServicePackageRef = new ApplicationInstanceTypeServicePackageRef[applicationManifestType.ServiceManifestImport.Length];
            for (int i = 0; i < applicationManifestType.ServiceManifestImport.Length; i++)
            {
                applicationInstanceType.ServicePackageRef[i] = new ApplicationInstanceTypeServicePackageRef();
                applicationInstanceType.ServicePackageRef[i].Name = applicationManifestType.ServiceManifestImport[i].ServiceManifestRef.ServiceManifestName;

                // The default rollout version for ServicePackage is <InstanceVersion>.0. This is so that if a ServiceManifest
                // is removed and later added, we generate unique rollout version for it and not reset the rollout version to 1.0
                applicationInstanceType.ServicePackageRef[i].RolloutVersion = RolloutVersion.CreateRolloutVersion(applicationInstanceVersion).ToString();
            }

            if (applicationManifestType.DefaultServices != null)
            {
                applicationInstanceType.DefaultServices = applicationManifestType.DefaultServices;
            }
            else
            {
                applicationInstanceType.DefaultServices = new DefaultServicesType();
            }

            if (applicationManifestType.ServiceTemplates != null)
            {
                applicationInstanceType.ServiceTemplates = applicationManifestType.ServiceTemplates;
            }
            else
            {
                applicationInstanceType.ServiceTemplates = new ServiceType[0];
            }

            // Apply Parameters
            foreach (ServiceType serviceType in applicationInstanceType.ServiceTemplates)
            {
                this.ApplyParameters(serviceType);
            }

            if (applicationInstanceType.DefaultServices.Items != null)
            {
                foreach (object defaultServiceItem in applicationInstanceType.DefaultServices.Items)
                {
                    if (defaultServiceItem is DefaultServicesTypeService)
                    {
                        var defaultService = (DefaultServicesTypeService)defaultServiceItem;

                        defaultService.ServicePackageActivationMode = this.ApplyParameters(defaultService.ServicePackageActivationMode);

                        if (!IsValidServicePackageActivationMode(defaultService.ServicePackageActivationMode))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                             TraceType,
                             StringResources.ImageBuilderError_InvalidServicePackageActivationModeOverride,
                             "DefaultService",
                             defaultService.Name,
                             defaultService.ServicePackageActivationMode,
                             "SharedProcess/ExclusiveProcess");
                        }

                        // Apply parameterized DNS name
                        if (!String.IsNullOrEmpty(defaultService.ServiceDnsName))
                        {
                            defaultService.ServiceDnsName = this.ApplyParameters(defaultService.ServiceDnsName);
                        }

                        this.ApplyParameters(defaultService.Item);
                    }
                    else if (defaultServiceItem is DefaultServicesTypeServiceGroup)
                    {
                        var defaultServiceGroup = (DefaultServicesTypeServiceGroup)defaultServiceItem;

                        defaultServiceGroup.ServicePackageActivationMode = this.ApplyParameters(defaultServiceGroup.ServicePackageActivationMode);

                        if (!IsValidServicePackageActivationMode(defaultServiceGroup.ServicePackageActivationMode))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                             TraceType,
                             StringResources.ImageBuilderError_InvalidServicePackageActivationModeOverride,
                             "DefaultServiceGroup",
                             defaultServiceGroup.Name,
                             defaultServiceGroup.ServicePackageActivationMode,
                             "SharedProcess/ExclusiveProcess");
                        }

                        this.ApplyParameters(defaultServiceGroup.Item);
                    }
                }
            }

            return applicationInstanceType;
        }

        public Collection<ServicePackageType> BuildServicePackage(string rolloutVersion)
        {
            ApplicationManifestType applicationManifestType = this.applicationTypeContext.ApplicationManifest;

            Collection<ServicePackageType> servicePackageTypes = new Collection<ServicePackageType>();

            string defaultUserRef = null;
            if (applicationManifestType.Policies != null && applicationManifestType.Policies.DefaultRunAsPolicy != null)
            {
                defaultUserRef = applicationManifestType.Policies.DefaultRunAsPolicy.UserRef;
            }

            foreach (ApplicationManifestTypeServiceManifestImport serviceManifestImport in applicationManifestType.ServiceManifestImport)
            {                
                ServiceManifest matchingServiceManifest = this.applicationTypeContext.ServiceManifests.First(
                    serviceManifest => ImageBuilderUtility.Equals(serviceManifest.ServiceManifestType.Name, serviceManifestImport.ServiceManifestRef.ServiceManifestName)
                        && ImageBuilderUtility.Equals(serviceManifest.ServiceManifestType.Version, serviceManifestImport.ServiceManifestRef.ServiceManifestVersion));

                ServiceManifestType matchingServiceManifestType = matchingServiceManifest.ServiceManifestType;
                
                ServicePackageType servicePackage = new ServicePackageType();
                servicePackage.Name = matchingServiceManifestType.Name;
                servicePackage.ManifestVersion = matchingServiceManifestType.Version;                                
                servicePackage.ManifestChecksum = matchingServiceManifest.Checksum;

                servicePackage.Description = matchingServiceManifestType.Description;

                // Build DigestedServiceTypes (ServiceGroupTypes convert service groups to services)
                IEnumerable<ServiceTypeType> serviceTypes = ApplicationInstanceBuilder.DigestServiceTypes(matchingServiceManifestType.ServiceTypes);

                servicePackage.DigestedServiceTypes = new ServicePackageTypeDigestedServiceTypes
                {                    
                    ServiceTypes = serviceTypes.ToArray()
                };

                // Build DigestedCodePackage
                servicePackage.DigestedCodePackage = new ServicePackageTypeDigestedCodePackage[matchingServiceManifest.CodePackages.Count];

                //Extract sharingpolicies which have Scope defined first.
                PackageSharingPolicyType codeScope = null;
                PackageSharingPolicyType configScope = null;
                PackageSharingPolicyType dataScope = null;
                if (serviceManifestImport.Policies != null)
                {
                    codeScope = GetSharingPolicyForScope(PackageSharingPolicyTypeScope.Code, serviceManifestImport.Policies);

                    configScope = GetSharingPolicyForScope(PackageSharingPolicyTypeScope.Config, serviceManifestImport.Policies);

                    dataScope = GetSharingPolicyForScope(PackageSharingPolicyTypeScope.Data, serviceManifestImport.Policies);

                    servicePackage.ServicePackageResourceGovernancePolicy = (ServicePackageResourceGovernancePolicyType)serviceManifestImport.Policies.FirstOrDefault(
                        policy => policy.GetType().Equals(typeof(ServicePackageResourceGovernancePolicyType)));
                    this.ApplyParameters(servicePackage.ServicePackageResourceGovernancePolicy);

                    servicePackage.ServiceFabricRuntimeAccessPolicy = (ServiceFabricRuntimeAccessPolicyType)serviceManifestImport.Policies.FirstOrDefault(
                    policy => policy.GetType().Equals(typeof(ServiceFabricRuntimeAccessPolicyType)));

                    servicePackage.ServicePackageContainerPolicy = (ServicePackageContainerPolicyType)serviceManifestImport.Policies.FirstOrDefault(
                            policy => policy.GetType().Equals(typeof(ServicePackageContainerPolicyType)));
                    if (servicePackage.ServicePackageContainerPolicy != null)
                    {
                        servicePackage.ServicePackageContainerPolicy.Isolation = ApplyParameters(servicePackage.ServicePackageContainerPolicy.Isolation);
                        servicePackage.ServicePackageContainerPolicy.Isolation = ValidateIsolationParameter(servicePackage.ServicePackageContainerPolicy.Isolation);
                    }
                }

                List<CodePackageDebugParameters> serviceDebugParams = null;
                if (this.debugParameters != null)
                {
                    serviceDebugParams = this.debugParameters.FindAll(
                         param => ImageBuilderUtility.Equals(matchingServiceManifest.ServiceManifestType.Name, param.ServiceManifestName));
                }

                for (int i = 0; i < matchingServiceManifest.CodePackages.Count; i++)
                {
                    List<RunAsPolicyType> runasPolicies = new List<RunAsPolicyType>();

                    servicePackage.DigestedCodePackage[i] = new ServicePackageTypeDigestedCodePackage();
                    servicePackage.DigestedCodePackage[i].CodePackage = matchingServiceManifest.CodePackages[i].CodePackageType;
                    servicePackage.DigestedCodePackage[i].ContentChecksum = matchingServiceManifest.CodePackages[i].Checksum;
                    RunAsPolicyType runAsPolicy = null;
                    RunAsPolicyType setuprunAsPolicy = null;
                    ResourceGovernancePolicyType resourceGovernancePolicy = null;
                    if (serviceManifestImport.Policies != null)
                    {
                        runAsPolicy = (RunAsPolicyType)serviceManifestImport.Policies.FirstOrDefault(
                            policy => policy.GetType().Equals(typeof(RunAsPolicyType)) &&
                                ImageBuilderUtility.Equals(((RunAsPolicyType)policy).CodePackageRef, matchingServiceManifest.CodePackages[i].CodePackageType.Name) &&
                                (((RunAsPolicyType)policy).EntryPointType.Equals(RunAsPolicyTypeEntryPointType.Main) ||
                                ((RunAsPolicyType)policy).EntryPointType.Equals(RunAsPolicyTypeEntryPointType.All)));

                        setuprunAsPolicy = (RunAsPolicyType)serviceManifestImport.Policies.FirstOrDefault(
                            policy => policy.GetType().Equals(typeof(RunAsPolicyType)) &&
                                ImageBuilderUtility.Equals(((RunAsPolicyType)policy).CodePackageRef, matchingServiceManifest.CodePackages[i].CodePackageType.Name) &&
                                (((RunAsPolicyType)policy).EntryPointType.Equals(RunAsPolicyTypeEntryPointType.Setup)));

                        if (runAsPolicy != null)
                        {
                            runasPolicies.Add(runAsPolicy);
                        }
                        if (setuprunAsPolicy != null)
                        {
                            runasPolicies.Add(setuprunAsPolicy);
                        }
                       
                        if ( codeScope != null ||
                            this.GetSharingPolicyForPackage(matchingServiceManifest.CodePackages[i].CodePackageType.Name,
                                serviceManifestImport.Policies) != null)
                        {
                            servicePackage.DigestedCodePackage[i].IsShared = true;
                            servicePackage.DigestedCodePackage[i].IsSharedSpecified = true;
                        }
                        ContainerHostPoliciesType containerPolicies = (ContainerHostPoliciesType)serviceManifestImport.Policies.FirstOrDefault(
                                 policy => policy.GetType().Equals(typeof(ContainerHostPoliciesType)) &&
                                     ImageBuilderUtility.Equals(((ContainerHostPoliciesType)policy).CodePackageRef,
                                     matchingServiceManifest.CodePackages[i].CodePackageType.Name));
                        if(containerPolicies != null)
                        {
                            containerPolicies.Isolation = ApplyParameters(containerPolicies.Isolation);
                            containerPolicies.Isolation = ValidateIsolationParameter(containerPolicies.Isolation);

                            containerPolicies.Hostname = this.ApplyParameters(containerPolicies.Hostname);
                            containerPolicies.UseDefaultRepositoryCredentials = this.ApplyParameters(containerPolicies.UseDefaultRepositoryCredentials);
                            bool defaultRepoVal = CheckIsValidBoolean(containerPolicies.UseDefaultRepositoryCredentials);

                            containerPolicies.UseTokenAuthenticationCredentials = this.ApplyParameters(containerPolicies.UseTokenAuthenticationCredentials);
                            bool tokenAuthVal = CheckIsValidBoolean(containerPolicies.UseTokenAuthenticationCredentials);

                            if(defaultRepoVal && tokenAuthVal)
                            {
                                ImageBuilderUtility.TraceAndThrowValidationError(
                                  TraceType,
                                  StringResources.ImageBuilder_DefaultRepoCredAndTokenAuthBothTrue);
                            }

                            servicePackage.DigestedCodePackage[i].ContainerHostPolicies = containerPolicies;
                            this.ApplyParameters(servicePackage.DigestedCodePackage[i].ContainerHostPolicies);
                        }

                        servicePackage.DigestedCodePackage[i].CodePackage.EnvironmentVariables =
                            matchingServiceManifest.CodePackages[i].CodePackageType.EnvironmentVariables;

                        resourceGovernancePolicy = (ResourceGovernancePolicyType)serviceManifestImport.Policies.FirstOrDefault(
                           policy => policy.GetType().Equals(typeof(ResourceGovernancePolicyType)) &&
                           ImageBuilderUtility.Equals(((ResourceGovernancePolicyType)policy).CodePackageRef, matchingServiceManifest.CodePackages[i].CodePackageType.Name));

                        ConfigPackagePoliciesType configPackagePolicies = (ConfigPackagePoliciesType)serviceManifestImport.Policies.FirstOrDefault(
                            policy => policy.GetType().Equals(typeof(ConfigPackagePoliciesType)) &&
                            ImageBuilderUtility.Equals(((ConfigPackagePoliciesType)policy).CodePackageRef,
                             matchingServiceManifest.CodePackages[i].CodePackageType.Name));

                        if (configPackagePolicies != null)
                        {
                            servicePackage.DigestedCodePackage[i].ConfigPackagePolicies = configPackagePolicies;
                            ApplyParamsAndValidateConfigPolicies(servicePackage.DigestedCodePackage[i].ConfigPackagePolicies, matchingServiceManifest, servicePackage.DigestedCodePackage[i].CodePackage.EntryPoint.Item is ContainerHostEntryPointType);
                        }
                    }

                    if (serviceManifestImport.EnvironmentOverrides != null &&
                           serviceManifestImport.EnvironmentOverrides.Length > 0)
                    {
                        EnvironmentOverridesType envOverrides = serviceManifestImport.EnvironmentOverrides
                            .FirstOrDefault(
                                (envOverride =>
                                    ImageBuilderUtility.Equals(envOverride.CodePackageRef,
                                        matchingServiceManifest.CodePackages[i].CodePackageType.Name)));

                        if (envOverrides != null && envOverrides.EnvironmentVariable != null)
                        {
                            foreach (var envVar in envOverrides.EnvironmentVariable)
                            {
                                var codePackageVariables =
                                    matchingServiceManifest.CodePackages[i].CodePackageType.EnvironmentVariables;

                                if (codePackageVariables != null &&
                                    codePackageVariables.Length > 0)
                                {
                                    var matchingEnvVariable =
                                        codePackageVariables.FirstOrDefault(
                                                env => ImageBuilderUtility.Equals(env.Name, envVar.Name));
                                    if (matchingEnvVariable != null)
                                    {
                                        matchingEnvVariable.Value = this.ApplyParameters(envVar.Value);
                                       
                                        if(!String.IsNullOrEmpty(envVar.Type))
                                        {
                                            var type = this.ApplyParameters(envVar.Type);

                                            EnvironmentVariableTypeType envVariableType;
                                            if (!Enum.TryParse<EnvironmentVariableTypeType>(type, out envVariableType))
                                            {
                                                ImageBuilderUtility.TraceAndThrowValidationError(
                                                    TraceType,
                                                    StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                                                    envVar.Name,
                                                    "Type",
                                                    "EnvironmentOverrides",
                                                    type,
                                                    "PlainText/Encrypted/SecretsStoreRef");
                                            }
                                            matchingEnvVariable.Type = envVariableType;
                                        }  
                                    }
                                }
                            }
                        }
                    }

                    // If no RunAsPolicy is specified, use DefaultRunAs policy if it exists
                    if (runAsPolicy == null && defaultUserRef != null)
                    {
                        runAsPolicy = new RunAsPolicyType()
                        {
                            CodePackageRef = matchingServiceManifest.CodePackages[i].CodePackageType.Name,
                            UserRef = defaultUserRef,
                            EntryPointType = RunAsPolicyTypeEntryPointType.Main
                        };

                        runasPolicies.Add(runAsPolicy);
                    }
                    if (runasPolicies.Count > 0)
                    {
                        servicePackage.DigestedCodePackage[i].RunAsPolicy = runasPolicies.ToArray();
                    }
                    for (int j = 0; j < runasPolicies.Count; j++)
                    {
                        this.ApplyParameters(servicePackage.DigestedCodePackage[i].RunAsPolicy[j]);
                    }
                    if(resourceGovernancePolicy != null)
                    {
                        this.ApplyParameters(resourceGovernancePolicy);
                    }
                    servicePackage.DigestedCodePackage[i].ResourceGovernancePolicy = resourceGovernancePolicy;

                    if (serviceDebugParams != null &&
                        serviceDebugParams.Count != 0)
                    {
                        List<CodePackageDebugParameters> codePackageDebugParams = serviceDebugParams.FindAll(
                            codePackageParam => ImageBuilderUtility.Equals(servicePackage.DigestedCodePackage[i].CodePackage.Name, codePackageParam.CodePackageName));
                        foreach (CodePackageDebugParameters dparams in codePackageDebugParams)
                        {
                            servicePackage.DigestedCodePackage[i].DebugParameters = new DebugParametersType();
                            servicePackage.DigestedCodePackage[i].DebugParameters.Arguments = dparams.DebugArguments;
                            servicePackage.DigestedCodePackage[i].DebugParameters.ProgramExePath = dparams.DebugExePath;
                            servicePackage.DigestedCodePackage[i].DebugParameters.CodePackageLinkFolder = dparams.CodePackageLinkFolder;
                            servicePackage.DigestedCodePackage[i].DebugParameters.LockFile = dparams.LockFile;
                            servicePackage.DigestedCodePackage[i].DebugParameters.WorkingFolder = dparams.WorkingFolder;
                            servicePackage.DigestedCodePackage[i].DebugParameters.DebugParametersFile = dparams.DebugParametersFile;
                            DebugParametersTypeEntryPointType entryPointType;
                            if (Enum.TryParse<DebugParametersTypeEntryPointType>(dparams.EntryPointType, out entryPointType))
                            {
                                servicePackage.DigestedCodePackage[i].DebugParameters.EntryPointType = entryPointType;
                            }
                            else
                            {
                                servicePackage.DigestedCodePackage[i].DebugParameters.EntryPointType = DebugParametersTypeEntryPointType.Main;
                            }
                            if (dparams.ContainerDebugParams != null)
                            {
                                servicePackage.DigestedCodePackage[i].DebugParameters.ContainerEntryPoint = dparams.ContainerDebugParams.Entrypoint;
                                servicePackage.DigestedCodePackage[i].DebugParameters.ContainerEnvironmentBlock = dparams.ContainerDebugParams.EnvVars;
                                servicePackage.DigestedCodePackage[i].DebugParameters.ContainerMountedVolume = dparams.ContainerDebugParams.Volumes;
                                servicePackage.DigestedCodePackage[i].DebugParameters.ContainerLabel = dparams.ContainerDebugParams.Labels;
                            }
                        }
                    }
                }

                // Build DigestedConfigPackage
                servicePackage.DigestedConfigPackage = new ServicePackageTypeDigestedConfigPackage[matchingServiceManifest.ConfigPackages.Count];
                for (int i = 0; i < matchingServiceManifest.ConfigPackages.Count; i++)
                {
                    servicePackage.DigestedConfigPackage[i] = new ServicePackageTypeDigestedConfigPackage();
                    servicePackage.DigestedConfigPackage[i].ConfigPackage = matchingServiceManifest.ConfigPackages[i].ConfigPackageType;
                    servicePackage.DigestedConfigPackage[i].ContentChecksum = matchingServiceManifest.ConfigPackages[i].Checksum;

                    if (serviceManifestImport.ConfigOverrides != null)
                    {
                        ConfigOverrideType configOverride = serviceManifestImport.ConfigOverrides.FirstOrDefault(
                            configOverrideType => ImageBuilderUtility.Equals(configOverrideType.Name, matchingServiceManifest.ConfigPackages[i].ConfigPackageType.Name));
                        servicePackage.DigestedConfigPackage[i].ConfigOverride = configOverride;
                    }

                    this.ApplyParameters(servicePackage.DigestedConfigPackage[i].ConfigOverride);

                    if (serviceManifestImport.Policies != null)
                    {
                        if (configScope != null || 
                            this.GetSharingPolicyForPackage(matchingServiceManifest.ConfigPackages[i].ConfigPackageType.Name, 
                            serviceManifestImport.Policies) != null)
                        {
                            servicePackage.DigestedConfigPackage[i].IsShared = true;
                            servicePackage.DigestedConfigPackage[i].IsSharedSpecified = true;
                        }
                    }

                    if (serviceDebugParams != null 
                        && serviceDebugParams.Count != 0)
                    {
                        var configPackageDebugParams = serviceDebugParams.FindAll(
                            configPackageParam => ImageBuilderUtility.Equals(servicePackage.DigestedConfigPackage[i].ConfigPackage.Name, configPackageParam.ConfigPackageName));
                        foreach (var dparams in configPackageDebugParams)
                        {
                            servicePackage.DigestedConfigPackage[i].DebugParameters = new DebugParametersType();
                            servicePackage.DigestedConfigPackage[i].DebugParameters.ConfigPackageLinkFolder = dparams.ConfigPackageLinkFolder;
                        }
                    }
                }

                // Build DigestedDataPackage
                servicePackage.DigestedDataPackage = new ServicePackageTypeDigestedDataPackage[matchingServiceManifest.DataPackages.Count];
                for (int i = 0; i < matchingServiceManifest.DataPackages.Count; i++)
                {
                    servicePackage.DigestedDataPackage[i] = new ServicePackageTypeDigestedDataPackage();
                    servicePackage.DigestedDataPackage[i].DataPackage = matchingServiceManifest.DataPackages[i].DataPackageType;
                    servicePackage.DigestedDataPackage[i].ContentChecksum = matchingServiceManifest.DataPackages[i].Checksum;

                    if (serviceManifestImport.Policies != null)
                    {
                        if (dataScope != null ||
                            this.GetSharingPolicyForPackage(matchingServiceManifest.DataPackages[i].DataPackageType.Name, 
                                serviceManifestImport.Policies) != null)
                        {
                            servicePackage.DigestedDataPackage[i].IsShared = true;
                            servicePackage.DigestedDataPackage[i].IsSharedSpecified = true;
                        }
                    }

                    if (serviceDebugParams != null
                        && serviceDebugParams.Count != 0)
                    {
                        var dataPackageDebugParams = serviceDebugParams.FindAll(
                            dataPackageParam => ImageBuilderUtility.Equals(servicePackage.DigestedDataPackage[i].DataPackage.Name, dataPackageParam.DataPackageName));
                        foreach (var dparams in dataPackageDebugParams)
                        {
                            servicePackage.DigestedDataPackage[i].DebugParameters = new DebugParametersType();
                            servicePackage.DigestedDataPackage[i].DebugParameters.DataPackageLinkFolder = dparams.DataPackageLinkFolder;
                        }
                    }
                }                

                // Build DigestedResources
                servicePackage.DigestedResources = new ServicePackageTypeDigestedResources();
                GetDigestedEndpoints(serviceManifestImport, matchingServiceManifestType, applicationManifestType, servicePackage, defaultUserRef);

                // Build NetworkPolicies
                if (serviceManifestImport.Policies != null)
                {
                    NetworkPoliciesType networkPolicies = (NetworkPoliciesType)serviceManifestImport.Policies.FirstOrDefault(
                        policy => policy.GetType().Equals(typeof(NetworkPoliciesType)));

                    if (networkPolicies != null)
                    {
                        servicePackage.NetworkPolicies = networkPolicies.Items;

                        this.ApplyParameters(servicePackage.NetworkPolicies);
                    }
                }

                // Copy Diagnostics settings
                if (matchingServiceManifestType.Diagnostics != null)
                {
                    servicePackage.Diagnostics = matchingServiceManifestType.Diagnostics;
                }
                else
                {
                    servicePackage.Diagnostics = new ServiceDiagnosticsType();
                }

                using (MemoryStream stream = new MemoryStream())
                {
                    ImageBuilderUtility.WriteXml<ServicePackageType>(stream, servicePackage);
                    stream.Seek(0, SeekOrigin.Begin);
                    servicePackage.ContentChecksum = ChecksumUtility.ComputeHash(stream);
                }

                // The RolloutVersion is applied to the ServicePackage after computing the ContentChecksum.
                // This is done so that if a ServicePackage is created with identical data but with different 
                // RolloutVersion (as in the case of upgrade rollback), the ContentChecksum value will the same.
                // Hosting uses this when upgrading the ServicePackage.
                PopulateServicePackageWithRolloutVersion(servicePackage, rolloutVersion);

                servicePackageTypes.Add(servicePackage);
            }

            return servicePackageTypes;
        }

        private void VerifySourceLocation(string settingName, string sectionName, string type)
        {
            if (!ImageBuilderUtility.Equals("PlainText", type)
                && !ImageBuilderUtility.Equals("Encrypted", type)
                && !ImageBuilderUtility.Equals("SecretsStoreRef", type))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                  TraceType,
                  StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                  settingName,
                  "Type",
                  sectionName,
                  type,
                  "PlainText/Encrypted/SecretsStoreRef");
            }
        }

        private void ApplyParamsAndValidateConfigPolicies(
            ConfigPackagePoliciesType configPackagePolicies,
            ServiceManifest matchingServiceManifest,
            bool isContainerHost)
        {
            if (configPackagePolicies.ConfigPackage == null)
            {
                return;
            }

            foreach (ConfigPackageDescriptionType configPackage in configPackagePolicies.ConfigPackage)
            {
                configPackage.Name = this.ApplyParameters(configPackage.Name);
                configPackage.SectionName = this.ApplyParameters(configPackage.SectionName);

                if (!String.IsNullOrEmpty(configPackage.MountPoint))
                {
                    configPackage.MountPoint = this.ApplyParameters(configPackage.MountPoint);
                }

                if (!String.IsNullOrEmpty(configPackage.EnvironmentVariableName))
                {
                    configPackage.EnvironmentVariableName = this.ApplyParameters(configPackage.EnvironmentVariableName);

                }
            }

            ValidateConfiguration(configPackagePolicies, matchingServiceManifest, isContainerHost);
        }

        private void ValidateConfiguration(
            ConfigPackagePoliciesType configPackagePolicies,
            ServiceManifest matchingServiceManifest,
            bool isContainerHost)
        {
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Validating ConfigPackagePolices for CodePackageRef:{0}",
                configPackagePolicies.CodePackageRef);

            var buildLayout = this.applicationTypeContext.BuildLayoutSpecification;
            var appTypeName = this.applicationTypeContext.ApplicationManifest.ApplicationTypeName;
            IDictionary<string, HashSet<string>> mountPointsParameters = new Dictionary<string, HashSet<string>>();
            IDictionary<string, HashSet<string>> mountPointEnvVar = new Dictionary<string, HashSet<string>> ();

            string guidForEmptyMountPointsOrEnvVar = Guid.NewGuid().ToString();
            string mountPoint;

            StoreLayoutSpecification storeLayoutSpecification = StoreLayoutSpecification.Create();
            SettingsType settingsType = null;
            var configPackageName = "";
            foreach (ConfigPackageDescriptionType configPackage in configPackagePolicies.ConfigPackage)
            {
                // Validation1: Verify the config package is in ServiceManifest
                ConfigPackage configPackageInServiceManifest = matchingServiceManifest.ConfigPackages
                   .FirstOrDefault(
                    (configPackageInManifest =>
                      ImageBuilderUtility.Equals(configPackageInManifest.ConfigPackageType.Name,
                         configPackage.Name)));

                if (configPackageInServiceManifest == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                       TraceType,
                       StringResources.ImageBuilderError_InvalidRef,
                       "ConfigPackage",
                        configPackage.Name,
                        "ConfigPackagePolicies",
                        "Config",
                        "ServiceManifest");
                }

                //Do not read the config Package if the ConfigPackage Name is same but the SectionName is different 
                if (String.IsNullOrEmpty(configPackageName) || !ImageBuilderUtility.Equals(configPackageName, configPackage.Name))
                {
                    string tempFile = "";
                    string directoryToExtract = "";
                    try
                    {
                        configPackageName = configPackage.Name;
                        settingsType = null;
                        string configPackageDirectory = storeLayoutSpecification.GetConfigPackageFolder(appTypeName, matchingServiceManifest.ServiceManifestType.Name, configPackageInServiceManifest.ConfigPackageType.Name, configPackageInServiceManifest.ConfigPackageType.Version);
                        string settingsFile = storeLayoutSpecification.GetSettingsFile(configPackageDirectory);

                        if (this.ImageStoreWrapper.DoesContentExists(settingsFile, timeoutHelper.GetRemainingTime()))
                        {
                            settingsType = this.ImageStoreWrapper.GetFromStore<SettingsType>(settingsFile, timeoutHelper.GetRemainingTime());
                        }
                        else
                        {
                            //Look for Compressed config file
                            string archiveFile = storeLayoutSpecification.GetSubPackageArchiveFile(configPackageDirectory);

                            if (this.ImageStoreWrapper.DoesContentExists(archiveFile, timeoutHelper.GetRemainingTime()))
                            {
                                string currentWorkingDir = Directory.GetCurrentDirectory();
                                tempFile = Path.Combine(currentWorkingDir, Guid.NewGuid().ToString());

                                if (!this.ImageStoreWrapper.DownloadIfContentExists(archiveFile, tempFile, timeoutHelper.GetRemainingTime()))
                                {
                                    throw new FileNotFoundException(StringResources.Error_FileNotFound, archiveFile);
                                }

                                directoryToExtract = tempFile + '_' + configPackageName;
                                ImageBuilderUtility.ExtractArchive(tempFile, directoryToExtract);

                                string settingsFilePath = FabricDirectory.GetFiles(directoryToExtract, "Settings.xml", SearchOption.TopDirectoryOnly).FirstOrDefault();

                                if (String.IsNullOrEmpty(settingsFilePath))
                                {
                                    throw new FileNotFoundException(StringResources.Error_FileNotFound, settingsFilePath);
                                }

                                settingsType = ImageBuilderUtility.ReadXml<SettingsType>(settingsFilePath);
                            }
                        } 
                    }
                    finally
                    {
                        if (!String.IsNullOrEmpty(tempFile) && FabricFile.Exists(tempFile))
                        {
                            FabricFile.Delete(tempFile);
                        }
                        if (!String.IsNullOrEmpty(directoryToExtract))
                        {
                            FabricDirectory.Delete(directoryToExtract, true);
                        }
                    }

                }

                //Validation2: Section shold be present in Setting.xml
                if (settingsType == null 
                    || settingsType.Section == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                         TraceType,
                         StringResources.Error_ImageBuilderConfigPackageValidationFailed,
                         configPackage.Name,
                         configPackage.SectionName,
                         configPackagePolicies.CodePackageRef);
                }

                SettingsTypeSection sectionInSettingXml = settingsType.Section.FirstOrDefault(
                    (section => ImageBuilderUtility.Equals(section.Name, configPackage.SectionName)));

                if (sectionInSettingXml == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                       TraceType,
                       StringResources.Error_ImageBuilderConfigPackageValidationFailed,
                       configPackage.Name,
                       configPackage.SectionName,
                       configPackagePolicies.CodePackageRef);
                }

                //Validation3: Check if the mount point is absolute for containers and relative for non container hosts
                mountPoint = guidForEmptyMountPointsOrEnvVar;
                if (!String.IsNullOrEmpty(configPackage.MountPoint))
                {
                    bool isRootedPath = Path.IsPathRooted(configPackage.MountPoint);

                    // If not a container, validate mount directory is a relative path.
                    if (isRootedPath && !isContainerHost)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                              TraceType,
                              StringResources.Error_ImageBuilderInvalidConfigPackageMountPoint,
                              configPackage.Name,
                              configPackage.MountPoint,
                              "relative",
                              "non container");
                    }
                    else if (!isRootedPath && isContainerHost)
                    {
                        // If container, validate mount directory is an absolute path.
                        ImageBuilderUtility.TraceAndThrowValidationError(
                              TraceType,
                              StringResources.Error_ImageBuilderInvalidConfigPackageMountPoint,
                              configPackage.Name,
                              configPackage.MountPoint,
                              "absolute",
                              "container");
                    }

                    mountPoint = configPackage.MountPoint;
                }

                //Validation4: For same mount points there should not be duplicate parameters present in any section
                if (!mountPointsParameters.ContainsKey(mountPoint))
                {
                    mountPointsParameters.Add(mountPoint, new HashSet<string>());
                }

                HashSet<string> parameters = mountPointsParameters[mountPoint];

                if (sectionInSettingXml.Parameter != null)
                {
                    foreach (SettingsTypeSectionParameter parameter in sectionInSettingXml.Parameter)
                    {
                        if (parameters.Contains(parameter.Name))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.Error_ImageBuilderConfigPackageDuplicateElement,
                                configPackage.Name,
                                configPackage.MountPoint);
                        }
                        mountPointsParameters[mountPoint].Add(parameter.Name);
                    }
                }
                //Validation5: Check if the host is not containerhost then the mount point should have an EnvironmentVariableName specified
                string envVarName = String.IsNullOrEmpty(configPackage.EnvironmentVariableName) ? guidForEmptyMountPointsOrEnvVar : configPackage.EnvironmentVariableName;

                if (!mountPointEnvVar.ContainsKey(mountPoint))
                {
                    mountPointEnvVar.Add(mountPoint, new HashSet<string>());
                }

                mountPointEnvVar[mountPoint].Add(envVarName);

                foreach (var mounts in mountPointEnvVar)
                {
                    if (!ImageBuilderUtility.Equals(envVarName, guidForEmptyMountPointsOrEnvVar))
                    {
                        if (!ImageBuilderUtility.Equals(mounts.Key, mountPoint)
                            && mounts.Value.Contains(envVarName))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.ImageBuilderError_DuplicateElementFound,
                                "MountPoint",
                                "EnvironmentVariableName",
                                 configPackage.EnvironmentVariableName);
                        }
                    }
                }
            }

            if (!isContainerHost)
            {
                foreach (var mounts in mountPointEnvVar)
                {
                    //Check if there is any mount point that have an empty environment variable
                    if (mounts.Value.Count == 1 && mounts.Value.Contains(guidForEmptyMountPointsOrEnvVar))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                                    TraceType,
                                    StringResources.Error_ImageBuilderInvalidEnvironmentVarForMountPoint,
                                    configPackagePolicies.CodePackageRef,
                                    mounts.Key);
                    }
                }
            }
        }

        private void GetDigestedEndpoints(
            ApplicationManifestTypeServiceManifestImport serviceManifestImport,
            ServiceManifestType matchingServiceManifestType,
            ApplicationManifestType applicationManifestType,
            ServicePackageType servicePackage,
            string defaultUserRef)
        {
            var endpointPolicies = this.GetEndPointPolicies(serviceManifestImport.Policies);

            if (matchingServiceManifestType.Resources != null && matchingServiceManifestType.Resources.Endpoints != null)
            {
                servicePackage.DigestedResources.DigestedEndpoints =
                    new ServicePackageTypeDigestedResourcesDigestedEndpoint[
                        matchingServiceManifestType.Resources.Endpoints.Length];
                List<EndpointCertificateType> matchingCertificates = new List<EndpointCertificateType>();
                HashSet<string> matchingHttpsEndpointNames = new HashSet<string>();
                HashSet<string> overriddenEndpoints = new HashSet<string>();
                this.ApplyParameters(serviceManifestImport.ResourceOverrides);

                for (int i = 0; i < matchingServiceManifestType.Resources.Endpoints.Length; i++)
                {
                    servicePackage.DigestedResources.DigestedEndpoints[i] =
                        new ServicePackageTypeDigestedResourcesDigestedEndpoint();
                    servicePackage.DigestedResources.DigestedEndpoints[i].Endpoint =
                        matchingServiceManifestType.Resources.Endpoints[i];

                    //Override the endpoints specified in ServiceManifest while creating the DigestedService Package.
                    this.OverrideEndpoints(serviceManifestImport, servicePackage, matchingServiceManifestType, i, overriddenEndpoints);

                    ServicePackageTypeDigestedResourcesDigestedEndpoint endpointInManifest = servicePackage.DigestedResources.DigestedEndpoints[i];

                    SecurityAccessPolicyType securityAccessPolicy = null;
                    EndpointBindingPolicyType endpointBindingPolicy = null;

                    if (serviceManifestImport.Policies != null)
                    {
                        securityAccessPolicy = (SecurityAccessPolicyType)serviceManifestImport.Policies.FirstOrDefault(
                            policy => policy.GetType().Equals(typeof (SecurityAccessPolicyType))
                                      &&
                                      ImageBuilderUtility.Equals(((SecurityAccessPolicyType) policy).ResourceRef,
                                          endpointInManifest.Endpoint.Name)
                                      &&
                                      (((SecurityAccessPolicyType) policy)).ResourceType ==
                                      SecurityAccessPolicyTypeResourceType.Endpoint);

                        endpointBindingPolicy = endpointPolicies.FirstOrDefault(
                            policy =>
                                ImageBuilderUtility.Equals(policy.EndpointRef,
                                    endpointInManifest.Endpoint.Name));

                        if (endpointBindingPolicy != null)
                        {
                            if (overriddenEndpoints.Contains(endpointInManifest.Endpoint.Name) && (endpointInManifest.Endpoint.Protocol != EndpointTypeProtocol.https))
                            {
                                //The endpoint have been overridden to not be https. In this case ignore the endpoint binding policy.
                                ImageBuilder.TraceSource.WriteInfo(
                                    TraceType,
                                    "The endpoint {0} have been overridden to not be https. In this case ignore the endpoint binding policy.",
                                    endpointInManifest.Endpoint.Name);

                                endpointBindingPolicy = null;

                            }
                            else
                            {
                                if (endpointInManifest.Endpoint.Protocol != EndpointTypeProtocol.https)
                                {
                                    ImageBuilderUtility.TraceAndThrowValidationError(
                                        TraceType,
                                        StringResources.ImageBuilderError_InvalidRef,
                                        "EndpointRef",
                                        endpointBindingPolicy.EndpointRef,
                                        "EndpointBindingPolicy",
                                        "Resource",
                                        "ServiceManifest");
                                }
                                matchingHttpsEndpointNames.Add(endpointBindingPolicy.EndpointRef);
                                EndpointCertificateType certificate = applicationManifestType.Certificates
                                    .EndpointCertificate.FirstOrDefault(
                                        cert => endpointBindingPolicy.CertificateRef.Equals(cert.Name));
                                if (certificate != null)
                                {
                                    //Multiple EndpointBindingPolicies can point to the same cert, this check ensures that we add only one to digestedresources
                                    var matchingCert =
                                        matchingCertificates.FirstOrDefault(
                                            cert => ImageBuilderUtility.Equals(cert.Name, certificate.Name));
                                    if (matchingCert == null)
                                    {
                                        matchingCertificates.Add(certificate);
                                    }
                                }

                            }
                        }
                    }

                    if (securityAccessPolicy == null && defaultUserRef != null)
                    {
                        securityAccessPolicy = new SecurityAccessPolicyType()
                        {
                            ResourceRef = endpointInManifest.Endpoint.Name,
                            PrincipalRef = defaultUserRef,
                            GrantRights = SecurityAccessPolicyTypeGrantRights.Read
                        };
                    }

                    servicePackage.DigestedResources.DigestedEndpoints[i].SecurityAccessPolicy = securityAccessPolicy;
                    servicePackage.DigestedResources.DigestedEndpoints[i].EndpointBindingPolicy = endpointBindingPolicy;

                    this.ApplyParameters(servicePackage.DigestedResources.DigestedEndpoints[i].SecurityAccessPolicy);

                }
                //If there are no https endpoints specified and there are EndpointBinding Policies. In that case the matchingHttpsEndpointNames will be empty.
                foreach (var endpointPolicy in endpointPolicies)
                {
                    if (!matchingHttpsEndpointNames.Contains(endpointPolicy.EndpointRef))
                    {
                        ServicePackageTypeDigestedResourcesDigestedEndpoint matchingEndpoint = servicePackage.DigestedResources.DigestedEndpoints.FirstOrDefault(
                            endpoint => ImageBuilderUtility.Equals(endpointPolicy.EndpointRef,
                                    endpoint.Endpoint.Name));

                        if (matchingEndpoint!=null &&
                            matchingEndpoint.Endpoint != null &&
                            overriddenEndpoints.Contains(matchingEndpoint.Endpoint.Name) &&
                            matchingEndpoint.Endpoint.Protocol != EndpointTypeProtocol.https)
                        {
                            continue;
                        }

                        ImageBuilderUtility.TraceAndThrowValidationError(
                            TraceType,
                            StringResources.ImageBuilderError_InvalidRef,
                            "EndpointRef",
                            endpointPolicy.EndpointRef,
                            "EndpointBindingPolicy",
                            "Resource",
                            "ServiceManifest");
                    }
                }
                if (matchingCertificates.Count > 0)
                {
                    servicePackage.DigestedResources.DigestedCertificates = matchingCertificates.ToArray();
                }
            }
        }

        private void OverrideEndpoints(
            ApplicationManifestTypeServiceManifestImport serviceManifestImport,
             ServicePackageType servicePackage,
             ServiceManifestType matchingServiceManifestType,
             int endPointIndex,
             HashSet<string> overriddenEndpoints)
        {
            EndpointOverrideType endpointOverride = null;

            if (serviceManifestImport.ResourceOverrides != null
                && serviceManifestImport.ResourceOverrides.Endpoints != null
                && serviceManifestImport.ResourceOverrides.Endpoints.Length > 0)
            {
                endpointOverride = (EndpointOverrideType)serviceManifestImport.ResourceOverrides.Endpoints.FirstOrDefault(
                    endpoint => endpoint.GetType().Equals(typeof(EndpointOverrideType))
                    &&
                    ImageBuilderUtility.Equals(((EndpointOverrideType)endpoint).Name,
                       matchingServiceManifestType.Resources.Endpoints[endPointIndex].Name));

                if (endpointOverride != null)
                {
                    overriddenEndpoints.Add(endpointOverride.Name);

                    //Validate all the values that are overridden are correct.
                    if (!String.IsNullOrEmpty(endpointOverride.Protocol))
                    {
                        EndpointTypeProtocol protocol;
                        if (!Enum.TryParse(endpointOverride.Protocol, true, out protocol))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                             TraceType,
                             StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                             endpointOverride.Name,
                             "Protocol",
                             "ResourceOverrides",
                             endpointOverride.Protocol,
                             "http/https/tcp/udp");
                        }

                        servicePackage.DigestedResources.DigestedEndpoints[endPointIndex].Endpoint.Protocol = protocol;
                    }

                    if (!String.IsNullOrEmpty(endpointOverride.Port))
                    {
                        int port;
                        if (!int.TryParse(endpointOverride.Port, out port))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                               TraceType,
                               StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                               endpointOverride.Name,
                               "Port",
                               "ResourceOverrides",
                               endpointOverride.Port,
                               "int");
                        }

                        servicePackage.DigestedResources.DigestedEndpoints[endPointIndex].Endpoint.Port = port;
                        servicePackage.DigestedResources.DigestedEndpoints[endPointIndex].Endpoint.PortSpecified = true;
                    }

                    if (!String.IsNullOrEmpty(endpointOverride.Type))
                    {
                        EndpointTypeType endPointType;
                        if (!Enum.TryParse(endpointOverride.Type, true, out endPointType))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                              TraceType,
                              StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                              endpointOverride.Name,
                              "Type",
                              "ResourceOverrides",
                              endpointOverride.Type,
                              "internal/input");
                        }

                        servicePackage.DigestedResources.DigestedEndpoints[endPointIndex].Endpoint.Type = endPointType;
                    }

                    if (!String.IsNullOrEmpty(endpointOverride.UriScheme))
                    {
                        servicePackage.DigestedResources.DigestedEndpoints[endPointIndex].Endpoint.UriScheme = endpointOverride.UriScheme;
                    }

                    if (!String.IsNullOrEmpty(endpointOverride.PathSuffix))
                    {
                        servicePackage.DigestedResources.DigestedEndpoints[endPointIndex].Endpoint.PathSuffix = endpointOverride.PathSuffix;
                    }
                }
            }
        }

        private PackageSharingPolicyType GetSharingPolicyForPackage(string packageName, object[] policies)
        {
            return (PackageSharingPolicyType) policies.FirstOrDefault(
                policy => policy.GetType().Equals(typeof (PackageSharingPolicyType)) &&
                          ImageBuilderUtility.Equals(((PackageSharingPolicyType) policy).PackageRef, packageName));
        }

        //return sharing policies that match the scope specified or Scope "All"
        private PackageSharingPolicyType GetSharingPolicyForScope(PackageSharingPolicyTypeScope scope, object[] policies)
        {
            return (PackageSharingPolicyType)policies.FirstOrDefault(
                                policy => policy.GetType().Equals(typeof(PackageSharingPolicyType)) &&
                                String.IsNullOrEmpty(((PackageSharingPolicyType)policy).PackageRef) &&
                                (((PackageSharingPolicyType)policy).Scope == scope ||
                                ((PackageSharingPolicyType)policy).Scope == PackageSharingPolicyTypeScope.All));
        }

        private static void PopulateApplicationPackageWithRolloutVersion(ApplicationPackageType applicationPackage, string rolloutVersion)
        {
            applicationPackage.RolloutVersion = rolloutVersion;
            applicationPackage.DigestedEnvironment.RolloutVersion = rolloutVersion;
            applicationPackage.DigestedCertificates.RolloutVersion = rolloutVersion;
        }

        private List<EndpointBindingPolicyType> GetEndPointPolicies(object[] policies)
        {
            DuplicateDetector duplicateDetector = new DuplicateDetector("EndpointBindingPolicy", "EndpointRef");
            List<EndpointBindingPolicyType> endpointPolicies = new List<EndpointBindingPolicyType>();
            if (policies != null)
            {
                foreach (var policy in policies)
                {
                    var endpointPolicy = policy as EndpointBindingPolicyType;
                    if (endpointPolicy != null)
                    {
                        endpointPolicy.EndpointRef = this.ApplyParameters(endpointPolicy.EndpointRef);
                        duplicateDetector.Add(endpointPolicy.EndpointRef);
                        endpointPolicies.Add(endpointPolicy);
                    }
                }
            }
            return endpointPolicies;
        }

        private static void PopulateServicePackageWithRolloutVersion(ServicePackageType servicePackage, string rolloutVersion)
        {
            servicePackage.RolloutVersion = rolloutVersion;
            servicePackage.DigestedServiceTypes.RolloutVersion = rolloutVersion;
            
            foreach (var digestedCodePackage in servicePackage.DigestedCodePackage)
            {
                digestedCodePackage.RolloutVersion = rolloutVersion;
            }

            foreach (var digestedConfigPackage in servicePackage.DigestedConfigPackage)
            {
                digestedConfigPackage.RolloutVersion = rolloutVersion;
            }

            foreach (var digestedDataPackage in servicePackage.DigestedDataPackage)
            {
                digestedDataPackage.RolloutVersion = rolloutVersion;
            }

            servicePackage.DigestedResources.RolloutVersion = rolloutVersion;
        }        

        private static IEnumerable<ServiceTypeType> DigestServiceTypes(object[] manifestServiceTypes)
        {
            System.Fabric.Interop.Utility.ReleaseAssert(manifestServiceTypes != null, "manifestServiceTypes must not be null");

            return manifestServiceTypes.Select(manifestServiceType =>
            {
                ServiceTypeType serviceType = manifestServiceType as ServiceTypeType;
                if (serviceType != null)
                {
                    return serviceType;
                }
                else
                {
                    ServiceGroupTypeType serviceGroupType = manifestServiceType as ServiceGroupTypeType;
                    System.Fabric.Interop.Utility.ReleaseAssert(serviceGroupType != null, "Unexpected service type");
                    return ApplicationInstanceBuilder.ConvertServiceGroupTypeToServiceType(serviceGroupType);
                }
            });
        }

        private static ServiceTypeType ConvertServiceGroupTypeToServiceType(ServiceGroupTypeType serviceGroupType)
        {
            ServiceTypeType serviceType = null;

            if (serviceGroupType is StatefulServiceGroupTypeType)
            {
                serviceType = new StatefulServiceTypeType
                {
                    HasPersistedState = ((StatefulServiceGroupTypeType)serviceGroupType).HasPersistedState,
                };
            }
            else
            {
                serviceType = new StatelessServiceTypeType();
            }

            serviceType.ServiceTypeName = serviceGroupType.ServiceGroupTypeName;

            serviceType.Extensions = serviceGroupType.Extensions;
            serviceType.PlacementConstraints = serviceGroupType.PlacementConstraints;

            // get all the members load metrics and group by name
            var memberMetrics = serviceGroupType.ServiceGroupMembers.SelectMany(member => member.LoadMetrics ?? new LoadMetricType[0]);
            var memberMetricsByName = memberMetrics.GroupBy(metric => metric.Name);

            // sum up the default values for metrics of the same name
            var combinedMetrics = memberMetricsByName.Select(namedMetrics => 
            {
                bool hasWeight = namedMetrics.Any(metric => metric.WeightSpecified);
                LoadMetricTypeWeight weight = LoadMetricTypeWeight.Zero;
                if (hasWeight)
                {
                    weight = namedMetrics.Where(metric => metric.WeightSpecified).Max(metric => metric.Weight);
                }

                return new LoadMetricType
                {
                    Name = namedMetrics.Key,
                    PrimaryDefaultLoad = (uint)namedMetrics.Select(metric => metric.PrimaryDefaultLoad).Sum(load => (decimal)load),
                    SecondaryDefaultLoad = (uint)namedMetrics.Select(metric => metric.SecondaryDefaultLoad).Sum(load => (decimal)load),
                    Weight = weight,
                    WeightSpecified = hasWeight,
                };
            });

            var loadMetricResult = combinedMetrics.ToArray();

            if (serviceGroupType.LoadMetrics != null && serviceGroupType.LoadMetrics.Any())
            {
                serviceType.LoadMetrics = serviceGroupType.LoadMetrics;
            }
            else if (loadMetricResult.Length > 0)
            {
                serviceType.LoadMetrics = loadMetricResult;
            }
            else
            {
                serviceType.LoadMetrics = null;
            }

            return serviceType;
        }

        private bool IsValidServicePackageActivationMode(string activationMode)
        {
            return (activationMode.Equals("SharedProcess", StringComparison.OrdinalIgnoreCase) ||
                    activationMode.Equals("ExclusiveProcess", StringComparison.OrdinalIgnoreCase));
        }

        private void ApplyParameters(ContainerHostPoliciesType containerPolicies)
        {
            DuplicateDetector duplicateDetector = new DuplicateDetector("CertificateRef", "Name");
            if (containerPolicies != null && containerPolicies.Items != null)
            {
                foreach (var item in containerPolicies.Items)
                {
                    Type policyType = item.GetType();
                    if (policyType.Equals(typeof(RepositoryCredentialsType)))
                    {
                        var repositoryCredentials = item as RepositoryCredentialsType;

                        repositoryCredentials.AccountName = this.ApplyParameters(repositoryCredentials.AccountName);
                        repositoryCredentials.Password = this.ApplyParameters(repositoryCredentials.Password);
                        repositoryCredentials.Email = this.ApplyParameters(repositoryCredentials.Email);
                        ValidateRepositoryCredentialPasswordType(repositoryCredentials);

                    }
                    else if (policyType.Equals(typeof(ContainerLoggingDriverType)))
                    {
                        var logConfig = item as ContainerLoggingDriverType;
                        logConfig.Driver = this.ApplyParameters(logConfig.Driver);
                        if (logConfig.Items != null)
                        {
                            foreach (var driverOpt in logConfig.Items)
                            {
                                driverOpt.Name = this.ApplyParameters(driverOpt.Name);
                                driverOpt.Value = this.ApplyParameters(driverOpt.Value);
                                driverOpt.IsEncrypted = this.ApplyParameters(driverOpt.IsEncrypted);
                                CheckIsValidBoolean(driverOpt.IsEncrypted);
                                // driver options for the container log do not require or reference secrets.
                            }
                        }
                    }
                    else if (policyType.Equals(typeof(ContainerVolumeType)))
                    {
                        var volumeDriver = item as ContainerVolumeType;
                        volumeDriver.Destination = this.ApplyParameters(volumeDriver.Destination);
                        volumeDriver.Source = this.ApplyParameters(volumeDriver.Source);
                        volumeDriver.Driver = this.ApplyParameters(volumeDriver.Driver);
                        if (volumeDriver.Items != null)
                        {
                            foreach (var driverOpt in volumeDriver.Items)
                            {
                                driverOpt.Name = this.ApplyParameters(driverOpt.Name);
                                driverOpt.Value = this.ApplyParameters(driverOpt.Value);
                                driverOpt.IsEncrypted = this.ApplyParameters(driverOpt.IsEncrypted);
                                driverOpt.Type = this.ApplyParameters(driverOpt.Type);
                                // verify whether this driver option is a secret or references one
                                ValidateVolumeDriverOptionProtectionType(driverOpt);
                            }
                        }
                    }
                    else if (policyType.Equals(typeof(ContainerLabelType)))
                    {
                        var label = item as ContainerLabelType;
                        duplicateDetector.Add(label.Name);
                        label.Value = this.ApplyParameters(label.Value);
                    }
                    else if (policyType.Equals(typeof(ContainerNetworkConfigType)))
                    {
                        var networkConfig = item as ContainerNetworkConfigType;
                        networkConfig.NetworkType = this.ApplyParameters(networkConfig.NetworkType);
                        if (String.IsNullOrEmpty(networkConfig.NetworkType) ||
                            (!networkConfig.NetworkType.Equals(StringConstants.OpenNetworkConfig, StringComparison.Ordinal) &&
                            !networkConfig.NetworkType.Equals(StringConstants.OtherNetworkConfig, StringComparison.Ordinal)))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                   TraceType,
                                   StringResources.ImageBuilderError_InvalidValue,
                                   "NetworkType",
                                   networkConfig.NetworkType);
                        }
                    }
                    else if (policyType.Equals(typeof(ContainerCertificateType)))
                    {
                        var certificateRef = item as ContainerCertificateType;
                        duplicateDetector.Add(this.ApplyParameters(certificateRef.Name));
                        certificateRef.Name = this.ApplyParameters(certificateRef.Name);
                        certificateRef.X509FindValue = this.ApplyParameters(certificateRef.X509FindValue);
                        certificateRef.X509StoreName = this.ApplyParameters(certificateRef.X509StoreName);
                        certificateRef.DataPackageRef = this.ApplyParameters(certificateRef.DataPackageRef);
                        certificateRef.DataPackageVersion = this.ApplyParameters(certificateRef.DataPackageVersion);
                        certificateRef.RelativePath = this.ApplyParameters(certificateRef.RelativePath);

                        if ((!string.IsNullOrEmpty(certificateRef.DataPackageRef) && !string.IsNullOrEmpty(certificateRef.X509FindValue))
                            || (string.IsNullOrEmpty(certificateRef.DataPackageRef) && string.IsNullOrEmpty(certificateRef.X509FindValue)))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                   TraceType,
                                   StringResources.ImageBuilderError_InvalidContainerCertParameters,
                                   certificateRef.Name,
                                   containerPolicies.CodePackageRef);
                        }

                        if (!string.IsNullOrEmpty(certificateRef.DataPackageRef) && string.IsNullOrEmpty(certificateRef.RelativePath))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                   TraceType,
                                   StringResources.ImageBuilderError_UnknownContainerCertRelativePaths,
                                   certificateRef.Name,
                                   containerPolicies.CodePackageRef);
                        }

                        if (!string.IsNullOrEmpty(certificateRef.DataPackageRef) && string.IsNullOrEmpty(certificateRef.DataPackageVersion))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                   TraceType,
                                   StringResources.ImageBuilderError_UnknownContainerCertDataPackageVersion,
                                   certificateRef.Name,
                                   containerPolicies.CodePackageRef);
                        }

                        certificateRef.Password = this.ApplyParameters(certificateRef.Password);
                    }
                    else if (policyType.Equals(typeof(SecurityOptionsType)))
                    {
                        var securityOpt = item as SecurityOptionsType;
                        securityOpt.Value = this.ApplyParameters(securityOpt.Value);
                    }
                    else if (policyType.Equals(typeof(ImageOverridesType)))
                    {
                        var images = item as ImageOverridesType;

                        if (images.Image != null)
                        {
                            foreach (ImageType image in images.Image)
                            {
                                image.Name = this.ApplyParameters(image.Name);
                                image.Os = this.ApplyParameters(image.Os);
                            }
                        }
                    }

                    containerPolicies.RunInteractive = this.ApplyParameters(containerPolicies.RunInteractive);
                    if (String.IsNullOrEmpty(containerPolicies.RunInteractive))
                    {
                        containerPolicies.RunInteractive = "False";
                    }
                    else
                    {
                        bool runInteractive;
                        if (!Boolean.TryParse(containerPolicies.RunInteractive, out runInteractive))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                                TraceType,
                                StringResources.ImageBuilderError_InvalidValue,
                                "RunInteractive",
                                containerPolicies.RunInteractive);
                        }
                    }

                    containerPolicies.ContainersRetentionCount = this.ApplyParameters(containerPolicies.ContainersRetentionCount);
                    if (!String.IsNullOrEmpty(containerPolicies.ContainersRetentionCount))
                    {
                        long retentionCount = 0;
                        if (!Int64.TryParse(containerPolicies.ContainersRetentionCount, out retentionCount))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationError(
                              TraceType,
                              StringResources.ImageBuilderError_InvalidValue,
                              "ContainersRetentionCount",
                              containerPolicies.ContainersRetentionCount);
                        }
                    }

                    containerPolicies.AutoRemove = this.ApplyParameters(containerPolicies.AutoRemove);
                    if (!String.IsNullOrEmpty(containerPolicies.AutoRemove))
                    {
                        CheckIsValidBoolean(containerPolicies.AutoRemove);
                    }
                }
            }
        }

        private void ValidateRepositoryCredentialPasswordType(RepositoryCredentialsType repositoryCredentials)
        {
            bool isTypePlainTextOrEmpty = true;
            bool isTypeEncrypted = false;
            bool isTypeEmpty = true;
            if (!String.IsNullOrEmpty(repositoryCredentials.Type))
            {
                isTypeEmpty = false;

                repositoryCredentials.Type = this.ApplyParameters(repositoryCredentials.Type);
                VerifySourceLocation("RepositoryCredentials", "ContainerHostPolicies", repositoryCredentials.Type);

                EnvironmentVariableTypeType repositoryCredenetailPasswordType;
                if (!Enum.TryParse<EnvironmentVariableTypeType>(repositoryCredentials.Type, out repositoryCredenetailPasswordType))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                        TraceType,
                        StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                        "RepositoryCredentials",
                        "Type",
                        "ContainerHostPolicies",
                        repositoryCredentials.Type,
                        "PlainText/Encrypted/SecretsStoreRef");
                }

                if (!(EnvironmentVariableTypeType.PlainText == repositoryCredenetailPasswordType))
                {
                    isTypePlainTextOrEmpty = false;
                }
                if (EnvironmentVariableTypeType.Encrypted == repositoryCredenetailPasswordType)
                {
                    isTypeEncrypted = true;
                }
            }

            // If you have specified the Type and it's not empty/PlainText and then you cannot have an empty password.
            if (string.IsNullOrEmpty(repositoryCredentials.Password) && (repositoryCredentials.PasswordEncrypted || !isTypePlainTextOrEmpty))
            {
                // Accountname is PII so here as pass in a blank string, the exception that is thrown will have the actual AccountName.
                ImageBuilder.TraceSource.WriteError(TraceType, StringResources.ImageBuilderError_RepositoryCredentialsBlankPassword, repositoryCredentials.AccountName);

                throw new FabricImageBuilderValidationException(
                    string.Format(System.Globalization.CultureInfo.InvariantCulture, StringResources.ImageBuilderError_RepositoryCredentialsBlankPassword, repositoryCredentials.AccountName));
            }

            // If PasswordEncrypted is set to true then you cannot have Type set to SecretsStoreRef/PlainText
            if (repositoryCredentials.PasswordEncrypted && !isTypeEmpty && !isTypeEncrypted)
            {

                ImageBuilder.TraceSource.WriteError(TraceType, StringResources.ImageBuilderError_InvalidRepositoryCredentialsType, repositoryCredentials.AccountName);

                throw new FabricImageBuilderValidationException(
                    string.Format(System.Globalization.CultureInfo.InvariantCulture, StringResources.ImageBuilderError_InvalidRepositoryCredentialsType, repositoryCredentials.AccountName));
            }
        }

        /// <summary>
        /// Verifies whether the flags and type of this driver option are a valid combination.
        /// </summary>
        /// <param name="driverOption"></param>
        private void ValidateVolumeDriverOptionProtectionType(DriverOptionType driverOption)
        {
            bool isEncrypted = CheckIsValidBoolean(driverOption.IsEncrypted);

            // TODO [dragosav]: look into renaming the EnvVarTypeType to something more generic, 
            // as the values are not specific to environment variables.
            EnvironmentVariableTypeType driverOptionType;
            if (!Enum.TryParse<EnvironmentVariableTypeType>(driverOption.Type, out driverOptionType))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_ConfigOverrideTypeMismatch,
                    "VolumeDriverOptions",
                    "Type",
                    "ContainerHostPolicies",
                    driverOption.Type,
                    "PlainText/Encrypted/SecretsStoreRef");
            }

            // since we're phasing out IsEncrypted, and the Type is just being introduced, we can't
            // enforce a strict consistency of the type and 'encrypted' flag, only that this isn't an
            // "encrypted secret reference"; that is, any combination of type = {'plaintext', 'encrypted'},
            // and IsEncrypted = {true, false} is valid; isEncrypted and type == 'secretsStoreRef' is not. 
            bool isSecretReference = (driverOptionType == EnvironmentVariableTypeType.SecretsStoreRef);

            if (isEncrypted && isSecretReference)
            {
                ImageBuilder.TraceSource.WriteError(
                    TraceType,
                    "Driver option '{0}' cannot be both encrypted and declared as a secret reference.",
                    driverOption.Name);

                throw new FabricImageBuilderValidationException(
                    string.Format(
                        System.Globalization.CultureInfo.InvariantCulture,
                        StringResources.ImageBuilderError_InvalidValue,
                        driverOption.Name,
                        driverOption.Type));
            }

            if ((isEncrypted || isSecretReference)
                && String.IsNullOrWhiteSpace(driverOption.Value))
            {
                ImageBuilder.TraceSource.WriteError(
                    TraceType,
                    "Driver option '{0}' is either encrypted or declared as a secret reference, and may not be empty.",
                    driverOption.Name);

                throw new FabricImageBuilderValidationException(
                    string.Format(
                        System.Globalization.CultureInfo.InvariantCulture,
                        StringResources.ImageBuilderError_InvalidValue,
                        driverOption.Name,
                        driverOption.Type));
            }
        }

        private bool CheckIsValidBoolean(string key)
        {
            bool flag;
            if (!Boolean.TryParse(key, out flag))
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                      TraceType,
                      StringResources.ImageBuilder_InvalidBooleanValue,
                      key);
            }

            return flag;
        }

        private void ApplyParameters(RunAsPolicyType runAsPolicy)
        {
            if (runAsPolicy != null)
            {
                runAsPolicy.UserRef = this.ApplyParameters(runAsPolicy.UserRef);
            }
        }

        private void ApplyParameters()
        {
            
        }

        private void ApplyParameters(SecurityAccessPolicyType securityAccessPolicy)
        {
            if (securityAccessPolicy != null)
            {
                securityAccessPolicy.PrincipalRef = this.ApplyParameters(securityAccessPolicy.PrincipalRef);
            }
        }

        private void ApplyParameters(EnvironmentType applicationEnvironment)
        {
            if (applicationEnvironment == null)
            {
                return;
            }

            // Apply parameters for Application Principals defined
            if (applicationEnvironment.Principals != null)
            {
                if (applicationEnvironment.Principals.Users != null)
                {
                    foreach (SecurityPrincipalsTypeUser securityPrincipalUser in applicationEnvironment.Principals.Users)
                    {
                        securityPrincipalUser.Name = this.ApplyParameters(securityPrincipalUser.Name);
                        securityPrincipalUser.AccountName = this.ApplyParameters(securityPrincipalUser.AccountName);
                        securityPrincipalUser.Password = this.ApplyParameters(securityPrincipalUser.Password);
                       
                        if (securityPrincipalUser.MemberOf != null)
                        {
                            foreach (object memberOf in securityPrincipalUser.MemberOf)
                            {
                                if (memberOf.GetType() == typeof(SecurityPrincipalsTypeUserGroup))
                                {
                                    ((SecurityPrincipalsTypeUserGroup)memberOf).NameRef = this.ApplyParameters(((SecurityPrincipalsTypeUserGroup)memberOf).NameRef);
                                }
                                else if (memberOf.GetType() == typeof(SecurityPrincipalsTypeUserSystemGroup))
                                {
                                    ((SecurityPrincipalsTypeUserSystemGroup)memberOf).Name = this.ApplyParameters(((SecurityPrincipalsTypeUserSystemGroup)memberOf).Name);
                                }
                                else
                                {
                                    System.Fabric.Interop.Utility.ReleaseAssert(true, "Invalid MemberOf type");                                    
                                }
                            }
                        }
                    }
                }

                if (applicationEnvironment.Principals.Groups != null)
                {
                    foreach (SecurityPrincipalsTypeGroup securityPrincipalGroup in applicationEnvironment.Principals.Groups)
                    {
                        securityPrincipalGroup.Name = this.ApplyParameters(securityPrincipalGroup.Name);

                        if (securityPrincipalGroup.Membership != null)
                        {
                            foreach (object membership in securityPrincipalGroup.Membership)
                            {
                                if (membership.GetType() == typeof(SecurityPrincipalsTypeGroupDomainGroup))
                                {
                                    ((SecurityPrincipalsTypeGroupDomainGroup)membership).Name = this.ApplyParameters(((SecurityPrincipalsTypeGroupDomainGroup)membership).Name);
                                }
                                else if (membership.GetType() == typeof(SecurityPrincipalsTypeGroupDomainUser))
                                {
                                    ((SecurityPrincipalsTypeGroupDomainUser)membership).Name = this.ApplyParameters(((SecurityPrincipalsTypeGroupDomainUser)membership).Name);
                                }
                                else if (membership.GetType() == typeof(SecurityPrincipalsTypeGroupSystemGroup))
                                {
                                    ((SecurityPrincipalsTypeGroupSystemGroup)membership).Name = this.ApplyParameters(((SecurityPrincipalsTypeGroupSystemGroup)membership).Name);
                                }
                                else
                                {
                                    System.Fabric.Interop.Utility.ReleaseAssert(true, "Invalid Membership type");                                                                        
                                }
                            }
                        }
                    }
                }
            }

            // Apply parameters on Application Policies defined
            if (applicationEnvironment.Policies != null )
            {
                if (applicationEnvironment.Policies.DefaultRunAsPolicy != null)
                {
                    applicationEnvironment.Policies.DefaultRunAsPolicy.UserRef = this.ApplyParameters(applicationEnvironment.Policies.DefaultRunAsPolicy.UserRef);
                }
                if(applicationEnvironment.Policies.SecurityAccessPolicies != null)
                {
                    foreach (SecurityAccessPolicyType accessPolicy in applicationEnvironment.Policies.SecurityAccessPolicies.SecurityAccessPolicy)
                    {
                        this.ApplyParameters(accessPolicy);
                    }
                }
            }

            // Apply parameters for diagnostics
            if (applicationEnvironment.Diagnostics != null)
            {
                this.ApplyParameters(applicationEnvironment.Diagnostics);
            }
        }

        private void ApplyParameters(DiagnosticsType diagnostics)
        {
            if (diagnostics.CrashDumpSource != null)
            {
                diagnostics.CrashDumpSource.IsEnabled = this.ApplyParameters(diagnostics.CrashDumpSource.IsEnabled);
                if (diagnostics.CrashDumpSource.Destinations != null)
                {
                    this.ApplyParameters(diagnostics.CrashDumpSource.Destinations.AzureBlob);
                    this.ApplyParameters(diagnostics.CrashDumpSource.Destinations.FileStore);
                    this.ApplyParameters(diagnostics.CrashDumpSource.Destinations.LocalStore);
                }

                this.ApplyParameters(diagnostics.CrashDumpSource.Parameters);
            }
            if (diagnostics.ETWSource != null)
            {
                diagnostics.ETWSource.IsEnabled = this.ApplyParameters(diagnostics.ETWSource.IsEnabled);
                if (diagnostics.ETWSource.Destinations != null)
                {
                    this.ApplyParameters(diagnostics.ETWSource.Destinations.AzureBlob);
                    this.ApplyParameters(diagnostics.ETWSource.Destinations.FileStore);
                    this.ApplyParameters(diagnostics.ETWSource.Destinations.LocalStore);
                }

                this.ApplyParameters(diagnostics.ETWSource.Parameters);
            }
            if (diagnostics.FolderSource != null)
            {
                foreach (DiagnosticsTypeFolderSource folderSource in diagnostics.FolderSource)
                {
                    folderSource.IsEnabled = this.ApplyParameters(folderSource.IsEnabled);
                    folderSource.RelativeFolderPath = this.ApplyParameters(folderSource.RelativeFolderPath);
                    folderSource.DataDeletionAgeInDays = this.ApplyParameters(folderSource.DataDeletionAgeInDays);
                    if (folderSource.Destinations != null)
                    {
                        this.ApplyParameters(folderSource.Destinations.AzureBlob);
                        this.ApplyParameters(folderSource.Destinations.FileStore);
                        this.ApplyParameters(folderSource.Destinations.LocalStore);
                    }

                    this.ApplyParameters(folderSource.Parameters);
                }
            }
        }

        private void ApplyParameters(AzureStoreBaseType azureStore)
        {
            azureStore.IsEnabled = this.ApplyParameters(azureStore.IsEnabled);
            azureStore.ConnectionString = this.ApplyParameters(azureStore.ConnectionString);
            azureStore.ConnectionStringIsEncrypted = this.ApplyParameters(azureStore.ConnectionStringIsEncrypted);
            azureStore.UploadIntervalInMinutes = this.ApplyParameters(azureStore.UploadIntervalInMinutes);
            azureStore.DataDeletionAgeInDays = this.ApplyParameters(azureStore.DataDeletionAgeInDays);
            this.ApplyParameters(azureStore.Parameters);
        }

        private void ApplyParameters(AzureBlobType azureBlob)
        {
            this.ApplyParameters((AzureStoreBaseType)azureBlob);
            azureBlob.ContainerName = this.ApplyParameters(azureBlob.ContainerName);
        }

        private void ApplyParameters(AzureBlobType[] azureBlobs)
        {
            if (azureBlobs == null)
            {
                return;
            }
            foreach (AzureBlobType azureBlob in azureBlobs)
            {
                this.ApplyParameters(azureBlob);
            }
        }

        private void ApplyParameters(AzureBlobETWType[] azureBlobs)
        {
            if (azureBlobs == null)
            {
                return;
            }
            foreach (AzureBlobETWType azureBlob in azureBlobs)
            {
                this.ApplyParameters((AzureBlobType)azureBlob);
                azureBlob.LevelFilter = this.ApplyParameters(azureBlob.LevelFilter);
            }
        }

        private void ApplyParameters(FileStoreType fileStore)
        {
            fileStore.IsEnabled = this.ApplyParameters(fileStore.IsEnabled);
            fileStore.Path = this.ApplyParameters(fileStore.Path);
            fileStore.UploadIntervalInMinutes = this.ApplyParameters(fileStore.UploadIntervalInMinutes);
            fileStore.DataDeletionAgeInDays = this.ApplyParameters(fileStore.DataDeletionAgeInDays);
            fileStore.AccountType = this.ApplyParameters(fileStore.AccountType);
            fileStore.AccountName = this.ApplyParameters(fileStore.AccountName);
            fileStore.Password = this.ApplyParameters(fileStore.Password);
            fileStore.PasswordEncrypted = this.ApplyParameters(fileStore.PasswordEncrypted);
            this.ApplyParameters(fileStore.Parameters);
        }

        private void ApplyParameters(FileStoreType[] fileStores)
        {
            if (fileStores == null)
            {
                return;
            }
            foreach (FileStoreType fileStore in fileStores)
            {
                this.ApplyParameters(fileStore);
            }
        }

        private void ApplyParameters(FileStoreETWType[] fileStores)
        {
            if (fileStores == null)
            {
                return;
            }
            foreach (FileStoreETWType fileStore in fileStores)
            {
                this.ApplyParameters((FileStoreType)fileStore);
                fileStore.LevelFilter = this.ApplyParameters(fileStore.LevelFilter);
            }
        }

        private void ApplyParameters(LocalStoreType localStore)
        {
            localStore.IsEnabled = this.ApplyParameters(localStore.IsEnabled);
            localStore.RelativeFolderPath = this.ApplyParameters(localStore.RelativeFolderPath);
            localStore.DataDeletionAgeInDays = this.ApplyParameters(localStore.DataDeletionAgeInDays);
            this.ApplyParameters(localStore.Parameters);
        }

        private void ApplyParameters(LocalStoreType[] localStores)
        {
            if (localStores == null)
            {
                return;
            }
            foreach (LocalStoreType localStore in localStores)
            {
                this.ApplyParameters(localStore);
            }
            return;
        }

        private void ApplyParameters(LocalStoreETWType[] localStores)
        {
            if (localStores == null)
            {
                return;
            }
            foreach (LocalStoreETWType localStore in localStores)
            {
                this.ApplyParameters((LocalStoreType)localStore);
                localStore.LevelFilter = this.ApplyParameters(localStore.LevelFilter);
            }
            return;
        }

        private void ApplyParameters(ParameterType[] parameters)
        {
            if (parameters == null)
            {
                return;
            }
            foreach (ParameterType parameter in parameters)
            {
                parameter.Value = this.ApplyParameters(parameter.Value);
                parameter.IsEncrypted = this.ApplyParameters(parameter.IsEncrypted);
            }
        }

        private void ApplyParameters(ApplicationPackageTypeDigestedCertificates digestedCertificates)
        {
            if (digestedCertificates == null)
            {
                return;
            }

            if (digestedCertificates.SecretsCertificate != null)
            {
                foreach (FabricCertificateType certificate in digestedCertificates.SecretsCertificate)
                {
                    certificate.X509StoreName = this.ApplyParameters(certificate.X509StoreName);
                    certificate.X509FindValue = this.ApplyParameters(certificate.X509FindValue);
                }
            }
            if (digestedCertificates.EndpointCertificate != null)
            {
                foreach (EndpointCertificateType certificate in digestedCertificates.EndpointCertificate)
                {
                    certificate.X509StoreName = this.ApplyParameters(certificate.X509StoreName);
                    certificate.X509FindValue = this.ApplyParameters(certificate.X509FindValue);
                } 
            }
        }

        private void ApplyParameters(ServiceType serviceType)
        {
            if (serviceType == null)
            {
                return;
            }

            serviceType.PlacementConstraints = this.ApplyParameters(serviceType.PlacementConstraints);

            bool isStatefulService = serviceType is StatefulServiceType;

            if (isStatefulService)
            {
                StatefulServiceType statefulServiceType = (StatefulServiceType)serviceType;

                statefulServiceType.MinReplicaSetSize = this.ApplyParameters(statefulServiceType.MinReplicaSetSize);
                statefulServiceType.TargetReplicaSetSize = this.ApplyParameters(statefulServiceType.TargetReplicaSetSize);
                statefulServiceType.ReplicaRestartWaitDurationSeconds = this.ApplyParameters(statefulServiceType.ReplicaRestartWaitDurationSeconds);
                statefulServiceType.QuorumLossWaitDurationSeconds = this.ApplyParameters(statefulServiceType.QuorumLossWaitDurationSeconds);
                statefulServiceType.StandByReplicaKeepDurationSeconds = this.ApplyParameters(statefulServiceType.StandByReplicaKeepDurationSeconds);
            }
            else
            {
                StatelessServiceType statelessServiceType = (StatelessServiceType)serviceType;
                statelessServiceType.InstanceCount = this.ApplyParameters(statelessServiceType.InstanceCount);
            }

            ApplyParameters(serviceType.ServiceScalingPolicies);

            if (serviceType.UniformInt64Partition != null)
            {
                serviceType.UniformInt64Partition.HighKey = this.ApplyParameters(serviceType.UniformInt64Partition.HighKey);
                serviceType.UniformInt64Partition.LowKey = this.ApplyParameters(serviceType.UniformInt64Partition.LowKey);
                serviceType.UniformInt64Partition.PartitionCount = this.ApplyParameters(serviceType.UniformInt64Partition.PartitionCount);
            }
            else if (serviceType.NamedPartition != null)
            {
                foreach (var p in serviceType.NamedPartition.Partition)
                {
                    p.Name = this.ApplyParameters(p.Name);
                }
            }
        }

        private void ApplyParameters(ScalingPolicyType[] scalingPolicies)
        {
            if (scalingPolicies == null)
            {
                return;
            }
            for (int i = 0; i < scalingPolicies.Length; ++i)
            {
                if (scalingPolicies[i] != null)
                {
                    if (scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism != null)
                    {
                        scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism.ScaleIncrement = this.ApplyParameters(scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism.ScaleIncrement);
                        scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism.MinPartitionCount = this.ApplyParameters(scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism.MinPartitionCount);
                        scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism.MaxPartitionCount = this.ApplyParameters(scalingPolicies[i].AddRemoveIncrementalNamedPartitionScalingMechanism.MaxPartitionCount);
                    }
                    else if (scalingPolicies[i].InstanceCountScalingMechanism != null)
                    {
                        scalingPolicies[i].InstanceCountScalingMechanism.ScaleIncrement = this.ApplyParameters(scalingPolicies[i].InstanceCountScalingMechanism.ScaleIncrement);
                        scalingPolicies[i].InstanceCountScalingMechanism.MinInstanceCount = this.ApplyParameters(scalingPolicies[i].InstanceCountScalingMechanism.MinInstanceCount);
                        scalingPolicies[i].InstanceCountScalingMechanism.MaxInstanceCount = this.ApplyParameters(scalingPolicies[i].InstanceCountScalingMechanism.MaxInstanceCount);
                    }

                    if (scalingPolicies[i].AveragePartitionLoadScalingTrigger != null)
                    {
                        scalingPolicies[i].AveragePartitionLoadScalingTrigger.MetricName = this.ApplyParameters(scalingPolicies[i].AveragePartitionLoadScalingTrigger.MetricName);
                        scalingPolicies[i].AveragePartitionLoadScalingTrigger.LowerLoadThreshold = this.ApplyParameters(scalingPolicies[i].AveragePartitionLoadScalingTrigger.LowerLoadThreshold);
                        scalingPolicies[i].AveragePartitionLoadScalingTrigger.UpperLoadThreshold = this.ApplyParameters(scalingPolicies[i].AveragePartitionLoadScalingTrigger.UpperLoadThreshold);
                        scalingPolicies[i].AveragePartitionLoadScalingTrigger.ScaleIntervalInSeconds = this.ApplyParameters(scalingPolicies[i].AveragePartitionLoadScalingTrigger.ScaleIntervalInSeconds);
                    }
                    else if (scalingPolicies[i].AverageServiceLoadScalingTrigger != null)
                    {
                        scalingPolicies[i].AverageServiceLoadScalingTrigger.MetricName = this.ApplyParameters(scalingPolicies[i].AverageServiceLoadScalingTrigger.MetricName);
                        scalingPolicies[i].AverageServiceLoadScalingTrigger.LowerLoadThreshold = this.ApplyParameters(scalingPolicies[i].AverageServiceLoadScalingTrigger.LowerLoadThreshold);
                        scalingPolicies[i].AverageServiceLoadScalingTrigger.UpperLoadThreshold = this.ApplyParameters(scalingPolicies[i].AverageServiceLoadScalingTrigger.UpperLoadThreshold);
                        scalingPolicies[i].AverageServiceLoadScalingTrigger.ScaleIntervalInSeconds = this.ApplyParameters(scalingPolicies[i].AverageServiceLoadScalingTrigger.ScaleIntervalInSeconds);
                        scalingPolicies[i].AverageServiceLoadScalingTrigger.UseOnlyPrimaryLoad = this.ApplyParameters(scalingPolicies[i].AverageServiceLoadScalingTrigger.UseOnlyPrimaryLoad);
                    }
                }
            }
        }

        private void ApplyParameters(ConfigOverrideType configOVerrideType)
        {
            if (configOVerrideType != null && configOVerrideType.Settings != null)
            {
                foreach (SettingsOverridesTypeSection settingsOverridesSection in configOVerrideType.Settings)
                {
                    if (settingsOverridesSection.Parameter != null)
                    {
                        foreach (SettingsOverridesTypeSectionParameter overrideParameter in settingsOverridesSection.Parameter)
                        {
                            overrideParameter.Value = this.ApplyParameters(overrideParameter.Value);
                            if(!String.IsNullOrEmpty(overrideParameter.Type))
                            {
                                overrideParameter.Type = this.ApplyParameters(overrideParameter.Type);
                                VerifySourceLocation(overrideParameter.Name, "ConfigOverrides", overrideParameter.Type);
                            }
                        }
                    }
                }
            }
        }

        private void ApplyParameters(ResourceOverridesType resourceOverrides)
        {
            if (resourceOverrides != null)
            {
                if (resourceOverrides.Endpoints != null)
                {
                    foreach (EndpointOverrideType endpoint in resourceOverrides.Endpoints)
                    {
                        ImageBuilder.TraceSource.WriteInfo(
    TraceType,
    "Ignoring the termination {0}",
    endpoint.Name);
                        endpoint.Name = this.ApplyParameters(endpoint.Name);
                        endpoint.Port = this.ApplyParameters(endpoint.Port);
                        endpoint.Protocol = this.ApplyParameters(endpoint.Protocol);
                        endpoint.Type = this.ApplyParameters(endpoint.Type);
                        endpoint.UriScheme = this.ApplyParameters(endpoint.UriScheme);
                        endpoint.PathSuffix = this.ApplyParameters(endpoint.PathSuffix);
                    }
                }
            }
        }

        private void ApplyParameters(ServicePackageResourceGovernancePolicyType spResourceGovernancePolicy)
        {
            if (spResourceGovernancePolicy == null)
            {
                return;
            }
            spResourceGovernancePolicy.CpuCores = ApplyParameters(spResourceGovernancePolicy.CpuCores);
            spResourceGovernancePolicy.MemoryInMB = ApplyParameters(spResourceGovernancePolicy.MemoryInMB);
        }

        private void ApplyParameters(ResourceGovernancePolicyType resourceGovernancePolicy)
        {
            if(resourceGovernancePolicy == null)
            {
                return;
            }
            resourceGovernancePolicy.CpuShares = this.ApplyParameters(resourceGovernancePolicy.CpuShares);
            resourceGovernancePolicy.CpuPercent = this.ApplyParameters(resourceGovernancePolicy.CpuPercent);
            resourceGovernancePolicy.MemoryInMB = this.ApplyParameters(resourceGovernancePolicy.MemoryInMB);
            resourceGovernancePolicy.MemorySwapInMB = this.ApplyParameters(resourceGovernancePolicy.MemorySwapInMB);
            resourceGovernancePolicy.MemoryReservationInMB = this.ApplyParameters(resourceGovernancePolicy.MemoryReservationInMB);
            resourceGovernancePolicy.MaximumIOps = this.ApplyParameters(resourceGovernancePolicy.MaximumIOps);
            resourceGovernancePolicy.MaximumIOBandwidth = this.ApplyParameters(resourceGovernancePolicy.MaximumIOBandwidth);
            resourceGovernancePolicy.BlockIOWeight = this.ApplyParameters(resourceGovernancePolicy.BlockIOWeight);
            resourceGovernancePolicy.DiskQuotaInMB = this.ApplyParameters(resourceGovernancePolicy.DiskQuotaInMB);
            resourceGovernancePolicy.KernelMemoryInMB = this.ApplyParameters(resourceGovernancePolicy.KernelMemoryInMB);
            resourceGovernancePolicy.ShmSizeInMB = this.ApplyParameters(resourceGovernancePolicy.ShmSizeInMB);

        }

        private void ApplyParameters(ContainerNetworkPolicyType[] containerNetworkPolicies)
        {
            if (containerNetworkPolicies == null)
            {
                return;
            }

            foreach (var containerNetworkPolicy in containerNetworkPolicies)
            {
                 containerNetworkPolicy.NetworkRef = this.ApplyParameters(containerNetworkPolicy.NetworkRef);
            }
        }

        private string ApplyParameters(string value)
        {
            string returnValue = value;

            if (!string.IsNullOrEmpty(value) && value.Length > 2)
            {
                if (value[0] == '[' && value[value.Length - 1] == ']')
                {
                    string key = value.Substring(1, value.Length - 2);
                    if (!this.parameters.TryGetValue(key, out returnValue))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationError(
                            TraceType,
                            StringResources.ImageBuilderError_CouldNotResolveParameter,
                            key);
                    }

                    // Unspecified parameters are legal since they may be user application parameters.
                    // Validation performed later will detect errors in the digested manifest.
                }
            }

            return returnValue;
        }

        private string ValidateIsolationParameter(string isolation)
        {
            if (String.IsNullOrEmpty(isolation) || isolation.Equals("default", StringComparison.OrdinalIgnoreCase))
            {
                isolation = "process";
            }
            else
            {
                isolation = isolation.ToLower();
                if (!isolation.Equals("process", StringComparison.Ordinal) &&
                    !isolation.Equals("hyperv", StringComparison.Ordinal))
                {
                    ImageBuilderUtility.TraceAndThrowValidationError(
                   TraceType,
                   StringResources.ImageBuilderError_InvalidValue,
                   "Isolation",
                   isolation);
                }
            }
            return isolation;
        }
    }
}
