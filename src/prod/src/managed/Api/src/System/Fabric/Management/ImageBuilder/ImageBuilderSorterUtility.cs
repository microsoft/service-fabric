// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    internal static class ImageBuilderSorterUtility
    {
        public static void SortApplicationManifestType(ApplicationManifestType applicationManifest)
        {
            applicationManifest.Parameters = SortApplicationParameters(applicationManifest.Parameters);
            applicationManifest.ServiceManifestImport = SortServiceManifestImports(applicationManifest.ServiceManifestImport);
            applicationManifest.ServiceTemplates = SortServiceTemplates(applicationManifest.ServiceTemplates);
            applicationManifest.DefaultServices = SortDefaultServices(applicationManifest.DefaultServices);

            if (applicationManifest.Principals != null)
            {
                applicationManifest.Principals.Groups = SortGroups(applicationManifest.Principals.Groups);
                applicationManifest.Principals.Users = SortUsers(applicationManifest.Principals.Users);
            }

            if (applicationManifest.Policies != null) 
            {
                if(applicationManifest.Policies.LogCollectionPolicies != null)
                {
                    applicationManifest.Policies.LogCollectionPolicies.LogCollectionPolicy = SortLogCollectionPolicies(applicationManifest.Policies.LogCollectionPolicies.LogCollectionPolicy);
                }
                if (applicationManifest.Policies.SecurityAccessPolicies != null)
                {
                    applicationManifest.Policies.SecurityAccessPolicies.SecurityAccessPolicy = applicationManifest.Policies.SecurityAccessPolicies.SecurityAccessPolicy.OrderBy(
                        securityAccessPolicy => securityAccessPolicy.ResourceRef).ToArray();
                }
            }
            if (applicationManifest.Certificates != null)
            {
                if (applicationManifest.Certificates.SecretsCertificate != null)
                {
                    applicationManifest.Certificates.SecretsCertificate = applicationManifest.Certificates.SecretsCertificate.OrderBy(
                            secretCertificate => secretCertificate.Name).ToArray();
                }
                if (applicationManifest.Certificates.EndpointCertificate != null)
                {
                    applicationManifest.Certificates.EndpointCertificate = applicationManifest.Certificates.EndpointCertificate.OrderBy(
                        endpointCertificate => endpointCertificate.Name).ToArray();
                }
            }
        }

        public static void SortServiceManifestType(ServiceManifestType serviceManifest)
        {
            serviceManifest.ServiceTypes = SortServiceTypes(serviceManifest.ServiceTypes);
            serviceManifest.CodePackage = SortCodePackages(serviceManifest.CodePackage);
            serviceManifest.ConfigPackage = SortConfigPackages(serviceManifest.ConfigPackage);
            serviceManifest.DataPackage = SortDataPackages(serviceManifest.DataPackage);

            if (serviceManifest.Resources != null)
            {
                serviceManifest.Resources.Endpoints = SortResourceEndpoints(serviceManifest.Resources.Endpoints);
            }

            SortServiceDiagnostics(serviceManifest.Diagnostics);
        }

        public static void SortApplicationPackageType(ApplicationPackageType applicationPackage)
        {
            // Since User names and Group names can be parameterized, the actual values are known only after creating the ApplicationInstance.
            // Hence the Users and Groups on ApplicationInstance needs to be sorted. Non-parameterized name are sorted using the Manifest during 
            // provisioning.
            applicationPackage.DigestedEnvironment.Principals.Groups = SortGroups(applicationPackage.DigestedEnvironment.Principals.Groups);
            applicationPackage.DigestedEnvironment.Principals.Users = SortUsers(applicationPackage.DigestedEnvironment.Principals.Users);

            if (applicationPackage.DigestedEnvironment.Diagnostics != null)
            {
                SortDiagnostics(applicationPackage.DigestedEnvironment.Diagnostics);
            }
        }

        private static ApplicationPoliciesTypeLogCollectionPoliciesLogCollectionPolicy[] SortLogCollectionPolicies(ApplicationPoliciesTypeLogCollectionPoliciesLogCollectionPolicy[] logCollectionPolicies)
        {            
            return (logCollectionPolicies == null) ? null : logCollectionPolicies.OrderBy(logCollectionPolicy => logCollectionPolicy.Path).ToArray();
        }

        private static SecurityPrincipalsTypeUser[] SortUsers(SecurityPrincipalsTypeUser[] users)
        {
            if (users == null)
            {
                return null;
            }

            SecurityPrincipalsTypeUser[] sortedUsers = users.OrderBy(user => user.Name).ToArray();            
            foreach (SecurityPrincipalsTypeUser user in sortedUsers)
            {
                user.MemberOf = SortMemberOf(user.MemberOf);                
            }

            return sortedUsers;
        }

        private static object[] SortMemberOf(object[] memberOfArray)
        {
            if (memberOfArray == null)
            {
                return null;
            }

            IEnumerable<SecurityPrincipalsTypeUserGroup> userGroups = memberOfArray.OfType<SecurityPrincipalsTypeUserGroup>();
            IOrderedEnumerable<SecurityPrincipalsTypeUserGroup> orderedUserGroups = userGroups.OrderBy(userGroup => userGroup.NameRef);

            IEnumerable<SecurityPrincipalsTypeUserSystemGroup> systemGroups = memberOfArray.OfType<SecurityPrincipalsTypeUserSystemGroup>();
            IOrderedEnumerable<SecurityPrincipalsTypeUserSystemGroup> orderedSystemGroups = systemGroups.OrderBy(systemGroup => systemGroup.Name);

            return orderedUserGroups.Union<object>(orderedSystemGroups).ToArray();                    
        }

        private static SecurityPrincipalsTypeGroup[] SortGroups(SecurityPrincipalsTypeGroup[] groups)
        {
            if (groups == null)
            {
                return null;
            }

            SecurityPrincipalsTypeGroup[] sortedGroups = groups.OrderBy(group => group.Name).ToArray();            
            foreach (SecurityPrincipalsTypeGroup group in sortedGroups)
            {
                group.Membership = SortMemberships(group.Membership);                
            }

            return sortedGroups;
        }

        private static object[] SortMemberships(object[] memberships)
        {
            if (memberships == null)
            {
                return null;
            }

            IEnumerable<SecurityPrincipalsTypeGroupDomainUser> domainUserGroups = memberships.OfType<SecurityPrincipalsTypeGroupDomainUser>();
            IOrderedEnumerable<SecurityPrincipalsTypeGroupDomainUser> orderedDomainUserGroups = domainUserGroups.OrderBy(domainUserGroup => domainUserGroup.Name);

            IEnumerable<SecurityPrincipalsTypeGroupDomainGroup> domainGroups = memberships.OfType<SecurityPrincipalsTypeGroupDomainGroup>();
            IOrderedEnumerable<SecurityPrincipalsTypeGroupDomainGroup> orderedDomainGroups = domainGroups.OrderBy(domainGroup => domainGroup.Name);

            IEnumerable<SecurityPrincipalsTypeGroupSystemGroup> systemGroups = memberships.OfType<SecurityPrincipalsTypeGroupSystemGroup>();
            IOrderedEnumerable<SecurityPrincipalsTypeGroupSystemGroup> orderedSystemGroups = systemGroups.OrderBy(systemGroup => systemGroup.Name);

            return orderedDomainUserGroups.Union<object>(orderedDomainGroups).Union(orderedSystemGroups).ToArray();           
        }

        private static DefaultServicesType SortDefaultServices(DefaultServicesType defaultServices)
        {
            if (defaultServices == null)
            {
                return null;
            }

            Func<object, string> getName = (item) =>
            {
                if (item is DefaultServicesTypeService)
                {
                    return ((DefaultServicesTypeService)item).Name;
                }
                else
                {
                    return ((DefaultServicesTypeServiceGroup)item).Name;
                }
            };

            return new DefaultServicesType
            {
                Items = (defaultServices.Items == null) ? null : defaultServices.Items.OrderBy(getName).ToArray()
            };
        }

        private static ServiceType[] SortServiceTemplates(ServiceType[] serviceTemplates)
        {
            return (serviceTemplates == null) ? null : serviceTemplates.OrderBy(serviceTemplate => serviceTemplate.ServiceTypeName).ToArray();  
        }

        private static ApplicationManifestTypeServiceManifestImport[] SortServiceManifestImports(ApplicationManifestTypeServiceManifestImport[] serviceManifestImports)
        {
            ApplicationManifestTypeServiceManifestImport[] sortedServiceManifestImports = serviceManifestImports.OrderBy(serviceManifestImport => serviceManifestImport.ServiceManifestRef.ServiceManifestName).ToArray();

            foreach (ApplicationManifestTypeServiceManifestImport serviceManifestImport in serviceManifestImports)
            {
                serviceManifestImport.ConfigOverrides = SortConfigOverrides(serviceManifestImport.ConfigOverrides);
                serviceManifestImport.Policies = SortServiceImportPolicies(serviceManifestImport.Policies);
            }

            return sortedServiceManifestImports;
        }

        private static object[] SortServiceImportPolicies(object[] policies)
        {
            if (policies == null)
            {
                return null;
            }

            IEnumerable<RunAsPolicyType> runAsPolicies = policies.OfType<RunAsPolicyType>();
            IOrderedEnumerable<RunAsPolicyType> orderedRunAsPolicy = runAsPolicies.OrderBy(runAsPolicy => (runAsPolicy.CodePackageRef + runAsPolicy.EntryPointType));

            IEnumerable<SecurityAccessPolicyType> securityAccessPolicies = policies.OfType<SecurityAccessPolicyType>();
            IOrderedEnumerable<SecurityAccessPolicyType> orderedSecurityAccessPolicies = securityAccessPolicies.OrderBy(securityAccessPolicy => securityAccessPolicy.ResourceRef);

            IEnumerable<PackageSharingPolicyType> packageSharingPolicies = policies.OfType<PackageSharingPolicyType>();
            IOrderedEnumerable<PackageSharingPolicyType> orderedPackageSharingPolicies = packageSharingPolicies.OrderBy(packageSharingPolicy => packageSharingPolicy.PackageRef);

            IEnumerable<EndpointBindingPolicyType> endpointBindingPolcies = policies.OfType<EndpointBindingPolicyType>();
            IOrderedEnumerable<EndpointBindingPolicyType> orderedEndpointBindingPolicies =
                endpointBindingPolcies.OrderBy(endpointBindingPolicy => endpointBindingPolicy.EndpointRef);

            IEnumerable<ContainerHostPoliciesType> containerPolicies = policies.OfType<ContainerHostPoliciesType>();
            IOrderedEnumerable<ContainerHostPoliciesType> orderedContainerPolicies =
                containerPolicies.OrderBy(containerPolicy => containerPolicy.CodePackageRef);

            IEnumerable<ResourceGovernancePolicyType> resourceGovernancePolicies = policies.OfType<ResourceGovernancePolicyType>();
            IOrderedEnumerable<ResourceGovernancePolicyType> orderedResourceGovernancePolicies =
                resourceGovernancePolicies.OrderBy(resourceGovernancePolicy => resourceGovernancePolicy.CodePackageRef);

            // Only one of these is allowed, so no point in sorting.
            IEnumerable<ServicePackageResourceGovernancePolicyType> spResourceGovernancePolicies = policies.OfType<ServicePackageResourceGovernancePolicyType>();
            IEnumerable<ServiceFabricRuntimeAccessPolicyType> spSfRuntimeAccessPolicies = policies.OfType<ServiceFabricRuntimeAccessPolicyType>();

            IEnumerable<ConfigPackagePoliciesType> configPackagePolicies = policies.OfType<ConfigPackagePoliciesType>();
            IOrderedEnumerable<ConfigPackagePoliciesType> orderedconfigPackagePolicies =
               configPackagePolicies.OrderBy(configPackagePolicy => configPackagePolicy.CodePackageRef);

            IEnumerable<ServicePackageContainerPolicyType> spContainerPolicy = policies.OfType<ServicePackageContainerPolicyType>();

            IEnumerable<NetworkPoliciesType> networkPolicies = policies.OfType<NetworkPoliciesType>();

            //
            // The ordering of the policies here must match the order of reading in 
            // ServiceModel\ServicepackageDescription -> ReadXml()
            //

            return orderedRunAsPolicy.Union<object>(orderedSecurityAccessPolicies).
                Union<object>(orderedPackageSharingPolicies).
                Union<object>(orderedEndpointBindingPolicies).
                Union<object>(orderedContainerPolicies).
                Union<object>(orderedResourceGovernancePolicies).
                Union<object>(spResourceGovernancePolicies).
                Union<object>(orderedconfigPackagePolicies).
                Union<object>(spContainerPolicy).
                Union<object>(spSfRuntimeAccessPolicies).
                Union<object>(networkPolicies).ToArray();
        }

        private static ConfigOverrideType[] SortConfigOverrides(ConfigOverrideType[] configOverrides)
        {
            if (configOverrides == null)
            {
                return null;
            }

            ConfigOverrideType[] sortedConfigOverrides = configOverrides.OrderBy(configOverride => configOverride.Name).ToArray();

            foreach (ConfigOverrideType configOverride in sortedConfigOverrides)
            {
                configOverride.Settings = SortOverrideSettings(configOverride.Settings);
            }

            return sortedConfigOverrides;
        }

        private static SettingsOverridesTypeSection[] SortOverrideSettings(SettingsOverridesTypeSection[] overrideSettings)
        {
            if (overrideSettings == null)
            {
                return null;
            }

            SettingsOverridesTypeSection[] sortedOverrideSettings = overrideSettings.OrderBy(overrideSetting => overrideSetting.Name).ToArray();

            foreach (SettingsOverridesTypeSection overrideSetting in sortedOverrideSettings)
            {
                overrideSetting.Parameter = SortOverrideParameters(overrideSetting.Parameter);
            }

            return sortedOverrideSettings;
        }

        private static SettingsOverridesTypeSectionParameter[] SortOverrideParameters(SettingsOverridesTypeSectionParameter[] overrideParameters)
        {
            return (overrideParameters == null) ? null : overrideParameters.OrderBy(overrideParameter => overrideParameter.Name).ToArray();  
        }

        private static ApplicationManifestTypeParameter[] SortApplicationParameters(ApplicationManifestTypeParameter[] applicationParameters)
        {
            return (applicationParameters == null) ? null : applicationParameters.OrderBy(parameter => parameter.Name).ToArray(); 
        }

        private static EndpointType[] SortResourceEndpoints(EndpointType[] endpoints)
        {
            return (endpoints == null) ? null : endpoints.OrderBy(endpoint => endpoint.Name).ToArray();                 
        }

        private static DataPackageType[] SortDataPackages(DataPackageType[] dataPackages)
        {
            return (dataPackages == null) ? null : dataPackages.OrderBy(dataPackage => dataPackage.Name).ToArray();     
        }

        private static ConfigPackageType[] SortConfigPackages(ConfigPackageType[] configPackages)
        {
            return (configPackages == null) ? null : configPackages.OrderBy(configPackage => configPackage.Name).ToArray();            
        }

        private static CodePackageType[] SortCodePackages(CodePackageType[] codePackages)
        {
            return codePackages.OrderBy(codePackage => codePackage.Name).ToArray();            
        }

        private static object[] SortServiceTypes(object[] serviceTypes)
        {            
            return serviceTypes.OrderBy(manifestServiceType =>
            {
                ServiceTypeType serviceType = manifestServiceType as ServiceTypeType;
                if (serviceType != null)
                {
                    return serviceType.ServiceTypeName;
                }
                else
                {
                    return ((ServiceGroupTypeType)manifestServiceType).ServiceGroupTypeName;
                }
            }).ToArray();            
        }

        private static void SortServiceDiagnostics(ServiceDiagnosticsType diagnostics)
        {
            if (null != diagnostics)
            {
                if (null != diagnostics.ETW)
                {
                    if (null != diagnostics.ETW.ProviderGuids)
                    {
                        diagnostics.ETW.ProviderGuids = SortProviderGuids(diagnostics.ETW.ProviderGuids);
                    }

                    diagnostics.ETW.ManifestDataPackages = SortDataPackages(diagnostics.ETW.ManifestDataPackages);
                }
            }
        }

        private static ServiceDiagnosticsTypeETWProviderGuid[] SortProviderGuids(ServiceDiagnosticsTypeETWProviderGuid[] providerGuids)
        {
            return providerGuids.OrderBy(providerGuid => providerGuid.Value).ToArray();
        }

        private static void SortDiagnostics(DiagnosticsType diagnostics)
        {
            if (diagnostics.CrashDumpSource != null)
            {
                if (diagnostics.CrashDumpSource.Destinations != null)
                {
                    diagnostics.CrashDumpSource.Destinations.AzureBlob = SortAzureBlob(diagnostics.CrashDumpSource.Destinations.AzureBlob);
                    diagnostics.CrashDumpSource.Destinations.FileStore = SortFileStore(diagnostics.CrashDumpSource.Destinations.FileStore);
                    diagnostics.CrashDumpSource.Destinations.LocalStore = SortLocalStore(diagnostics.CrashDumpSource.Destinations.LocalStore);
                }

                diagnostics.CrashDumpSource.Parameters = SortParameters(diagnostics.CrashDumpSource.Parameters);
            }
            if (diagnostics.ETWSource != null)
            {
                if (diagnostics.ETWSource.Destinations != null)
                {
                    diagnostics.ETWSource.Destinations.AzureBlob = SortAzureBlob(diagnostics.ETWSource.Destinations.AzureBlob);
                    diagnostics.ETWSource.Destinations.FileStore = SortFileStore(diagnostics.ETWSource.Destinations.FileStore);
                    diagnostics.ETWSource.Destinations.LocalStore = SortLocalStore(diagnostics.ETWSource.Destinations.LocalStore);
                }

                diagnostics.ETWSource.Parameters = SortParameters(diagnostics.ETWSource.Parameters);
            }
            if (diagnostics.FolderSource != null)
            {
                diagnostics.FolderSource = SortFolderSource(diagnostics.FolderSource);
                foreach (DiagnosticsTypeFolderSource folderSource in diagnostics.FolderSource)
                {
                    if (folderSource.Destinations != null)
                    {
                        folderSource.Destinations.AzureBlob = SortAzureBlob(folderSource.Destinations.AzureBlob);
                        folderSource.Destinations.FileStore = SortFileStore(folderSource.Destinations.FileStore);
                        folderSource.Destinations.LocalStore = SortLocalStore(folderSource.Destinations.LocalStore);
                    }

                    folderSource.Parameters = SortParameters(folderSource.Parameters);
                }
            }
        }

        private static DiagnosticsTypeFolderSource[] SortFolderSource(DiagnosticsTypeFolderSource[] folderSources)
        {
            if (folderSources == null)
            {
                return null;
            }
            foreach (DiagnosticsTypeFolderSource folderSource in folderSources)
            {
                folderSource.Parameters = SortParameters(folderSource.Parameters);
            }
            return folderSources.OrderBy(folderSource => folderSource.RelativeFolderPath).ToArray();
        }

        private static AzureBlobType[] SortAzureBlob(AzureBlobType[] azureBlobs)
        {
            if (azureBlobs == null)
            {
                return null;
            }
            foreach (AzureBlobType azureBlob in azureBlobs)
            {
                azureBlob.Parameters = SortParameters(azureBlob.Parameters);
            }
            return azureBlobs.OrderBy(azureBlob => String.Concat(azureBlob.ConnectionString, azureBlob.ContainerName)).ToArray();
        }

        private static AzureBlobETWType[] SortAzureBlob(AzureBlobETWType[] azureBlobs)
        {
            if (azureBlobs == null)
            {
                return null;
            }
            foreach (AzureBlobETWType azureBlob in azureBlobs)
            {
                azureBlob.Parameters = SortParameters(azureBlob.Parameters);
            }
            return azureBlobs.OrderBy(azureBlob => String.Concat(azureBlob.ConnectionString, azureBlob.ContainerName)).ToArray();
        }

        private static FileStoreType[] SortFileStore(FileStoreType[] fileStores)
        {
            if (fileStores == null)
            {
                return null;
            }
            foreach (FileStoreType fileStore in fileStores)
            {
                fileStore.Parameters = SortParameters(fileStore.Parameters);
            }
            return fileStores.OrderBy(fileStore => fileStore.Path).ToArray();
        }

        private static FileStoreETWType[] SortFileStore(FileStoreETWType[] fileStores)
        {
            if (fileStores == null)
            {
                return null;
            }
            foreach (FileStoreETWType fileStore in fileStores)
            {
                fileStore.Parameters = SortParameters(fileStore.Parameters);
            }
            return fileStores.OrderBy(fileStore => fileStore.Path).ToArray();
        }

        private static LocalStoreETWType[] SortLocalStore(LocalStoreETWType[] localStores)
        {
            if (localStores == null)
            {
                return null;
            }
            foreach (LocalStoreETWType localStore in localStores)
            {
                localStore.Parameters = SortParameters(localStore.Parameters);
            }
            return localStores.OrderBy(localStore => localStore.RelativeFolderPath).ToArray();
        }

        private static LocalStoreType[] SortLocalStore(LocalStoreType[] localStores)
        {
            if (localStores == null)
            {
                return null;
            }
            foreach (LocalStoreType localStore in localStores)
            {
                localStore.Parameters = SortParameters(localStore.Parameters);
            }
            return localStores.OrderBy(localStore => localStore.RelativeFolderPath).ToArray();
        }

        private static ParameterType[] SortParameters(ParameterType[] parameters)
        {
            return (parameters == null) ? null : parameters.OrderBy(parameter => parameter.Name).ToArray();
        }
    }
}