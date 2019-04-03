// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Interop;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;

    internal class ApplicationManifestValidator : IApplicationValidator
    {
        private static readonly string TraceType = "ApplicationManifestValidator";

        private bool IsSFVolumeDiskServiceEnabled
        {
            get; set;
        }

        public void Validate(ApplicationTypeContext applicationTypeContext, bool isComposeDeployment, bool isSFVolumeDiskServiceEnabled)
        {
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Validating ApplicationManifest. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}.",
                applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                applicationTypeContext.ApplicationManifest.ApplicationTypeVersion);

            if (!ManifestValidatorUtility.IsValidName(applicationTypeContext.ApplicationManifest.ApplicationTypeName, isComposeDeployment))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    applicationTypeContext.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidName_arg2_prefix,
                    "ApplicationTypeName",
                    applicationTypeContext.ApplicationManifest.ApplicationTypeName,
                    Path.DirectorySeparatorChar,
                    StringConstants.DoubleDot,
                    StringConstants.ComposeDeploymentTypePrefix);
            }

            if (!ManifestValidatorUtility.IsValidVersion(applicationTypeContext.ApplicationManifest.ApplicationTypeVersion))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    applicationTypeContext.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidVersion_arg2,
                    "ApplicationTypeVersion",
                    applicationTypeContext.ApplicationManifest.ApplicationTypeVersion,
                    Path.DirectorySeparatorChar,
                    StringConstants.DoubleDot);
            }

            // Set the flag indicate of SFVolumeDisk cluster support so that it can be used during validation
            this.IsSFVolumeDiskServiceEnabled = isSFVolumeDiskServiceEnabled;

            this.ValidatePrincipals(applicationTypeContext.ApplicationManifest.Principals, applicationTypeContext.GetApplicationManifestFileName());
            this.ValidateApplicationPolicies(applicationTypeContext);

            this.ValidateServiceManifestImports(applicationTypeContext);

            this.ValidateDefaultServices(applicationTypeContext);
            this.ValidateServiceTemplates(applicationTypeContext);

        }

        public void Validate(ApplicationInstanceContext applicationInstanceContext)
        {
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Validating ApplicationInstance. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}.",
                applicationInstanceContext.ApplicationInstance.ApplicationTypeName,
                applicationInstanceContext.ApplicationInstance.ApplicationTypeVersion);

            BuildLayoutSpecification buildLayoutSpecification = BuildLayoutSpecification.Create();
            string applicationManifestFile = buildLayoutSpecification.GetApplicationManifestFile();
            List<ResourceGovernancePolicyType> resourceGovernancePolicies = new List<ResourceGovernancePolicyType>();

            if (applicationInstanceContext.ApplicationInstance.DefaultServices != null && applicationInstanceContext.ApplicationInstance.DefaultServices.Items != null)
            {
                foreach (object defaultServiceItem in applicationInstanceContext.ApplicationInstance.DefaultServices.Items)
                {
                    DefaultServicesTypeService defaultService = defaultServiceItem as DefaultServicesTypeService;
                    if (defaultService != null)
                    {
                        this.ValidateServiceType(defaultService.Item, applicationInstanceContext.MergedApplicationParameters, applicationManifestFile);
                    }
                    else
                    {
                        DefaultServicesTypeServiceGroup defaultServiceGroup = defaultServiceItem as DefaultServicesTypeServiceGroup;
                        if (defaultServiceGroup != null)
                        {
                            this.ValidateServiceGroupType(defaultServiceGroup.Item, applicationInstanceContext.MergedApplicationParameters, applicationManifestFile);
                        }
                    }
                }
            }

            if (applicationInstanceContext.ApplicationInstance.ServiceTemplates != null)
            {
                foreach (ServiceType serviceTemplate in applicationInstanceContext.ApplicationInstance.ServiceTemplates)
                {
                    this.ValidateServiceType(serviceTemplate, applicationInstanceContext.MergedApplicationParameters, applicationManifestFile);
                }
            }
            if (applicationInstanceContext.ApplicationPackage.DigestedEnvironment.Principals != null)
            {
                this.ValidatePrincipals(applicationInstanceContext.ApplicationPackage.DigestedEnvironment.Principals, applicationManifestFile);
            }
            foreach (var servicePackage in applicationInstanceContext.ServicePackages)
            {
                foreach (var digestedCodePackage in servicePackage.DigestedCodePackage)
                {
                    if (digestedCodePackage.ResourceGovernancePolicy != null)
                    {
                        resourceGovernancePolicies.Add(digestedCodePackage.ResourceGovernancePolicy);
                        ValidateResourceGovernanceSettings(digestedCodePackage.ResourceGovernancePolicy, applicationInstanceContext.MergedApplicationParameters, applicationManifestFile);
                    }
                }
                if (servicePackage.ServicePackageResourceGovernancePolicy != null)
                {
                    ValidateServicePackageResourceGovernancePolicy(servicePackage.ServicePackageResourceGovernancePolicy, applicationInstanceContext.MergedApplicationParameters, applicationManifestFile);
                }
                this.ValidateCodePackagesResourceGovernance(
                    resourceGovernancePolicies,
                    applicationInstanceContext.MergedApplicationParameters,
                    applicationManifestFile);
                resourceGovernancePolicies.Clear();

                ValidateServicePackageNetworkPolicies(servicePackage, applicationManifestFile);
            }
        }

        private void ValidatePrincipals(SecurityPrincipalsType principals, string applicationManifestName)
        {

            DuplicateDetector duplicatePrincipalNameDetector = new DuplicateDetector("Principal", "Name", applicationManifestName);

            if (principals != null)
            {
                if (principals.Groups != null)
                {
                    foreach (SecurityPrincipalsTypeGroup group in principals.Groups)
                    {
                        duplicatePrincipalNameDetector.Add(group.Name);

                        if (group.Name.Contains("|"))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                            TraceType,
                                            applicationManifestName,
                                            StringResources.ImageBuilderError_InvalidName_arg1,
                                            "GroupPrincipal",
                                            group.Name,
                                            "|");
                        }
                    }
                }

                if (principals.Users != null)
                {
                    foreach (SecurityPrincipalsTypeUser user in principals.Users)
                    {
                        if (user.MemberOf != null)
                        {
                            foreach (object membership in user.MemberOf)
                            {
                                if (membership.GetType() == typeof(SecurityPrincipalsTypeUserGroup))
                                {
                                    bool matchingGroupFound = false;

                                    if (principals.Groups != null)
                                    {
                                        SecurityPrincipalsTypeGroup matchingGroup = principals.Groups.FirstOrDefault(
                                            group => ImageBuilderUtility.Equals(group.Name, ((SecurityPrincipalsTypeUserGroup)membership).NameRef));

                                        matchingGroupFound = (matchingGroup != null) ? true : false;
                                    }

                                    if (!matchingGroupFound)
                                    {
                                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                            TraceType,
                                            applicationManifestName,
                                            StringResources.ImageBuilderError_ReferencingInvalidGroupPrincipal,
                                            user.Name,
                                            ((SecurityPrincipalsTypeUserGroup)membership).NameRef);
                                    }
                                }
                            }
                        }

                        duplicatePrincipalNameDetector.Add(user.Name);

                        if (user.Name.Contains("|"))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                            TraceType,
                                            applicationManifestName,
                                            StringResources.ImageBuilderError_InvalidName_arg1,
                                            "UserPrincipal",
                                            user.Name,
                                            "|");
                        }
                        if (user.AccountType == SecurityPrincipalsTypeUserAccountType.DomainUser)
                        {
                            if (String.IsNullOrEmpty(user.Password))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                            TraceType,
                                            applicationManifestName,
                                            StringResources.ImageBuilderError_PasswordMissingForDomainUser,
                                            user.Name);
                            }
                        }
                        else if (!String.IsNullOrEmpty(user.Password) ||
                                user.PasswordEncryptedSpecified)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                        TraceType,
                                        applicationManifestName,
                                        StringResources.ImageBuilderError_PasswordSpecifiedForNonDomainUser,
                                        user.Name);

                        }

                        if ((user.PerformInteractiveLogon || user.LoadUserProfile) &&
                            user.AccountType != SecurityPrincipalsTypeUserAccountType.DomainUser &&
                            user.AccountType != SecurityPrincipalsTypeUserAccountType.LocalUser)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                           TraceType,
                                           applicationManifestName,
                                           StringResources.ImageBuilderError_InvalidLogonOption,
                                           user.AccountType.ToString());
                        }

                        if (user.NTLMAuthenticationPolicy != null &&
                            user.NTLMAuthenticationPolicy.IsEnabled &&
                            user.AccountType != SecurityPrincipalsTypeUserAccountType.LocalUser)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                            TraceType,
                                            applicationManifestName,
                                            StringResources.ImageBuilderError_InvalidNTLMAuthenticationEnabled,
                                            user.Name,
                                            SecurityPrincipalsTypeUserAccountType.LocalUser);
                        }

                        if (user.AccountType == SecurityPrincipalsTypeUserAccountType.DomainUser ||
                           user.AccountType == SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount)
                        {
                            if (String.IsNullOrEmpty(user.AccountName))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                            TraceType,
                                            applicationManifestName,
                                            StringResources.ImageBuilderError_AccountNameNotSpecified,
                                            user.Name);
                                return;
                            }
                            if (!ImageBuilderUtility.IsValidAccountName(user.AccountName) &&
                                !ImageBuilderUtility.IsParameter(user.AccountName))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                   TraceType,
                                   applicationManifestName,
                                   StringResources.ImageBuilderError_InvalidAccountNameFormat,
                                   user.Name);
                            }
                        }
                    }
                }
            }
        }

        private void ValidateApplicationPolicies(ApplicationTypeContext context)
        {
            ApplicationPoliciesType applicationPolicies = context.ApplicationManifest.Policies;
            SecurityPrincipalsType applicationPrincipals = context.ApplicationManifest.Principals;

            if (applicationPolicies != null)
            {
                if (applicationPolicies.DefaultRunAsPolicy != null)
                {
                    bool isValid = this.IsValidUserRef(
                        applicationPolicies.DefaultRunAsPolicy.UserRef,
                        applicationPrincipals,
                        context.ApplicationParameters,
                        "DefaultRunAsPolicy",
                        context.GetApplicationManifestFileName());
                    if (!isValid)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_InvalidRef,
                            "UserRef",
                            applicationPolicies.DefaultRunAsPolicy.UserRef,
                            "DefaultRunAsPolicy",
                            "Principal",
                            "ApplicationManifest");
                    }
                }

                if (applicationPolicies.HealthPolicy != null)
                {
                    this.ValidateHealthPolicy(applicationPolicies.HealthPolicy, context);
                }

                if (applicationPolicies.SecurityAccessPolicies != null)
                {
                    this.ValidateSecurityAccessPolicies(applicationPolicies.SecurityAccessPolicies, context);
                }
            }
        }

        private void ValidateServiceManifestImports(ApplicationTypeContext context)
        {
            ApplicationManifestType applicationManifestType = context.ApplicationManifest;
            IEnumerable<ServiceManifestType> serviceManifestTypes = context.GetServiceManifestTypes();

            ApplicationManifestTypeServiceManifestImport[] serviceManifestImports = applicationManifestType.ServiceManifestImport;

            bool useNewStyleContainerNetworkConfig = false;
            bool useOldStyleContainerNetworkConfig = false;
            for (int i = 0; i < serviceManifestImports.Length; i++)
            {
                ApplicationManifestTypeServiceManifestImport serviceManifestImport = serviceManifestImports[i];
                ServiceManifestType serviceManifestType = serviceManifestTypes.ElementAt(i);
                // Validates that the Name in the ServiceManifestImport of ApplicationManifest matches the Name in the ServiceManifest
                if (!ImageBuilderUtility.Equals(serviceManifestImport.ServiceManifestRef.ServiceManifestName, serviceManifestType.Name))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidServiceManifestRef,
                        "Name",
                        serviceManifestImport.ServiceManifestRef.ServiceManifestName,
                        serviceManifestType.Name);
                }

                // Validates that the Version in the ServiceManifestImport of ApplicationManifest matches the Version in the ServiceManifest
                if (!ImageBuilderUtility.Equals(serviceManifestImport.ServiceManifestRef.ServiceManifestVersion, serviceManifestType.Version))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidServiceManifestRef,
                        "Version",
                        serviceManifestImport.ServiceManifestRef.ServiceManifestVersion,
                        serviceManifestType.Version);
                }

                // Validates the Policies in ServiceManifestImport
                if (serviceManifestImport.Policies != null)
                {
                    DuplicateDetector duplicateRunAsPolicy = new DuplicateDetector("RunAsPolicy", "CodePackageRef", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicateSecurityAccessPolicy = new DuplicateDetector("SecurityAccessPolicy", "ResourceRef", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicatePackageSharingPolicy = new DuplicateDetector("PackageSharingPolicy", "PackageRef", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicateEndpointBindingPolicy = new DuplicateDetector("EndpointBindingPolicy", "EndpointRef", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicateContainerPolicy = new DuplicateDetector("ContainerHostPolicies", "CodePackageRef", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicateResourceGovernancePolicy = new DuplicateDetector("ResourceGovernancePolicy", "CodePackageRef", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicateServicePackageResourceGovernancePolicyType = new DuplicateDetector("ServicePackageResourceGovernancePolicy", "ServicePackageResourceGovernancePolicy", context.GetApplicationManifestFileName());
                    DuplicateDetector duplicateNetworkPolicies = new DuplicateDetector("NetworkPolicies", "NetworkPolicies", context.GetApplicationManifestFileName());

                    List<RunAsPolicyType> runasPolicies = new List<RunAsPolicyType>();
                    List<ContainerHostPoliciesType> containerPolicies = new List<ContainerHostPoliciesType>();
                    List<ResourceGovernancePolicyType> resourceGovernancePolicies = new List<ResourceGovernancePolicyType>();

                    foreach (object policy in serviceManifestImport.Policies)
                    {
                        if (policy.GetType() == typeof(RunAsPolicyType))
                        {
                            RunAsPolicyType runasPolicy = (RunAsPolicyType)policy;
                            try
                            {
                                //If the entrypointtype is all we should add duplicate detection for both setup and main entry point.
                                if (runasPolicy.EntryPointType == RunAsPolicyTypeEntryPointType.All)
                                {
                                    duplicateRunAsPolicy.Add(runasPolicy.CodePackageRef + RunAsPolicyTypeEntryPointType.Main);
                                    duplicateRunAsPolicy.Add(runasPolicy.CodePackageRef + RunAsPolicyTypeEntryPointType.Setup);
                                }
                                else
                                {
                                    duplicateRunAsPolicy.Add(runasPolicy.CodePackageRef + runasPolicy.EntryPointType);
                                }
                            }
                            catch (FabricImageBuilderValidationException)
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    context.GetApplicationManifestFileName(),
                                    StringResources.ImageBuilderError_RunasPolicyCount,
                                    runasPolicy.CodePackageRef);

                            }
                            runasPolicies.Add(runasPolicy);

                        }
                        else if (policy.GetType() == typeof(SecurityAccessPolicyType))
                        {
                            duplicateSecurityAccessPolicy.Add(((SecurityAccessPolicyType)policy).ResourceRef);
                            this.ValidateServiceSecurityAccessPolicy((SecurityAccessPolicyType)policy, serviceManifestType, context);
                        }
                        else if (policy.GetType() == typeof(PackageSharingPolicyType))
                        {
                            duplicatePackageSharingPolicy.Add(((PackageSharingPolicyType)policy).PackageRef);
                            this.ValidatePackageSharingPolicy((PackageSharingPolicyType)policy, serviceManifestType, context);
                        }
                        else if (policy.GetType() == typeof(EndpointBindingPolicyType))
                        {
                            duplicateEndpointBindingPolicy.Add(((EndpointBindingPolicyType)policy).EndpointRef);
                            this.ValidateEndpointBindingPolicies((EndpointBindingPolicyType)policy, serviceManifestType, context, serviceManifestImport.ResourceOverrides);
                        }
                        else if (policy.GetType() == typeof(ContainerHostPoliciesType))
                        {
                            duplicateContainerPolicy.Add(((ContainerHostPoliciesType)policy).CodePackageRef);
                            containerPolicies.Add(policy as ContainerHostPoliciesType);
                        }
                        else if (policy.GetType() == typeof(ResourceGovernancePolicyType))
                        {
                            duplicateResourceGovernancePolicy.Add(((ResourceGovernancePolicyType)policy).CodePackageRef);
                            this.ValidateResourceGovernancePolicy(
                                (ResourceGovernancePolicyType)policy,
                                serviceManifestType,
                                context.ApplicationParameters,
                                context.GetApplicationManifestFileName());
                            resourceGovernancePolicies.Add(policy as ResourceGovernancePolicyType);
                        }
                        else if (policy.GetType() == typeof(ServicePackageResourceGovernancePolicyType))
                        {
                            duplicateServicePackageResourceGovernancePolicyType.Add(typeof(ServicePackageResourceGovernancePolicyType).Name);
                            this.ValidateServicePackageResourceGovernancePolicy((ServicePackageResourceGovernancePolicyType)policy,
                                context.ApplicationParameters,
                                context.GetApplicationManifestFileName());
                        }
                        else if (policy.GetType() == typeof(NetworkPoliciesType))
                        {
                            bool hasAnyContainerNetworkPolicy;
                            duplicateNetworkPolicies.Add(typeof(NetworkPoliciesType).Name);
                            this.ValidateApplicationManifestNetworkPolicies((NetworkPoliciesType)policy, serviceManifestType, context, out hasAnyContainerNetworkPolicy);
                            useNewStyleContainerNetworkConfig = useNewStyleContainerNetworkConfig || hasAnyContainerNetworkPolicy;
                        }
                        else if (policy.GetType() == typeof(ServiceFabricRuntimeAccessPolicyType))
                        {
                            // Service has specified to use the replicated store feature, so confirm if 
                            // SFVolumeDisk support is enabled in the cluster. If not, fail the validation.
                            ServiceFabricRuntimeAccessPolicyType runtimeAccessPolicy = policy as ServiceFabricRuntimeAccessPolicyType;
                            if (runtimeAccessPolicy.UseServiceFabricReplicatedStore)
                            {
                                if (!this.IsSFVolumeDiskServiceEnabled)
                                {
                                    // It is invalid to use replicated store attribute if SFVolumeDisk support is not enabled.
                                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    context.GetApplicationManifestFileName(),
                                    StringResources.ImageBuilderError_InvalidReplicatedStoreUsage);
                                }
                            }
                        }
                        else
                        {
                            System.Fabric.Interop.Utility.ReleaseAssert(true, "The policy type is not expected.");
                        }
                    }
                    if (runasPolicies.Count > 0)
                    {
                        this.ValidateServiceRunAsPolicy(runasPolicies, serviceManifestType, context);
                    }
                    if (containerPolicies.Count > 0)
                    {
                        bool hasAnyContainerNetworkConfig;
                        this.ValidateContainerHostPolicies(containerPolicies, serviceManifestType, context, out hasAnyContainerNetworkConfig);
                        useOldStyleContainerNetworkConfig = useOldStyleContainerNetworkConfig || hasAnyContainerNetworkConfig;
                    }

                    this.ValidateCodePackagesResourceGovernance(
                        resourceGovernancePolicies,
                        context.ApplicationParameters,
                        context.GetApplicationManifestFileName());
                }

                // Validate that new-style container network config via container network policies defined at service package level would not be used together 
                // with old-style container network config via network config in container host policies defined at code package level.
                // The validation is performed at application package level, i.e. in the entire application package old-style or new-style config has to be used consistently.
                if (useOldStyleContainerNetworkConfig && useNewStyleContainerNetworkConfig)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_NetworkConfigConflict);
                }

                // Validates the ConfigurationOverrides in the SeriverManifestImport
                this.ValidateConfigOverrides(serviceManifestImport.ConfigOverrides, serviceManifestImport.ServiceManifestRef.ServiceManifestName, context);

                this.ValidateEnvironmentOverrides(serviceManifestImport.EnvironmentOverrides, serviceManifestType, context);

                this.ValidateResourceOverrides(serviceManifestImport.ResourceOverrides, serviceManifestType, context);
            }
        }

        private void ValidateConfigOverrides(ConfigOverrideType[] configOverrides, string serviceManifestName, ApplicationTypeContext context)
        {
            if (configOverrides != null)
            {
                // Gets the Collection of ConfigurationPackages in the ServiceManifest
                IEnumerable<ConfigPackage> configurationPackageCollection = context.ServiceManifests.First(
                    serviceManifest => ImageBuilderUtility.Equals(serviceManifest.ServiceManifestType.Name, serviceManifestName)).ConfigPackages;

                DuplicateDetector configOverrideDuplicateDetector = new DuplicateDetector("ConfigOverride", "Name", context.GetApplicationManifestFileName());
                foreach (ConfigOverrideType configOverrideType in configOverrides)
                {
                    configOverrideDuplicateDetector.Add(configOverrideType.Name);

                    ConfigPackage matchingConfigurationPackage = configurationPackageCollection.FirstOrDefault<ConfigPackage>(
                        configurationPackage => ImageBuilderUtility.Equals(configurationPackage.ConfigPackageType.Name, configOverrideType.Name));

                    // This ConfigOverride in ApplicationManifest does not override any valid ConfigPackage for this Service.
                    if (matchingConfigurationPackage == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_ConfigOverrideDoesNotMatchConfigPackage,
                            configOverrideType.Name,
                            serviceManifestName);
                    }

                    this.ValidateConfigOverrideSettingsSection(configOverrideType.Settings, matchingConfigurationPackage, configOverrideType.Name, serviceManifestName, context);
                }
            }
        }

        private void ValidateConfigOverrideSettingsSection(SettingsOverridesTypeSection[] overrideSettingsSections, ConfigPackage matchingConfigurationPackage, string configOverrideName, string serviceManifestName, ApplicationTypeContext context)
        {
            if (overrideSettingsSections != null)
            {
                DuplicateDetector sectionDuplicateDetector = new DuplicateDetector("Section", "Name", context.GetApplicationManifestFileName());
                foreach (SettingsOverridesTypeSection overrideSettingTypeSection in overrideSettingsSections)
                {
                    sectionDuplicateDetector.Add(overrideSettingTypeSection.Name);

                    if (matchingConfigurationPackage.SettingsType == null)
                    {
                        continue;
                    }

                    SettingsTypeSection matchingSettingTypeSection = null;

                    if (matchingConfigurationPackage.SettingsType.Section != null)
                    {
                        matchingSettingTypeSection = matchingConfigurationPackage.SettingsType.Section.FirstOrDefault<SettingsTypeSection>(
                                        settingsTypeSection => ImageBuilderUtility.Equals(settingsTypeSection.Name, overrideSettingTypeSection.Name));
                    }

                    if (matchingSettingTypeSection == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_ConfigOverrideDoesNotMatchSectionName,
                            overrideSettingTypeSection.Name,
                            configOverrideName,
                            serviceManifestName);
                    }

                    this.ValidateConfigOverridesParameters(overrideSettingTypeSection.Parameter, matchingSettingTypeSection, serviceManifestName, context);
                }
            }
        }

        private void ValidateConfigOverridesParameters(SettingsOverridesTypeSectionParameter[] overrideParameters, SettingsTypeSection matchingSettingTypeSection, string serviceManifestName, ApplicationTypeContext context)
        {
            if (overrideParameters != null)
            {
                DuplicateDetector settingDuplicateDetector = new DuplicateDetector("Override Parameter", "Name", context.GetApplicationManifestFileName());
                foreach (SettingsOverridesTypeSectionParameter overrideParameter in overrideParameters)
                {
                    settingDuplicateDetector.Add(overrideParameter.Name);

                    SettingsTypeSectionParameter matchingParameter = null;
                    if (matchingSettingTypeSection.Parameter != null)
                    {
                        matchingParameter = matchingSettingTypeSection.Parameter.FirstOrDefault<SettingsTypeSectionParameter>(
                            setting => ImageBuilderUtility.Equals(setting.Name, overrideParameter.Name));
                    }

                    if (matchingParameter == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_ConfigOverrideDoesNotMatchParameterName,
                            overrideParameter.Name,
                            matchingSettingTypeSection.Name,
                            serviceManifestName);
                    }
                    else if (matchingParameter.IsEncrypted != overrideParameter.IsEncrypted)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_ConfigOverrideDoesNotMatchIsEncrypted,
                            overrideParameter.Name,
                            matchingSettingTypeSection.Name,
                            serviceManifestName);
                    }

                    ManifestValidatorUtility.IsValidParameter(overrideParameter.Value, "Value", context.ApplicationParameters, context.GetApplicationManifestFileName());
                }
            }
        }

        private void ValidateEnvironmentOverrides(
            EnvironmentOverridesType[] environmentOverrides,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context)
        {
            if (environmentOverrides == null)
            {
                return;
            }

            foreach (var envOverrides in environmentOverrides)
            {
                CodePackageType package = serviceManifestType.CodePackage.FirstOrDefault<CodePackageType>(
                    codePackageType => ImageBuilderUtility.Equals(codePackageType.Name, envOverrides.CodePackageRef));
                if (package == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidRef,
                        "CodePackageRef",
                        envOverrides.CodePackageRef,
                        "EnvironmentOverrides",
                        "CodePackageRef",
                        "ServiceManifest");

                    return;
                }

                foreach (EnvironmentVariableOverrideType envVar in envOverrides.EnvironmentVariable)
                {
                    if (package.EnvironmentVariables == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                              TraceType,
                              context.GetApplicationManifestFileName(),
                              StringResources.ImageBuilder_InvalidEnvVariableOverride,
                              envVar.Name,
                              envOverrides.CodePackageRef,
                              serviceManifestType.Name);

                        return;
                    }
                    var matchingEnvVariable = package.EnvironmentVariables.FirstOrDefault<EnvironmentVariableType>(
                        envVariable => ImageBuilderUtility.Equals(envVar.Name, envVariable.Name));
                    if (matchingEnvVariable == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilder_InvalidEnvVariableOverride,
                            envVar.Name,
                            envOverrides.CodePackageRef,
                            serviceManifestType.Name);
                    }
                }
            }
        }

        private void ValidateResourceOverrides(
            ResourceOverridesType resourceOverrides,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context)
        {
            if (resourceOverrides != null && resourceOverrides.Endpoints != null)
            {
                if (serviceManifestType.Resources != null && serviceManifestType.Resources.Endpoints != null)
                {
                    foreach (var endpointOverride in resourceOverrides.Endpoints)
                    {
                        EndpointType matchingResourceTypeEndpoint = serviceManifestType.Resources.Endpoints.FirstOrDefault<EndpointType>(
                                endpoint => ImageBuilderUtility.Equals(endpoint.Name, endpointOverride.Name));

                        if (matchingResourceTypeEndpoint == null)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                context.GetApplicationManifestFileName(),
                                StringResources.ImageBuilderError_InvalidRef,
                                "Endpoint",
                                endpointOverride.Name,
                                "ResourceOverrides",
                                "Endpoint",
                                "ServiceManifest Resources");
                        }
                    }
                }
            }
        }

        private void ValidateServiceSecurityAccessPolicy(
            SecurityAccessPolicyType securityAccessPolicy,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context)
        {
            bool matchingResourceFound = false;

            if (serviceManifestType.Resources != null && serviceManifestType.Resources.Endpoints != null)
            {
                EndpointType matchingResourceTypeEndpoint = serviceManifestType.Resources.Endpoints.FirstOrDefault<EndpointType>(
                    endpoint => ImageBuilderUtility.Equals(endpoint.Name, securityAccessPolicy.ResourceRef));

                matchingResourceFound = (matchingResourceTypeEndpoint != null) ? true : false;
            }

            if (!matchingResourceFound)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidRef,
                    "ResouceRef",
                    securityAccessPolicy.ResourceRef,
                    "SecurityAccessPolicy",
                    "Resource",
                    "ServiceManifest");
            }

            bool matchingPrincipalRefFound = this.IsValidUserRef(
                securityAccessPolicy.PrincipalRef,
                context.ApplicationManifest.Principals,
                context.ApplicationParameters,
                "SecurityAccessPolicy",
                context.GetApplicationManifestFileName());
            if (!matchingPrincipalRefFound)
            {
                if (!this.IsValidGroupRef(
                    securityAccessPolicy.PrincipalRef,
                    context.ApplicationManifest.Principals,
                    context.ApplicationParameters,
                    "SecurityAccessPolicy",
                    context.GetApplicationManifestFileName()))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidRef,
                        "PrincipalRef",
                        securityAccessPolicy.PrincipalRef,
                        "SecurityAccessPolicy",
                        "Principal",
                        "ApplicationManifest");
                }
            }
        }

        private void ValidatePackageSharingPolicy(
            PackageSharingPolicyType policy,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context)
        {
            object package = serviceManifestType.CodePackage?.FirstOrDefault<CodePackageType>(
                   codePackageType => ImageBuilderUtility.Equals(codePackageType.Name, policy.PackageRef));
            if (package != null)
            {
                CodePackageType codePackage = package as CodePackageType;
                EntryPointDescriptionTypeExeHost exeHost = codePackage.EntryPoint.Item as EntryPointDescriptionTypeExeHost;
                if (exeHost != null &&
                    (exeHost.WorkingFolder == ExeHostEntryPointTypeWorkingFolder.CodePackage ||
                    exeHost.WorkingFolder == ExeHostEntryPointTypeWorkingFolder.CodeBase))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                       TraceType,
                       context.GetApplicationManifestFileName(),
                       StringResources.ImageBuilderError_InvalidSharingWorkFolder,
                       codePackage.Name,
                       serviceManifestType.Name);
                }

                if (policy.Scope != PackageSharingPolicyTypeScope.Code &&
                      policy.Scope != PackageSharingPolicyTypeScope.None)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidPackageSharingScope,
                        codePackage.Name,
                        serviceManifestType.Name,
                        policy.Scope);
                }
            }
            else
            {
                package = serviceManifestType.ConfigPackage?.FirstOrDefault<ConfigPackageType>(
                   configPackageType => ImageBuilderUtility.Equals(configPackageType.Name, policy.PackageRef));
                if (package != null)
                {
                    if (policy.Scope != PackageSharingPolicyTypeScope.Config &&
                        policy.Scope != PackageSharingPolicyTypeScope.None)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_InvalidPackageSharingScope,
                            ((ConfigPackageType)package).Name,
                            serviceManifestType.Name,
                            policy.Scope);
                    }
                }
                else
                {
                    package = serviceManifestType.DataPackage?.FirstOrDefault<DataPackageType>(
                        dataPackageType => ImageBuilderUtility.Equals(dataPackageType.Name, policy.PackageRef));
                    if (package != null)
                    {
                        if (policy.Scope != PackageSharingPolicyTypeScope.Data &&
                        policy.Scope != PackageSharingPolicyTypeScope.None)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                context.GetApplicationManifestFileName(),
                                StringResources.ImageBuilderError_InvalidPackageSharingScope,
                                ((DataPackageType)package).Name,
                                serviceManifestType.Name,
                                policy.Scope);
                        }
                    }
                }
            }
            if (package == null && policy.Scope == PackageSharingPolicyTypeScope.None)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidRef,
                    "PackageRef",
                    policy.PackageRef,
                    "PackageSharingPolicy",
                    "Package",
                    "ServiceManifest");
            }
        }
        private void ValidateResourceGovernancePolicy(
          ResourceGovernancePolicyType resourceGovernancePolicy,
          ServiceManifestType serviceManifestType,
          IDictionary<string, string> parameters,
          string fileName)
        {
            CodePackageType matchingCodePackageType = serviceManifestType.CodePackage.FirstOrDefault<CodePackageType>(
                   codePackageType => ImageBuilderUtility.Equals(codePackageType.Name, resourceGovernancePolicy.CodePackageRef));

            if (matchingCodePackageType == null)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_InvalidRef,
                    "CodePackageRef",
                    resourceGovernancePolicy.CodePackageRef,
                    "ResourceGovernancePolicy",
                    "CodePackage",
                    "ServiceManifest");
            }
            ValidateResourceGovernanceSettings(resourceGovernancePolicy, parameters, fileName);

        }

        void ValidateResourceGovernanceSettings(
          ResourceGovernancePolicyType resourceGovernancePolicy,
          IDictionary<string, string> parameters,
          string fileName)
        {
            int? cpuShares = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.CpuShares, "CpuShares", parameters, fileName);
            int? cpuPercent = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.CpuPercent, "CpuPercent", parameters, fileName);
            if (cpuShares != null && cpuShares > 0 &&
               cpuPercent != null && cpuPercent > 0)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                  TraceType,
                  fileName,
                  StringResources.ResourceGovernanceIncompatibleSettings);
            }

            int? memoryLimit = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.MemoryInMB, "MemoryInMB", parameters, fileName);
            if (memoryLimit.HasValue && memoryLimit > 0 && memoryLimit < 4)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_InvalidMemoryInMB,
                    memoryLimit.Value);
            }

            long? memorySwap = ManifestValidatorUtility.ValidateAsLong(resourceGovernancePolicy.MemorySwapInMB, "MemorySwapMB", parameters, fileName);
            if (memorySwap.HasValue && memorySwap.Value < -1)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                TraceType,
                fileName,
                StringResources.ImageBuilderError_InvalidMemorySwapValueSpecified,
                memorySwap.Value,
                "-1");
            }

            int? memoryReservation = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.MemoryReservationInMB, "MemoryReservationInMB", parameters, fileName);
            int? maximumIOps = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.MaximumIOps, "MaximumIOps", parameters, fileName);
            int? maximumIOBps = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.MaximumIOBandwidth, "MaximumIOBandwidth", parameters, fileName);
            int? blockIOWeight = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.BlockIOWeight, "BlockIOWeight", parameters, fileName);
            if (blockIOWeight.HasValue &&
               blockIOWeight != 0 &&
               (blockIOWeight < 10 ||
               blockIOWeight > 1000))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                 TraceType,
                 fileName,
                 StringResources.ImageBuilderError_InvalidBlockIOWeight,
                 blockIOWeight,
                 "10",
                 "1000");
            }

            int? DiskQuotaInMB = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.DiskQuotaInMB, "DiskQuotaInMB", parameters, fileName);
            int? KernelMemoryInMB = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.KernelMemoryInMB, "KernelMemoryInMB", parameters, fileName);
            int? ShmSizeInMB = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(resourceGovernancePolicy.ShmSizeInMB, "ShmSizeInMB", parameters, fileName);
        }

        private void ValidateApplicationManifestNetworkPolicies(
          NetworkPoliciesType networkPolicies,
          ServiceManifestType serviceManifestType,
          ApplicationTypeContext context,
          out bool hasAnyContainerNetworkPolicy)
        {
            hasAnyContainerNetworkPolicy = false;

            // Validate container network policies during application type provisioning.
            // Perform all validations that do not require network references materialized. 
            if (networkPolicies != null && networkPolicies.Items != null)
            {
                HashSet<string> servicePackageEndpointNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                if (serviceManifestType.Resources != null && serviceManifestType.Resources.Endpoints != null)
                {
                    foreach (var servicePackageEndpoint in serviceManifestType.Resources.Endpoints)
                    {
                        servicePackageEndpointNames.Add(servicePackageEndpoint.Name);
                    }
                }

                foreach (var containerNetworkPolicy in networkPolicies.Items)
                {
                    if (string.IsNullOrWhiteSpace(containerNetworkPolicy.NetworkRef))
                    {
                        // The only validation on a referenced network name performed during application type provisioning would be the name cannot be empty or white-space only.
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_InvalidNetworkReference);
                    }

                    hasAnyContainerNetworkPolicy = true;

                    if (containerNetworkPolicy.Items != null)
                    {
                        foreach (var containerNetworkPolicyEndpointBinding in containerNetworkPolicy.Items)
                        {
                            // Validate that all endpoint references in container network policies are valid. Endpoint names are case-insensitive.
                            if (!servicePackageEndpointNames.Contains(containerNetworkPolicyEndpointBinding.EndpointRef))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    context.GetApplicationManifestFileName(),
                                    StringResources.ImageBuilderError_InvalidRef,
                                    "EndpointRef",
                                    containerNetworkPolicyEndpointBinding.EndpointRef,
                                    "ContainerNetworkPolicy",
                                    "Endpoint",
                                    "ServiceManifest");
                            }
                        }
                    }
                }
            }
        }

        private void ValidateServicePackageNetworkPolicies(
            ServicePackageType servicePackage,
            string applicationManifestFileName)
        {
            // Validate container network policies during application creation.
            // Perform all validations that require network references materialized. 
            if (servicePackage.NetworkPolicies != null)
            {
                // "Open" and "Nat" are used as case insensitive reserved network names.
                // Meanwhile names of general network resources are case sensitive like those of other high-level resources.
                // Therefore, for duplication detection we need to perform comparisons differently.
                DuplicateDetector duplicateOpenOrNatContainerNetworkPolicies = new DuplicateDetector("ContainerNetworkPolicy", "NetworkRef", applicationManifestFileName, true);
                DuplicateDetector duplicateLocalOrFederatedContainerNetworkPolicies = new DuplicateDetector("ContainerNetworkPolicy", "NetworkRef", applicationManifestFileName, false);

                bool useLocalOrFederatedNetwork = false;
                bool useOpenNetwork = false;
                bool useNatNework = false;
                Dictionary<string, string> servicePackageEndpointContainerNetworkMap = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                foreach (var containerNetworkPolicy in servicePackage.NetworkPolicies)
                {
                    bool isOpenNetworkReference = ImageBuilderUtility.IsOpenNetworkReference(containerNetworkPolicy.NetworkRef);
                    bool isNatNetworkReference = ImageBuilderUtility.IsNatNetworkReference(containerNetworkPolicy.NetworkRef);
                    useOpenNetwork = useOpenNetwork || isOpenNetworkReference;
                    useNatNework = useNatNework || isNatNetworkReference;
                    bool isLocalOrFederatedNetworkReference = !isOpenNetworkReference && !isNatNetworkReference;
                    useLocalOrFederatedNetwork = useLocalOrFederatedNetwork || isLocalOrFederatedNetworkReference;

                    // Validate that there can be no more than one network policy that refers to a specific network in a service package.
                    if (isLocalOrFederatedNetworkReference)
                    {
                        duplicateLocalOrFederatedContainerNetworkPolicies.Add(containerNetworkPolicy.NetworkRef);
                    }
                    else
                    {
                        duplicateOpenOrNatContainerNetworkPolicies.Add(containerNetworkPolicy.NetworkRef);
                    }

                    if (containerNetworkPolicy.Items != null)
                    {
                        foreach (var containerNetworkPolicyEndpointBinding in containerNetworkPolicy.Items)
                        {
                            if (isLocalOrFederatedNetworkReference)
                            {
                                // Validate that an endpoint cannot be exposed on more than one local/federated container network.
                                if (servicePackageEndpointContainerNetworkMap.ContainsKey(containerNetworkPolicyEndpointBinding.EndpointRef) && ImageBuilderUtility.NotEquals(servicePackageEndpointContainerNetworkMap[containerNetworkPolicyEndpointBinding.EndpointRef], containerNetworkPolicy.NetworkRef))
                                {
                                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                        TraceType,
                                        applicationManifestFileName,
                                        StringResources.ImageBuilderError_EndpointOnMultipleNetworks,
                                        containerNetworkPolicyEndpointBinding.EndpointRef,
                                        servicePackageEndpointContainerNetworkMap[containerNetworkPolicyEndpointBinding.EndpointRef],
                                        containerNetworkPolicy.NetworkRef);
                                }

                                servicePackageEndpointContainerNetworkMap[containerNetworkPolicyEndpointBinding.EndpointRef] = containerNetworkPolicy.NetworkRef;
                            }
                        }
                    }
                }

                // Validate that a service package cannot be associated with Open network and NAT network at the same time.
                // All other network combinations are valid:
                // container network(s) only, Open network only, NAT network only, container network(s) + Open network, container network(s) + NAT network.
                if (useNatNework && useOpenNetwork)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        applicationManifestFileName,
                        StringResources.ImageBuilderError_IncompatibleNetworkPolicies,
                        servicePackage.Name);
                }

                // Validate that for a service package associated with any container network, all code package entry point hosts are container hosts.
                if (useLocalOrFederatedNetwork && servicePackage.DigestedCodePackage != null)
                {
                    foreach (var digestedCodePackage in servicePackage.DigestedCodePackage)
                    {
                        if (digestedCodePackage.CodePackage != null && digestedCodePackage.CodePackage.EntryPoint != null && !(digestedCodePackage.CodePackage.EntryPoint.Item is ContainerHostEntryPointType))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                applicationManifestFileName,
                                StringResources.ImageBuilderError_InvalidCodePackageEntryPointHost,
                                servicePackage.Name,
                                digestedCodePackage.CodePackage.Name,
                                digestedCodePackage.CodePackage.EntryPoint.Item.GetType());
                        }
                    }
                }
            }
        }

        private void ValidateServicePackageResourceGovernancePolicy(
          ServicePackageResourceGovernancePolicyType servicePackageResourceGovernancePolicy,
          IDictionary<string, string> parameters,
          string fileName)
        {
            // check only if CpuCores specified are > 0 - can be both double or int 
            double? cpuCores = ManifestValidatorUtility.ValidateAsDouble(servicePackageResourceGovernancePolicy.CpuCores, "CpuCores", parameters, fileName);
            // Field can be either zero or greater than 0.0001
            if (cpuCores.HasValue && cpuCores != 0 && cpuCores < 0.0001)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_InvalidCpuCores,
                    cpuCores.Value);
            }

            // Docker will not govern memory if provided value is smaller than 4MB, hence the following check.
            int? memoryLimit = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(servicePackageResourceGovernancePolicy.MemoryInMB, "MemoryInMB", parameters, fileName);
            if (memoryLimit.HasValue && memoryLimit > 0 && memoryLimit < 4)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_InvalidMemoryInMB,
                    memoryLimit.Value);
            }
        }
        private void ValidateCodePackagesResourceGovernance(
          List<ResourceGovernancePolicyType> resourceGovernancePolicies,
          IDictionary<string, string> parameters,
          string fileName)
        {
            int memorySpecifiedCount = 0;
            foreach (ResourceGovernancePolicyType rgPolicy in resourceGovernancePolicies)
            {
                int? memoryLimit = ManifestValidatorUtility.ValidateAsPositiveIntZeroIncluded(rgPolicy.MemoryInMB, "MemoryInMB", parameters, fileName);
                // If user specified memory as simple value check if the value exists and it is > 0
                // If user specified memory as parameter value, just check if the value exists (application should be provisioned but not created)
                if ((memoryLimit.HasValue && memoryLimit > 0) || ImageBuilderUtility.IsParameter(rgPolicy.MemoryInMB))
                {
                    ++memorySpecifiedCount;
                }
            }
            if (memorySpecifiedCount > 0 && memorySpecifiedCount < resourceGovernancePolicies.Count)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                 TraceType,
                 fileName,
                 StringResources.ImageBuilderError_CodePackagesInvalidMemoryValue);
            }
        }

        private void ValidateEndpointBindingPolicies(
            EndpointBindingPolicyType endpointBindingPolicy,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context,
            ResourceOverridesType resourceOverrides)
        {
            if (!this.IsValidEndpointCertRef(endpointBindingPolicy.CertificateRef, context.ApplicationManifest))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidRef,
                    "CertificateRef",
                    endpointBindingPolicy.CertificateRef,
                    "EndpointBindingPolicy",
                    "Certificate",
                    "ApplicationManifest");
            }

            if (!ManifestValidatorUtility.IsValidParameter(
                    endpointBindingPolicy.EndpointRef,
                    "EndpointBindingPolicy",
                    context.ApplicationParameters,
                    context.GetApplicationManifestFileName()))
            {
                if (serviceManifestType.Resources != null && serviceManifestType.Resources.Endpoints != null)
                {

                    EndpointType matchingResourceTypeEndpoint = serviceManifestType.Resources.Endpoints
                        .FirstOrDefault<EndpointType>(
                            endpoint => ImageBuilderUtility.Equals(endpoint.Name, endpointBindingPolicy.EndpointRef));
                    if (matchingResourceTypeEndpoint == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_InvalidRef,
                            "EndpointRef",
                            endpointBindingPolicy.EndpointRef,
                            "EndpointBindingPolicy",
                            "Resource",
                            "ServiceManifest");
                    }

                    EndpointOverrideType endpointOverride = null;
                    if (resourceOverrides != null && resourceOverrides.Endpoints != null && resourceOverrides.Endpoints.Length > 0)
                    {
                        endpointOverride = resourceOverrides.Endpoints.FirstOrDefault(
                            endpoint => ImageBuilderUtility.Equals(endpoint.Name, matchingResourceTypeEndpoint.Name));

                        if (endpointOverride != null && !String.IsNullOrEmpty(endpointOverride.Protocol))
                        {
                            //Ignore the validation since the protocol maybe overridden.
                            return;
                        }
                    }

                    if (matchingResourceTypeEndpoint.Protocol != EndpointTypeProtocol.https)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_InvalidEndpointBinding,
                            endpointBindingPolicy.EndpointRef);
                    }
                }
            }
        }

        private void ValidateSecurityAccessPolicies(
            ApplicationPoliciesTypeSecurityAccessPolicies securityAccessPolicies,
            ApplicationTypeContext context)
        {
            foreach (SecurityAccessPolicyType securityAccessPolicy in securityAccessPolicies.SecurityAccessPolicy)
            {
                bool matchingRefFound = this.IsValidUserRef(
                    securityAccessPolicy.PrincipalRef,
                    context.ApplicationManifest.Principals,
                    context.ApplicationParameters,
                    "SecurityAccessPolicy",
                    context.GetApplicationManifestFileName());
                if (!matchingRefFound)
                {
                    if (!this.IsValidGroupRef(
                        securityAccessPolicy.PrincipalRef,
                        context.ApplicationManifest.Principals,
                        context.ApplicationParameters,
                        "SecurityAccessPolicy",
                        context.GetApplicationManifestFileName()))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_InvalidRef,
                            "PrincipalRef",
                            securityAccessPolicy.PrincipalRef,
                            "SecurityAccessPolicy",
                            "Principal",
                            "ApplicationManifest");
                    }
                }
                bool matchingResourceRef = this.IsValidSecretCertRef(securityAccessPolicy.ResourceRef, context.ApplicationManifest);
                if (!matchingResourceRef)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidRef,
                        "ResourceRef",
                        securityAccessPolicy.ResourceRef,
                        "SecurityAccessPolicy",
                        "SecretsCertificate",
                        "ApplicationManifest");
                }
            }
        }

        private void ValidateContainerHostPolicies(
            List<ContainerHostPoliciesType> containerPolicies,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context,
            out bool hasAnyContainerNetworkConfig)
        {
            hasAnyContainerNetworkConfig = false;

            foreach (ContainerHostPoliciesType policy in containerPolicies)
            {
                CodePackageType matchingCodePackageType = serviceManifestType.CodePackage.FirstOrDefault<CodePackageType>(
                    codePackageType => ImageBuilderUtility.Equals(codePackageType.Name, policy.CodePackageRef));

                if (matchingCodePackageType == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidRef,
                        "CodePackageRef",
                        policy.CodePackageRef,
                        "ContainerHostPolicy",
                        "CodePackage",
                        "ServiceManifest");
                }
                else if (!(matchingCodePackageType.EntryPoint.Item is ContainerHostEntryPointType))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidRepositoryCredentials,
                        policy.CodePackageRef);
                }
                if (policy.Items != null && policy.Items.Length > 0)
                {
                    var duplicateDetector = new DuplicateDetector("PortBinding", "EndpointResourceRef",
                        context.GetApplicationManifestFileName());
                    bool hasNatBinding = false;
                    bool hasNetworkConfig = false;
                    foreach (var item in policy.Items)
                    {
                        if (item.GetType().Equals(typeof(PortBindingType)))
                        {
                            hasNatBinding = true;
                            var portBinding = item as PortBindingType;

                            if (serviceManifestType.Resources == null || serviceManifestType.Resources.Endpoints == null)
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    context.GetApplicationManifestFileName(),
                                    StringResources.ImageBuilder_InvalidPortBinding,
                                    policy.CodePackageRef,
                                    serviceManifestType.Name);
                            }
                            else
                            {
                                EndpointType matchingResourceTypeEndpoint = serviceManifestType.Resources.Endpoints
                                    .FirstOrDefault<EndpointType>(
                                        endpoint =>
                                            ImageBuilderUtility.Equals(endpoint.Name, portBinding.EndpointRef));

                                if (matchingResourceTypeEndpoint == null)
                                {
                                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                        TraceType,
                                        context.GetApplicationManifestFileName(),
                                        StringResources.ImageBuilder_InvalidPortBinding,
                                        policy.CodePackageRef,
                                        serviceManifestType.Name);
                                }
                                else
                                {
                                    duplicateDetector.Add(portBinding.EndpointRef);
                                }
                            }
                        }
                        else if (item.GetType().Equals(typeof(ContainerNetworkConfigType)))
                        {
                            var netConfig = item as ContainerNetworkConfigType;
                            if (!String.IsNullOrEmpty(netConfig.NetworkType) &&
                                String.Equals(netConfig.NetworkType, StringConstants.OpenNetworkConfig, StringComparison.OrdinalIgnoreCase))
                            {
                                hasNetworkConfig = true;
                            }

                            hasAnyContainerNetworkConfig = true;
                        }
                        else if (item.GetType().Equals(typeof(ContainerHealthConfigType)))
                        {
                            var healthConfig = item as ContainerHealthConfigType;

                            if (healthConfig.RestartContainerOnUnhealthyDockerHealthStatus == true &&
                                healthConfig.IncludeDockerHealthStatusInSystemHealthReport == false)
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    context.GetApplicationManifestFileName(),
                                    StringResources.ImageBuilder_InvalidHealthConfig,
                                    policy.CodePackageRef);
                            }
                        }

                        if (hasNatBinding && hasNetworkConfig)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                context.GetApplicationManifestFileName(),
                                StringResources.ImageBuilder_InvalidNetworkConfig,
                                policy.CodePackageRef);
                        }
                    }
                }
            }
        }

        private void ValidateServiceRunAsPolicy(
            List<RunAsPolicyType> runAsPolicies,
            ServiceManifestType serviceManifestType,
            ApplicationTypeContext context)
        {
            foreach (RunAsPolicyType runAsPolicy in runAsPolicies)
            {
                CodePackageType matchingCodePackageType = serviceManifestType.CodePackage.FirstOrDefault<CodePackageType>(
                    codePackageType => ImageBuilderUtility.Equals(codePackageType.Name, runAsPolicy.CodePackageRef));

                if (matchingCodePackageType == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                       TraceType,
                       context.GetApplicationManifestFileName(),
                       StringResources.ImageBuilderError_InvalidRef,
                       "CodePackageRef",
                       runAsPolicy.CodePackageRef,
                       "RunAsPolicy",
                       "CodePackage",
                       "ServiceManifest");

                    return;

                }

                bool matchingRefFound = this.IsValidUserRef(
                    runAsPolicy.UserRef,
                    context.ApplicationManifest.Principals,
                    context.ApplicationParameters,
                    "RunAsPolicy",
                    context.GetApplicationManifestFileName());
                if (!matchingRefFound)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_InvalidRef,
                        "UserRef",
                        runAsPolicy.UserRef,
                        "RunAsPolicy",
                        "Principal",
                        "ApplicationManifest");
                }

                if (runAsPolicy.EntryPointType == RunAsPolicyTypeEntryPointType.Setup)
                {
                    if (matchingCodePackageType.SetupEntryPoint == null ||
                        matchingCodePackageType.SetupEntryPoint.ExeHost == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            context.GetApplicationManifestFileName(),
                            StringResources.ImageBuilderError_SetupRunas,
                            runAsPolicy.CodePackageRef);
                    }
                }
            }
        }

        private bool IsValidSecretCertRef(string resourceName, ApplicationManifestType appManifest)
        {
            FabricCertificateType matchingCert = null;
            if (appManifest.Certificates != null && appManifest.Certificates.SecretsCertificate != null)
            {
                matchingCert = appManifest.Certificates.SecretsCertificate.FirstOrDefault<FabricCertificateType>(
                    cert => ImageBuilderUtility.Equals(resourceName, cert.Name));
            }

            return matchingCert != null;
        }

        private bool IsValidEndpointCertRef(string certRef, ApplicationManifestType appManifest)
        {
            if (appManifest.Certificates == null ||
                appManifest.Certificates.EndpointCertificate == null)
            {
                return false;
            }
            EndpointCertificateType matchingCert = appManifest.Certificates.EndpointCertificate.FirstOrDefault<EndpointCertificateType>(
                cert => ImageBuilderUtility.Equals(certRef, cert.Name));
            return matchingCert != null;
        }
        private bool IsValidUserRef(string referenceName,
            SecurityPrincipalsType principals,
            IDictionary<string, string> parameters,
            string propertyName,
            string fileName)
        {
            bool matchFound = false;
            if (!ManifestValidatorUtility.IsValidParameter(referenceName, propertyName, parameters, fileName))
            {
                if (principals != null)
                {
                    if (principals.Users != null)
                    {
                        SecurityPrincipalsTypeUser matchingUser = principals.Users.FirstOrDefault<SecurityPrincipalsTypeUser>(
                           user => ImageBuilderUtility.Equals(user.Name, referenceName));

                        if (matchingUser != null)
                        {
                            matchFound = true;
                        }
                    }
                }
            }
            else
            {
                //valid application parameter
                matchFound = true;
            }
            return matchFound;
        }

        private bool IsValidGroupRef(string referenceName,
            SecurityPrincipalsType principals,
            IDictionary<string, string> parameters,
            string propertyName,
            string fileName)
        {
            bool matchFound = false;
            if (!ManifestValidatorUtility.IsValidParameter(referenceName, propertyName, parameters, fileName))
            {
                if (principals != null)
                {
                    if (principals.Groups != null)
                    {
                        SecurityPrincipalsTypeGroup matchingGroup = principals.Groups.FirstOrDefault<SecurityPrincipalsTypeGroup>(
                            group => ImageBuilderUtility.Equals(group.Name, referenceName));

                        if (matchingGroup != null)
                        {
                            matchFound = true;
                        }
                    }

                }
            }
            else
            {
                //valid application parameter
                matchFound = true;
            }
            return matchFound;
        }

        private void ValidateHealthPolicy(ApplicationHealthPolicyType healthPolicy, ApplicationTypeContext context)
        {
            int percent;
            if (!int.TryParse(healthPolicy.MaxPercentUnhealthyDeployedApplications, out percent) || percent < 0 || percent > 100)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidValue,
                    "MaxPercentUnhealthyDeployedApplications",
                    healthPolicy.MaxPercentUnhealthyDeployedApplications);
            }

            if (healthPolicy.DefaultServiceTypeHealthPolicy != null)
            {
                this.ValidateServiceTypeHealthPolicy(healthPolicy.DefaultServiceTypeHealthPolicy, context);
            }

            if (healthPolicy.ServiceTypeHealthPolicy != null)
            {
                var duplicateDetector = new DuplicateDetector("ServiceTypeHealthPolicy", "ServiceTypeName", context.GetApplicationManifestFileName());

                foreach (var serviceTypeHealth in healthPolicy.ServiceTypeHealthPolicy)
                {
                    duplicateDetector.Add(serviceTypeHealth.ServiceTypeName);
                    this.ValidateServiceTypeHealthPolicy(serviceTypeHealth, context);
                }
            }
        }

        private void ValidateServiceTypeHealthPolicy(ServiceTypeHealthPolicyType serviceTypeHealth, ApplicationTypeContext context)
        {
            int percent;
            if (!int.TryParse(serviceTypeHealth.MaxPercentUnhealthyServices, out percent) || percent < 0 || percent > 100)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidValue,
                    "MaxPercentUnhealthyServices",
                    serviceTypeHealth.MaxPercentUnhealthyServices);
            }

            if (!int.TryParse(serviceTypeHealth.MaxPercentUnhealthyPartitionsPerService, out percent) || percent < 0 || percent > 100)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidValue,
                    "MaxPercentUnhealthyPartitionsPerService",
                    serviceTypeHealth.MaxPercentUnhealthyPartitionsPerService);
            }

            if (!int.TryParse(serviceTypeHealth.MaxPercentUnhealthyReplicasPerPartition, out percent) || percent < 0 || percent > 100)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    context.GetApplicationManifestFileName(),
                    StringResources.ImageBuilderError_InvalidValue,
                    "MaxPercentUnhealthyReplicasPerPartition",
                    serviceTypeHealth.MaxPercentUnhealthyReplicasPerPartition);
            }
        }

        private void ValidateDefaultServices(ApplicationTypeContext context)
        {
            if (context.ApplicationManifest.DefaultServices != null && context.ApplicationManifest.DefaultServices.Items != null)
            {
                DuplicateDetector defaultServicesDuplicateDetector = new DuplicateDetector("DefaultServices", "Name", context.GetApplicationManifestFileName());
                DuplicateDetector defaultServicesDnsNameDuplicateDetector = new DuplicateDetector("DefaultServices", "DnsName", context.GetApplicationManifestFileName());

                foreach (object defaultServiceItem in context.ApplicationManifest.DefaultServices.Items)
                {
                    if (defaultServiceItem is DefaultServicesTypeService)
                    {
                        DefaultServicesTypeService defaultService = (DefaultServicesTypeService)defaultServiceItem;
                        defaultServicesDuplicateDetector.Add(defaultService.Name);

                        this.ValidateServiceDnsName(defaultService, context.ApplicationParameters, defaultServicesDnsNameDuplicateDetector, context.GetApplicationManifestFileName());
                        this.ValidateServiceType(defaultService.Item, context.GetServiceManifestTypes(), context.ApplicationParameters, context.GetApplicationManifestFileName());
                    }

                    if (defaultServiceItem is DefaultServicesTypeServiceGroup)
                    {
                        DefaultServicesTypeServiceGroup defaultServiceGroup = (DefaultServicesTypeServiceGroup)defaultServiceItem;
                        defaultServicesDuplicateDetector.Add(defaultServiceGroup.Name);
                        this.ValidateServiceGroupType(defaultServiceGroup.Item, context.GetServiceManifestTypes(), context.ApplicationParameters, context.GetApplicationManifestFileName());
                    }
                }
            }
        }

        private void ValidateServiceDnsName(DefaultServicesTypeService defaultService, IDictionary<string, string> parameters, DuplicateDetector duplicateDetector, string fileName)
        {
            string serviceDnsName = defaultService.ServiceDnsName;

            if (!string.IsNullOrEmpty(serviceDnsName))
            {
                // Check if the value is parameterized? If yes, get the parameter value.
                if (ManifestValidatorUtility.IsValidParameter(serviceDnsName, "ServiceDnsName", parameters, fileName))
                {
                    string parameterName = serviceDnsName.Substring(1, serviceDnsName.Length - 2);
                    serviceDnsName = parameters[parameterName];
                }

                // Provided parameterized value itself can be empty.
                if (string.IsNullOrEmpty(serviceDnsName))
                {
                    return;
                }

                // At this point, we have the actual value.
                // check for RFC compliance.
                if (Uri.CheckHostName(serviceDnsName) != UriHostNameType.Dns)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_InvalidServiceDnsName,
                        defaultService.Name);
                }

                // Check for duplication.
                duplicateDetector.Add(serviceDnsName);
            }
        }

        private void ValidateServiceTemplates(ApplicationTypeContext context)
        {
            if (context.ApplicationManifest.ServiceTemplates != null)
            {
                DuplicateDetector serviceTypeNameDuplicateDetector = new DuplicateDetector("ServiceTemplates", "ServiceTypeName", context.GetApplicationManifestFileName());
                foreach (ServiceType serviceTemplate in context.ApplicationManifest.ServiceTemplates)
                {
                    serviceTypeNameDuplicateDetector.Add(serviceTemplate.ServiceTypeName);
                    this.ValidateServiceType(serviceTemplate, context.GetServiceManifestTypes(), context.ApplicationParameters, context.GetApplicationManifestFileName());
                }
            }
        }

        private void ValidateServiceType(
            ServiceType serviceType,
            IEnumerable<ServiceManifestType> serviceManifestTypes,
            IDictionary<string, string> parameters,
            string fileName)
        {
            this.ValidateServiceType(serviceType, parameters, fileName);
            Func<object, bool> filterServiceTypes = serviceOrServiceGroupType =>
            {
                if (serviceOrServiceGroupType is ServiceTypeType || serviceOrServiceGroupType is ServiceGroupTypeType)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            };
            Func<object, bool> findServiceTypes = serviceOrServiceGroupType =>
            {
                bool isValid = false;
                if (serviceOrServiceGroupType is ServiceTypeType)
                {
                    ServiceTypeType serviceTypeType = (ServiceTypeType)serviceOrServiceGroupType;
                    if (ImageBuilderUtility.Equals(serviceTypeType.ServiceTypeName, serviceType.ServiceTypeName))
                    {
                        if (serviceType.GetType().Equals(typeof(StatefulServiceType)))
                        {
                            isValid = serviceTypeType.GetType().Equals(typeof(StatefulServiceTypeType));
                        }
                        else if (serviceType.GetType().Equals(typeof(StatelessServiceType)))
                        {
                            isValid = serviceTypeType.GetType().Equals(typeof(StatelessServiceTypeType));
                        }
                    }
                }
                else if (serviceOrServiceGroupType is ServiceGroupTypeType)
                {
                    ServiceGroupTypeType serviceGroupTypeType = (ServiceGroupTypeType)serviceOrServiceGroupType;
                    if (ImageBuilderUtility.Equals(serviceGroupTypeType.ServiceGroupTypeName, serviceType.ServiceTypeName))
                    {
                        if (serviceType.GetType().Equals(typeof(StatefulServiceGroupType)))
                        {
                            isValid = serviceGroupTypeType.GetType().Equals(typeof(StatefulServiceGroupTypeType));
                        }
                        else if (serviceType.GetType().Equals(typeof(StatelessServiceGroupType)))
                        {
                            isValid = serviceGroupTypeType.GetType().Equals(typeof(StatelessServiceGroupTypeType));
                        }
                    }
                }

                return isValid;
            };

            ServiceManifestType matchingServiceManifestType = serviceManifestTypes.FirstOrDefault(
                serviceManifestType => serviceManifestType.ServiceTypes
                    .Where(filterServiceTypes)
                    .Any(findServiceTypes));

            if (matchingServiceManifestType == null)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_ServiceTypeNameNotSupportedInImports,
                    serviceType.ServiceTypeName);
            }
        }

        private void ValidateServiceType(ServiceType serviceType, IDictionary<string, string> parameters, string fileName)
        {
            if (serviceType == null)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_NullOrEmptyError,
                    "ServiceType");
            }
            else
            {
                if (serviceType.ServiceCorrelations != null && serviceType.ServiceCorrelations.Length > 0)
                {
                    //validate there is only one correlation
                    if (serviceType.ServiceCorrelations.Length > 1)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageBuilderError_MoreThanOneAffinity);
                    }

                    if (serviceType.ServiceCorrelations[0] == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_NullPropertyError,
                        "ServiceCorrelations[0]",
                        "ServiceType");
                    }

                    //validate that service name is not empty
                    if (string.IsNullOrEmpty(serviceType.ServiceCorrelations[0].ServiceName))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageBuilderError_InvalidValue,
                            "AffinityServiceName",
                            serviceType.ServiceCorrelations[0].ServiceName);
                    }
                }

                if (serviceType is StatelessServiceType)
                {
                    StatelessServiceType statelessServiceType = (StatelessServiceType)serviceType;
#if DotNetCoreClrLinux
                if (statelessServiceType == null)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageStoreError_OperationFailed,
                        "StatelessServiceType",
                        "failure of casting");
                }
#endif

                    long? instanceCount = ManifestValidatorUtility.ValidateAsLong(statelessServiceType.InstanceCount, "InstanceCount", parameters, fileName);
                    if (instanceCount != null && instanceCount < 1 && instanceCount != -1)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageBuilderError_InvalidInstanceCountValue,
                            instanceCount);
                    }

                    if (instanceCount == -1 && (serviceType.ServiceCorrelations != null && serviceType.ServiceCorrelations.Length > 0))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageBuilderError_EveryNodeServiceShouldNotHasAffinity);
                    }
                }
                else
                {
                    StatefulServiceType statefulServiceType = (StatefulServiceType)serviceType;
#if DotNetCoreClrLinux
                    if (statefulServiceType == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageStoreError_OperationFailed,
                            "StatefulServiceType",
                            "failure of casting");     
                     }         
#endif
                    int? minReplicatSetSizeValue = ManifestValidatorUtility.ValidateAsPositiveInteger(statefulServiceType.MinReplicaSetSize, "MinReplicaSetSize", parameters, fileName);
                    int? targetReplicaSetSizeValue = ManifestValidatorUtility.ValidateAsPositiveInteger(statefulServiceType.TargetReplicaSetSize, "TargetReplicaSetSize", parameters, fileName);

                    if (minReplicatSetSizeValue != null && targetReplicaSetSizeValue != null && minReplicatSetSizeValue > targetReplicaSetSizeValue)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageBuilderError_MinReplicaGreaterThanTargetReplica,
                            minReplicatSetSizeValue,
                            targetReplicaSetSizeValue);
                    }

                    // Empty string is allowed for ReplicaRestartWaitDuration, QuorumLossWaitDuration, and StandByReplicaKeepDuration
                    // These will get the default values from the constructor parameters

                    if (!String.IsNullOrEmpty(statefulServiceType.ReplicaRestartWaitDurationSeconds))
                    {
                        TimeSpan? replicaRestartWaitDuration = ManifestValidatorUtility.ValidateAsTimeSpan(statefulServiceType.ReplicaRestartWaitDurationSeconds, "ReplicaRestartWaitDurationSeconds", parameters, fileName);

                        if (replicaRestartWaitDuration != null)
                        {
                            TimeSpan rrwdvalue = replicaRestartWaitDuration.Value;

                            if ((TimeSpan.Compare(rrwdvalue, TimeSpan.MinValue) != 0) && (rrwdvalue.Ticks < 0))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    fileName,
                                    StringResources.ImageBuilderError_NegativeTimeDuration,
                                    rrwdvalue.Seconds);
                            }
                        }
                    }

                    if (!String.IsNullOrEmpty(statefulServiceType.QuorumLossWaitDurationSeconds))
                    {
                        TimeSpan? quorumLossWaitDuration = ManifestValidatorUtility.ValidateAsTimeSpan(statefulServiceType.QuorumLossWaitDurationSeconds, "QuorumLossWaitDurationSeconds", parameters, fileName);

                        if (quorumLossWaitDuration != null)
                        {
                            TimeSpan qlwdvalue = quorumLossWaitDuration.Value;

                            if ((TimeSpan.Compare(qlwdvalue, TimeSpan.MinValue) != 0) && (qlwdvalue.Ticks < 0))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    fileName,
                                    StringResources.ImageBuilderError_NegativeTimeDuration,
                                    qlwdvalue.Seconds);
                            }
                        }
                    }

                    if (!String.IsNullOrEmpty(statefulServiceType.StandByReplicaKeepDurationSeconds))
                    {
                        TimeSpan? standByReplicaKeepDuration = ManifestValidatorUtility.ValidateAsTimeSpan(statefulServiceType.StandByReplicaKeepDurationSeconds, "StandByReplicaKeepDurationSeconds", parameters, fileName);

                        if (standByReplicaKeepDuration != null)
                        {
                            TimeSpan srkdvalue = standByReplicaKeepDuration.Value;

                            if ((TimeSpan.Compare(srkdvalue, TimeSpan.MinValue) != 0) && (srkdvalue.Ticks < 0))
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    fileName,
                                    StringResources.ImageBuilderError_NegativeTimeDuration,
                                    srkdvalue.Seconds);
                            }
                        }
                    }
                }

                if (serviceType.UniformInt64Partition != null)
                {
                    int? partitionCount = ManifestValidatorUtility.ValidateAsPositiveInteger(serviceType.UniformInt64Partition.PartitionCount, "PartitionCount", parameters, fileName);
                    long? lowKey = ManifestValidatorUtility.ValidateAsLong(serviceType.UniformInt64Partition.LowKey, "LowKey", parameters, fileName);
                    long? highKey = ManifestValidatorUtility.ValidateAsLong(serviceType.UniformInt64Partition.HighKey, "HighKey", parameters, fileName);

                    if (highKey != null && lowKey != null)
                    {
                        if (lowKey > highKey)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                fileName,
                                StringResources.ImageBuilderError_LowKeyGreaterThanHighKey,
                                lowKey,
                                highKey);
                        }

                        if (partitionCount != null)
                        {
                            ulong availableKeys = (ulong)(highKey - lowKey);

                            // Since both highkey and lowkey are inclusive, we need to add 1. 
                            // Ensuring we are not overflowing ulong
                            availableKeys = (availableKeys == ulong.MaxValue) ? availableKeys : (availableKeys + 1);

                            if (availableKeys < (ulong)partitionCount)
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    fileName,
                                    StringResources.ImageBuilderError_InvalidUniformInt64Partition,
                                    lowKey,
                                    highKey,
                                    partitionCount);
                            }
                        }
                    }
                }
                else if (serviceType.NamedPartition != null)
                {
                    if (serviceType.NamedPartition.Partition == null)
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            fileName,
                            StringResources.ImageBuilderError_MissingNamedPartitionChildEntries);
                    }

                    DuplicateDetector duplicateNames = new DuplicateDetector("NamedPartition", "Name", fileName, false /*ignoreCase*/);
                    foreach (var p in serviceType.NamedPartition.Partition)
                    {
                        if (string.IsNullOrEmpty(p.Name))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                fileName,
                                StringResources.ImageBuilderError_NullOrEmptyError,
                                "NamedPartition");
                        }
                        else
                        {
                            duplicateNames.Add(p.Name);
                        }
                    }
                }

                if (!ManifestValidatorUtility.IsValidParameter(serviceType.PlacementConstraints, "PlacementConstraints", parameters, fileName))
                {
                    this.ValidatePlacementConstraints(fileName, serviceType.PlacementConstraints);
                }
            }
        }

        private void ValidatePlacementConstraints(string fileName, string placementConstraints)
        {
            if (!string.IsNullOrEmpty(placementConstraints))
            {
                if (!InteropHelpers.ExpressionValidatorHelper.IsValidExpression(placementConstraints))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_InvalidValue,
                        "PlacementConstraint",
                        placementConstraints);
                }
            }
        }

        private void ValidateServiceGroupType(ServiceType serviceGroupType, IDictionary<string, string> parameters, string fileName)
        {
            this.ValidateServiceType(serviceGroupType, parameters, fileName);

            ServiceGroupMemberType[] members = null;

            StatefulServiceGroupType statefulServiceGroupType = serviceGroupType as StatefulServiceGroupType;
            if (statefulServiceGroupType != null)
            {
                members = statefulServiceGroupType.Members;
            }
            else
            {
                StatelessServiceGroupType statelessServiceGroupType = serviceGroupType as StatelessServiceGroupType;
                if (statelessServiceGroupType != null)
                {
                    members = statelessServiceGroupType.Members;
                }
            }

            if (members == null || members.Length < 1)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_ServiceGroupHasNoMemberServices);
                return;
            }

            DuplicateDetector duplicateNames = new DuplicateDetector("Member", "Name", fileName);

            foreach (ServiceGroupMemberType member in members)
            {
                if (string.IsNullOrWhiteSpace(member.ServiceTypeName))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_ServiceGroupServiceTypeNameEmpty);
                }

                if (string.IsNullOrWhiteSpace(member.Name))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_ServiceGroupMemberNameEmpty);
                }

                duplicateNames.Add(member.Name);

                if (member.LoadMetrics != null)
                {
                    foreach (LoadMetricType metric in member.LoadMetrics)
                    {
                        if (string.IsNullOrWhiteSpace(metric.Name))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                fileName,
                                StringResources.ImageBuilderError_LoadBalancingMetricNameEmpty);
                        }
                    }
                }
            }
        }

        private void ValidateServiceGroupType(
            ServiceType serviceType,
            IEnumerable<ServiceManifestType> serviceManifestTypes,
            IDictionary<string, string> parameters,
            string fileName)
        {
            this.ValidateServiceGroupType(serviceType, parameters, fileName);

            Func<object, bool> filterServiceGroupTypes = serviceOrServiceGroupType => serviceOrServiceGroupType is ServiceGroupTypeType;
            Func<ServiceGroupTypeType, bool> findServiceGroupTypes = serviceGroupTypeType =>
            {
                bool isValid = false;
                if (ImageBuilderUtility.Equals(serviceGroupTypeType.ServiceGroupTypeName, serviceType.ServiceTypeName))
                {
                    if (serviceType.GetType().Equals(typeof(StatefulServiceGroupType)))
                    {
                        isValid = serviceGroupTypeType.GetType().Equals(typeof(StatefulServiceGroupTypeType));
                    }
                    else if (serviceType.GetType().Equals(typeof(StatelessServiceGroupType)))
                    {
                        isValid = serviceGroupTypeType.GetType().Equals(typeof(StatelessServiceGroupTypeType));
                    }
                }

                return isValid;
            };

            bool hasMatchingServiceManifestType = serviceManifestTypes.Any(
                serviceManifestType => serviceManifestType.ServiceTypes
                    .Where(filterServiceGroupTypes)
                    .Cast<ServiceGroupTypeType>()
                    .Any(findServiceGroupTypes));

            if (!hasMatchingServiceManifestType)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_ServiceTypeNameNotSupportedInImports,
                    serviceType.ServiceTypeName);
            }
        }
    }
}