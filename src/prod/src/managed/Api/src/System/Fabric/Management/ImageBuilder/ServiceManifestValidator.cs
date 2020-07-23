// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;

    internal class ServiceManifestValidator : IApplicationValidator
    {
        private static readonly string TraceType = "ServiceManifestValidator";

        public void Validate(ApplicationTypeContext applicationTypeContext, bool isComposeDeployment = false, bool isSFVolumeDiskServiceEnabled = false)
        {
            this.ValidateServiceManifests(applicationTypeContext);
            this.ValidateConfiguration(applicationTypeContext);
        }

        public void Validate(ApplicationInstanceContext applicationInstanceContext)
        {            
            // No instance validation required
        }

        private void ValidateServiceManifests(ApplicationTypeContext context)
        {
            IEnumerable<ServiceManifestType> serviceManifestTypes = context.GetServiceManifestTypes();

            // Validates the ServiceManifest
            var duplicateServiceTypeDetector = new DuplicateDetector("ServiceType", "ServiceTypeName");
            foreach (ServiceManifestType serviceManifestType in serviceManifestTypes)
            {
                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                    "Validating ServiceManifest. ApplicationTypeName:{0}, ServiceManifestName:{1}, ServiceManifestVersion:{2}",
                    context.ApplicationManifest.ApplicationTypeName,
                    serviceManifestType.Name,
                    serviceManifestType.Version);

                string serviceManifestFileName = context.GetServiceManifestFileName(serviceManifestType.Name);

                if (!ManifestValidatorUtility.IsValidName(serviceManifestType.Name))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        serviceManifestFileName,
                        StringResources.ImageBuilderError_InvalidName_arg2,
                        "ServiceManifest",
                        serviceManifestType.Name,
                        Path.DirectorySeparatorChar,
                        StringConstants.DoubleDot);
                }

                if (!ManifestValidatorUtility.IsValidVersion(serviceManifestType.Version))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        serviceManifestFileName,
                        StringResources.ImageBuilderError_InvalidVersion_arg2,
                        "ServiceManifestVersion",
                        serviceManifestType.Version,
                        Path.DirectorySeparatorChar,
                        StringConstants.DoubleDot);
                }

                var implicitServiceTypeCount = 0;
                var normalServiceTypeCount = 0;
                var hasServiceGroupTypes = false;

                foreach (object manifestServiceTypeType in serviceManifestType.ServiceTypes)
                {
                    string serviceTypeName = null;
                    string placementConstraints = null;
                    if (manifestServiceTypeType is ServiceTypeType serviceTypeType)
                    {
                        serviceTypeName = serviceTypeType.ServiceTypeName;
                        placementConstraints = serviceTypeType.PlacementConstraints;

                        if (IsImplicitServiceType(serviceTypeType))
                        {
                            ++implicitServiceTypeCount;
                        }
                        else
                        {
                            ++normalServiceTypeCount;
                        }
                    }
                    else
                    {
                        hasServiceGroupTypes = true;

                        ServiceGroupTypeType serviceGroupTypeType = (ServiceGroupTypeType)manifestServiceTypeType;
                        serviceTypeName = serviceGroupTypeType.ServiceGroupTypeName;
                        placementConstraints = serviceGroupTypeType.PlacementConstraints;
                    }

                    this.ValidatePlacementConstraints(serviceManifestFileName, placementConstraints);

                    duplicateServiceTypeDetector.Add(serviceTypeName);
                }

                this.ValidateServiceTypeCombination(
                    serviceManifestFileName,
                    implicitServiceTypeCount,
                    normalServiceTypeCount,
                    hasServiceGroupTypes);

                var activatorCodePackageCount = 0;

                // Validates the CodePackage in the ServiceManifest
                var codePackageDuplicateDetector = new DuplicateDetector("CodePackage", "Name", serviceManifestFileName);
                foreach (CodePackageType codePackageType in serviceManifestType.CodePackage)
                {
                    if (!ManifestValidatorUtility.IsValidName(codePackageType.Name))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            serviceManifestFileName,
                            StringResources.ImageBuilderError_InvalidName_arg2,
                            "CodePackage",
                            codePackageType.Name,
                            Path.DirectorySeparatorChar,
                            StringConstants.DoubleDot);
                    }

                    if (!ManifestValidatorUtility.IsValidVersion(codePackageType.Version))
                    {
                        ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                            TraceType,
                            serviceManifestFileName,
                            StringResources.ImageBuilderError_InvalidVersion_arg2,
                            "CodePackageVersion",
                            codePackageType.Version,
                            Path.DirectorySeparatorChar,
                            StringConstants.DoubleDot);
                    }

                    codePackageDuplicateDetector.Add(codePackageType.Name);

                    if(codePackageType.IsActivator)
                    {
                        if(implicitServiceTypeCount > 0)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_GuestAppWithActivatorCodePackage);
                        }

                        activatorCodePackageCount++;
                        if(activatorCodePackageCount > 1)
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_MultipleActivatorCodePackage);
                        }
                    }

                    var codePackageFolderName = context.BuildLayoutSpecification.GetCodePackageFolder(serviceManifestType.Name, codePackageType.Name);
                    var codePackageArchiveName = context.BuildLayoutSpecification.GetSubPackageArchiveFile(codePackageFolderName);

                    FileLocator fileLocator = null;
                    if (FabricFile.Exists(codePackageArchiveName))
                    {
                        fileLocator = new FileLocator(codePackageArchiveName, true);
                    }
                    else if (FabricDirectory.Exists(codePackageFolderName))
                    {
                        fileLocator = new FileLocator(codePackageFolderName, false);
                    }
                    else
                    {
                        // If the code package doesn't exist, it means it was already provisioned;
                        // validation must have passed at the time of first provisioning
                        continue;
                    }

                    if (codePackageType.SetupEntryPoint != null)
                    {
                        this.ValidateEntryPointPath(serviceManifestFileName, fileLocator, codePackageType.SetupEntryPoint.ExeHost.Program,
                            codePackageType.SetupEntryPoint.ExeHost.IsExternalExecutable);
                    }

                    if (codePackageType.EntryPoint != null && codePackageType.EntryPoint.Item != null)
                    {
                        if (codePackageType.EntryPoint.Item is DllHostEntryPointType)
                        {
                            this.ValidateEntryPoint(serviceManifestFileName, fileLocator, (DllHostEntryPointType)codePackageType.EntryPoint.Item);
                        }
                        else if (codePackageType.EntryPoint.Item is ExeHostEntryPointType)
                        {
                            this.ValidateEntryPoint(serviceManifestFileName, fileLocator, (ExeHostEntryPointType)codePackageType.EntryPoint.Item);
                        }
        
                    }
                }

                // Validates the ConfigPackage in the ServiceManifest
                if (serviceManifestType.ConfigPackage != null)
                {
                    DuplicateDetector configPackageDuplicateDetector = new DuplicateDetector("ConfigPackage", "Name", serviceManifestFileName);
                    foreach (ConfigPackageType configPackageType in serviceManifestType.ConfigPackage)
                    {
                        if (!ManifestValidatorUtility.IsValidName(configPackageType.Name))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_InvalidName_arg2,
                                "ConfigPackage",
                                configPackageType.Name,
                                Path.DirectorySeparatorChar,
                                StringConstants.DoubleDot);
                        }

                        if (!ManifestValidatorUtility.IsValidVersion(configPackageType.Version))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_InvalidVersion_arg2,
                                "ConfigPackageVersion",
                                configPackageType.Version,
                                Path.DirectorySeparatorChar,
                                StringConstants.DoubleDot);
                        }     

                        configPackageDuplicateDetector.Add(configPackageType.Name);
                    }
                }

                // Validates the DataPackage in the ServiceManifest
                if (serviceManifestType.DataPackage != null)
                {
                    DuplicateDetector dataPackageDuplicateDetector = new DuplicateDetector("DataPackage", "Name", serviceManifestFileName);
                    foreach (DataPackageType dataPackageType in serviceManifestType.DataPackage)
                    {
                        if (!ManifestValidatorUtility.IsValidName(dataPackageType.Name))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_InvalidName_arg2,
                                "DataPackage",
                                dataPackageType.Name,
                                Path.DirectorySeparatorChar,
                                StringConstants.DoubleDot);
                        }

                        if (!ManifestValidatorUtility.IsValidVersion(dataPackageType.Version))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_InvalidVersion_arg2,
                                "DataPackageVersion",
                                dataPackageType.Version,
                                Path.DirectorySeparatorChar,
                                StringConstants.DoubleDot);
                        }

                        dataPackageDuplicateDetector.Add(dataPackageType.Name);
                    }
                }

                // Validates the Resources in the ServiceManifest
                if (serviceManifestType.Resources != null && serviceManifestType.Resources.Endpoints != null)
                {
                    DuplicateDetector resourceNameDuplicateDetector = new DuplicateDetector("Endpoint", "Name", serviceManifestFileName);
                    foreach (var endpoint in serviceManifestType.Resources.Endpoints)
                    {
                        if (endpoint.Name.Contains(';') || endpoint.Name.Contains(','))
                        {
                            ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                TraceType,
                                serviceManifestFileName,
                                StringResources.ImageBuilderError_InvalidName_arg2,
                                "EndppointResource",
                                endpoint.Name,
                                ",",
                                ";");
                        }

                        if (!string.IsNullOrEmpty(endpoint.CodePackageRef))
                        {
                            var matchingCodePackage = serviceManifestType.CodePackage.FirstOrDefault(
                            codePackageType => ImageBuilderUtility.Equals(codePackageType.Name, endpoint.CodePackageRef));

                            if (matchingCodePackage == null)
                            {
                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                    TraceType,
                                    serviceManifestFileName,
                                    StringResources.ImageBuilderError_InvalidRefSameFile,
                                    "CodePackageRef",
                                    endpoint.CodePackageRef,
                                    "EndppointResource",
                                    endpoint.Name,
                                    "CodePackage",
                                    "ServiceManifest");
                            }
                        }
                        
                        resourceNameDuplicateDetector.Add(endpoint.Name);
                    }
                }
            }
        }

        bool IsImplicitServiceType(ServiceTypeType serviceTypeType)
        {
            return
                (serviceTypeType is StatelessServiceTypeType && ((StatelessServiceTypeType)serviceTypeType).UseImplicitHost) ||
                (serviceTypeType is StatefulServiceTypeType && ((StatefulServiceTypeType)serviceTypeType).UseImplicitHost);
        }

        void ValidateServiceTypeCombination(
            string serviceManifestFileName,
            int implicitServiceTypeCount,
            int normalServiceTypeCount,
            bool hasServiceGroupTypes)
        {
            if(implicitServiceTypeCount > 0 && (normalServiceTypeCount > 0 || hasServiceGroupTypes))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_InvalidServiceTypeCombination);
            }
        }

        private void ValidateEntryPoint(string serviceManifestFileName, FileLocator fileLocator, ExeHostEntryPointType entryPoint)
        {
            this.ValidateEntryPointPath(serviceManifestFileName, fileLocator, entryPoint.Program, entryPoint.IsExternalExecutable);
        }

        private void ValidateEntryPoint(string serviceManifestFileName, FileLocator fileLocator, DllHostEntryPointType entryPoint)
        {
            bool hasNative = false;
            foreach (var item in entryPoint.Items)
            {
                ManagedAssemblyType managedAssembly = item as ManagedAssemblyType;
                UnmanagedDllType unmanagedDll = item as UnmanagedDllType;

                hasNative = hasNative || unmanagedDll != null;

                if (unmanagedDll != null)
                {
                    this.ValidateEntryPointPath(serviceManifestFileName, fileLocator, unmanagedDll.Value.Trim());
                }
                else
                {
                    this.ValidateManageAssembly(serviceManifestFileName, fileLocator, managedAssembly.Value.Trim());
                }
            }

            if (hasNative && entryPoint.IsolationPolicy != DllHostEntryPointTypeIsolationPolicy.DedicatedProcess)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_InvalidIsolationPolicyForNativeDll);
            }
        }

        private void ValidateManageAssembly(string serviceManifestFileName, FileLocator fileLocator, string entryPointValue)
        {
            AssemblyName assemblyName = null;
            try
            {
                assemblyName = new AssemblyName(entryPointValue);
            }
            catch (Exception exception)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    exception,
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_InvalidValue,
                    "EntryPoint",
                    entryPointValue);
            }

            if (!fileLocator.AssemblyExists(assemblyName))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_EntryPointInvalidOrNotFound,
                    entryPointValue,
                    fileLocator.DirectoryOrArchivePath);
            }
        }

        private void ValidateEntryPointPath(string serviceManifestFileName, FileLocator fileLocator, string program, bool isExternalExecutable = false)
        {
            string entryPointPath = this.GetEntryPointPath(program, serviceManifestFileName);

            if (string.IsNullOrEmpty(entryPointPath))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_NullOrEmptyError,
                    "EntryPoint");
            }
            else if (entryPointPath.IndexOfAny(System.IO.Path.GetInvalidPathChars()) != -1)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_InvalidValue,
                    "EntryPoint",
                    entryPointPath);
            }
            else if (System.IO.Path.IsPathRooted(entryPointPath))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_RootedEntryPointNotSupported,
                    entryPointPath);
            }

            if (isExternalExecutable)
            {
                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                    "EntryPointPath {0} is external executable",
                    entryPointPath);
            }
            else if (!fileLocator.FileExists(entryPointPath))
            {
#if !DotNetCoreClrLinux
                if (string.IsNullOrEmpty(Path.GetExtension(entryPointPath)) && fileLocator.FileExists(entryPointPath + ".exe"))
                {
                    return;
                }
#endif
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    serviceManifestFileName,
                    StringResources.ImageBuilderError_EntryPointNotFound,
                    entryPointPath);
            }
        }

        private string GetEntryPointPath(string pathWithArguments, string serviceManifestFileName)
        {
            if (pathWithArguments.StartsWith("\"", StringComparison.OrdinalIgnoreCase))
            {
                int nextQuote = pathWithArguments.IndexOf("\"", 1, StringComparison.OrdinalIgnoreCase);

                if (nextQuote != -1)
                {
                    return pathWithArguments.Substring(1, nextQuote - 1);
                }
                else
                {
                    ImageBuilder.TraceSource.WriteError(
                        TraceType,
                        "Invalid EntryPoint '{0}'. Quotes do not match.",                        
                        pathWithArguments);

                    throw new FabricImageBuilderValidationException(
                                string.Format(
                                    CultureInfo.InvariantCulture,
                                    StringResources.ImageBuilderError_EntryPointQuotesDontMatch,
                                    pathWithArguments),
                                serviceManifestFileName);
                }
            }
            else
            {
                string[] split = pathWithArguments.Split(new char[] { ' ' });
                return split[0];
            }
        }

        private void ValidateConfiguration(ApplicationTypeContext context)
        {
            Dictionary<string, IEnumerable<ConfigPackage>> configurationPackageDictionary = new Dictionary<string,IEnumerable<ConfigPackage>>();
            foreach (var serviceManifest in context.ServiceManifests)
            {
                configurationPackageDictionary.Add(serviceManifest.ServiceManifestType.Name, serviceManifest.ConfigPackages);
            }
            
            ApplicationManifestTypeServiceManifestImport[] serviceManifestImports = context.ApplicationManifest.ServiceManifestImport;
            bool hasEncryptedParameter = false;

            foreach (KeyValuePair<string, IEnumerable<ConfigPackage>> configKeyValuePair in configurationPackageDictionary)
            {
                if (configKeyValuePair.Value != null)
                {
                    ApplicationManifestTypeServiceManifestImport matchingServiceManifestImport = serviceManifestImports.FirstOrDefault<ApplicationManifestTypeServiceManifestImport>(
                        serviceManifestImport => ImageBuilderUtility.Equals(serviceManifestImport.ServiceManifestRef.ServiceManifestName, configKeyValuePair.Key));

                    ReleaseAssert.AssertIf(
                        matchingServiceManifestImport == null,
                        "Could not find matching ServiceManifest with Name {0}",
                        configKeyValuePair.Key);

                    foreach (ConfigPackage configurationPackage in configKeyValuePair.Value)
                    {
                        ImageBuilder.TraceSource.WriteInfo(
                            TraceType,
                            "Validating the configuration. ApplicationTypeName:{0}, ServiceManifestName:{1}, ConfigPackageName:{2}",
                            context.ApplicationManifest.ApplicationTypeName,
                            matchingServiceManifestImport.ServiceManifestRef.ServiceManifestName,
                            configurationPackage.ConfigPackageType.Name);

                        if (configurationPackage.SettingsType != null && configurationPackage.SettingsType.Section != null)
                        {
                            string configPackageDirectory = context.BuildLayoutSpecification.GetConfigPackageFolder(configKeyValuePair.Key, configurationPackage.ConfigPackageType.Name);
                            string configPackageArchive = context.BuildLayoutSpecification.GetSubPackageArchiveFile(configPackageDirectory);

                            if (!FabricDirectory.Exists(configPackageDirectory) && !FabricFile.Exists(configPackageArchive))
                            {
                                continue;
                            }

                            ConfigOverrideType matchingConfigOverride = null;
                            if (matchingServiceManifestImport.ConfigOverrides != null)
                            {
                                matchingConfigOverride = matchingServiceManifestImport.ConfigOverrides.FirstOrDefault<ConfigOverrideType>(
                                    configOverride => ImageBuilderUtility.Equals(configOverride.Name, configurationPackage.ConfigPackageType.Name));
                            }

                            string settingsFileName = context.BuildLayoutSpecification.GetSettingsFile(configPackageDirectory);

                            DuplicateDetector sectionDuplicateDetector = new DuplicateDetector("Section", "Name", settingsFileName);
                            foreach (SettingsTypeSection section in configurationPackage.SettingsType.Section)
                            {
                                sectionDuplicateDetector.Add(section.Name);

                                SettingsOverridesTypeSection matchingConfigOverrideSection = null;
                                if (matchingConfigOverride != null && matchingConfigOverride.Settings != null)
                                {
                                    matchingConfigOverrideSection = matchingConfigOverride.Settings.FirstOrDefault<SettingsOverridesTypeSection>(
                                        configOverrideSection => ImageBuilderUtility.Equals(configOverrideSection.Name, section.Name));
                                }

                                if (section.Parameter != null)
                                {
                                    DuplicateDetector settingDuplicateDetector = new DuplicateDetector("Parameter", "Name", settingsFileName);
                                    foreach (SettingsTypeSectionParameter parameter in section.Parameter)
                                    {
                                        settingDuplicateDetector.Add(parameter.Name);

                                        if (!hasEncryptedParameter)
                                        {
                                            hasEncryptedParameter = parameter.IsEncrypted;
                                        }

                                        if (parameter.MustOverride)
                                        {
                                            SettingsOverridesTypeSectionParameter matchingOverrideParameter = null;
                                            if (matchingConfigOverrideSection != null && matchingConfigOverrideSection.Parameter != null)
                                            {
                                                matchingOverrideParameter = matchingConfigOverrideSection.Parameter.FirstOrDefault<SettingsOverridesTypeSectionParameter>(
                                                    configOverrideSetting => ImageBuilderUtility.Equals(configOverrideSetting.Name, parameter.Name));
                                            }

                                            if (matchingOverrideParameter == null)
                                            {
                                                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                                                    TraceType,
                                                    settingsFileName,
                                                    StringResources.ImageBuilderError_ParameterNotOverriden,
                                                    parameter.Name);
                                            }
                                        }

                                        if (!String.IsNullOrEmpty(parameter.Type))
                                        {
                                            this.VerifySourceLocation(parameter.Name, "Settings.xml", parameter.Type);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (hasEncryptedParameter &&
                RequireCertACLingForUsers(context.ApplicationManifest.Principals) &&
                (context.ApplicationManifest.Certificates == null || context.ApplicationManifest.Certificates.SecretsCertificate == null))
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        context.GetApplicationManifestFileName(),
                        StringResources.ImageBuilderError_EncryptedSettingsNoCertInAppManifest);
            }
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

        //If only LocalSystem is specified as Principal no need to precheck since LocalSystem has access to certificate
        private bool RequireCertACLingForUsers(SecurityPrincipalsType principals)
        {
            if (principals == null ||
                principals.Users == null)
            {
                return false;
            }
            foreach (SecurityPrincipalsTypeUser users in principals.Users)
            {
                if (users.AccountType != SecurityPrincipalsTypeUserAccountType.LocalSystem)
                {
                    return true;
                }
            }
            return false;
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

        //
        // Inner classes
        //

        private class FileLocator
        {
            private readonly bool isArchive;
            private readonly string directoryOrArchivePath;

            public FileLocator(string path, bool isArchive)
            {
                this.directoryOrArchivePath = path;
                this.isArchive = isArchive;
            }

            public string DirectoryOrArchivePath { get { return this.directoryOrArchivePath; } }

            public bool AssemblyExists(AssemblyName assemblyName)
            {
                if (isArchive)
                {
                    return ImageBuilderUtility.IsAnyInArchive(this.DirectoryOrArchivePath, new string[] 
                    {
                        string.Format(CultureInfo.InvariantCulture, "{0}.exe", assemblyName.Name),
                        string.Format(CultureInfo.InvariantCulture, "{0}.dll", assemblyName.Name)
                    });
                }
                else
                {
                    string searchPattern = string.Format(CultureInfo.InvariantCulture, "{0}.*", assemblyName.Name);
                    var matchingFiles = Directory.EnumerateFiles(this.DirectoryOrArchivePath, searchPattern, SearchOption.AllDirectories);
                    int count = matchingFiles.Count(file => ImageBuilderUtility.Equals(Path.GetExtension(file), ".exe") || ImageBuilderUtility.Equals(Path.GetExtension(file), ".dll"));

                    return (count > 0);
                }
            }

            public bool FileExists(string filePath)
            {
                if (isArchive)
                {
                    return ImageBuilderUtility.IsInArchive(this.DirectoryOrArchivePath, filePath);
                }
                else
                {
                    return FabricFile.Exists(Path.Combine(this.DirectoryOrArchivePath, filePath));
                }
            }
        }
    }
}