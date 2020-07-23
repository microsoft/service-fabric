// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
#if !DotNetCoreClr
    using System.DirectoryServices;
    using System.DirectoryServices.ActiveDirectory;
#endif
    using System.Fabric.Common;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Reflection;
    using System.Security;
    using Fabric.FabricDeployer;
    using Text.RegularExpressions;

    internal class FabricSettingsValidator
    {
        public int NonSeedNodeVoteCount { get; private set; }
        private List<InfrastructureNodeType> infrastructureInformation;
        private SecurityPrincipalsTypeUserAccountType fabricAccountType = SecurityPrincipalsTypeUserAccountType.NetworkService;
        public bool ShouldRegisterSpnForMachineAccount { get; private set; }
        public ClusterManifestType clusterManifest { get; set; }

        public bool IsTVSEnabled { get { return this.tokenValidationServiceValidator == null ? false : this.tokenValidationServiceValidator.IsTVSEnabled; } }

        public bool IsDCAEnabled { get { return this.dcaSettingsValidator == null ? false : this.dcaSettingsValidator.IsDCAEnabled; } }
        public bool IsHttpGatewayEnabled { get { return this.httpGatewaySettingsValidator == null ? false : this.httpGatewaySettingsValidator.IsHttpGatewayEnabled; } }
        public bool IsHttpAppGatewayEnabled { get { return this.httpAppGatewaySettingsValidator == null ? false : this.httpAppGatewaySettingsValidator.IsHttpAppGatewayEnabled; } }
        public bool IsDCAFileStoreEnabled { get { return this.dcaSettingsValidator == null ? false : this.dcaSettingsValidator.IsDCAFileStoreEnabled; } }
        public bool IsDCATableStoreEnabled { get { return this.dcaSettingsValidator == null ? false : this.dcaSettingsValidator.IsDCATableStoreEnabled; } }
        public bool IsAppLogCollectionEnabled { get { return this.dcaSettingsValidator == null ? false : this.dcaSettingsValidator.IsAppLogCollectionEnabled; } }
        public SecureString DiagnosticsFileStoreConnectionString { get { return this.dcaSettingsValidator == null ? null : this.dcaSettingsValidator.DiagnosticsFileStoreConnectionString; } }
        public SecureString DiagnosticsTableStoreConnectionString { get { return this.dcaSettingsValidator == null ? null : this.dcaSettingsValidator.DiagnosticsTableStoreConnectionString; } }
        public List<DCASettingsValidator.PluginInfo> DCAConsumers { get { return this.dcaSettingsValidator == null ? null : this.dcaSettingsValidator.ConsumerList; } }
        public int AppLogDirectoryQuotaInMB { get { return this.dcaSettingsValidator == null ? -1 : this.dcaSettingsValidator.AppLogDirectoryQuotaInMB; } }
        public string LogContainer { get { return this.dcaSettingsValidator == null ? null : this.dcaSettingsValidator.LogContainer; } }
        public string CrashDumpContainer { get { return this.dcaSettingsValidator == null ? null : this.dcaSettingsValidator.CrashDumpContainer; } }
        public string DiagnosticsTableStoreName { get { return this.dcaSettingsValidator == null ? null : this.dcaSettingsValidator.DiagnosticsTableStoreName; } }

        public bool IsRunAsPolicyEnabled
        {
            get
            {
                return this.windowsFabricSettings.GetParameter(FabricValidatorConstants.SectionNames.Hosting,
                        FabricValidatorConstants.ParameterNames.RunAsPolicyEnabled).GetValue<bool>();
            }
        }

        public bool IsV2NodeIdGeneratorEnabled
        {
            get
            {
                return this.windowsFabricSettings.GetParameter(FabricValidatorConstants.SectionNames.Federation,
                        FabricValidatorConstants.ParameterNames.UseV2NodeIdGenerator).GetValue<bool>();
            }
        }

        public string NodeIdGeneratorVersion
        {
            get
            {
                return this.windowsFabricSettings.GetParameter(FabricValidatorConstants.SectionNames.Federation,
                        FabricValidatorConstants.ParameterNames.NodeIdGeneratorVersion).GetValue<string>();
            }
        }

        public SecureString ImageStoreConnectionString
        {
            get
            {
                return this.windowsFabricSettings.GetParameter(FabricValidatorConstants.SectionNames.Management,
                    FabricValidatorConstants.ParameterNames.ImageStoreConnectionString).GetSecureValue(this.storeName);
            }
        }

        public FabricSettingsValidator(WindowsFabricSettings windowsFabricSettings, ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructureInformation)
        {
            this.windowsFabricSettings = windowsFabricSettings;
            this.dcaSettingsValidator = windowsFabricSettings.DcaSettingsValidator;
            this.storeName = windowsFabricSettings.StoreName;
            this.nodeTypes = clusterManifest.NodeTypes;
            this.clusterManifest = clusterManifest;
            this.infrastructureInformation = infrastructureInformation;
            this.ShouldRegisterSpnForMachineAccount = false;
        }

        public bool ValidateImageStore(bool isSingleMachineDeployment)
        {
            if (!this.windowsFabricSettings.SettingsMap.ContainsKey(FabricValidatorConstants.SectionNames.Management) ||
                !this.windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.Management].ContainsKey(FabricValidatorConstants.ParameterNames.ImageStoreConnectionString))
            {
                return true;
            }

            SecureString imageStoreRoot = this.ImageStoreConnectionString;
            char[] secureImageStoreChars = FabricValidatorUtility.SecureStringToCharArray(this.ImageStoreConnectionString);

            if ((secureImageStoreChars == null) || FabricValidatorUtility.EqualsIgnoreCase(secureImageStoreChars, FabricValidatorConstants.DefaultTag))
            {
                if (this.windowsFabricSettings.GetParameter(FabricValidatorConstants.SectionNames.Management,
                    FabricValidatorConstants.ParameterNames.ImageStoreConnectionString).IsEncrypted)
                {
                    WriteError(
                        "Value of Section:{0} and Parameter:{1} cannot be encypted when the value is {2}",
                        FabricValidatorConstants.SectionNames.Management,
                        FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                        FabricValidatorConstants.DefaultTag);
                    return false;
                }

                // string.empty or _default_ is allowed only in scale-min deployment
                if (isSingleMachineDeployment)
                {
                    return true;
                }
                else
                {
                    WriteError(
                        "Section {0} parameter {1} cannot be {2} or null for multimachine deployment",
                        FabricValidatorConstants.SectionNames.Management,
                        FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                        FabricValidatorConstants.DefaultTag);
                    return false;
                }
            }

            if (FabricValidatorUtility.StartsWithIgnoreCase(secureImageStoreChars, FabricValidatorConstants.FileImageStoreConnectionStringPrefix))
            {
                if (!ValidateImageStoreServiceSettings(true)) { return false; }
                IEnumerable<char> secureSubArray = secureImageStoreChars.Skip(FabricValidatorConstants.FileImageStoreConnectionStringPrefix.Length);

                // Rooted drives are allowed only in scale-min deployment
                if (secureSubArray.Contains(Path.VolumeSeparatorChar) && !isSingleMachineDeployment)
                {
                    WriteError(
                        "Section {0} parameter {1} cannont be rooted in a drive for multi machine deployments",
                        FabricValidatorConstants.SectionNames.Management,
                        FabricValidatorConstants.ParameterNames.ImageStoreConnectionString);
                    return false;
                }

                try
                {
                    string rootUri, accountName, accountPassword;
                    bool isManagedServiceAccount;
                    ImageStoreFactory.ParseFileImageStoreConnectionString(new string(secureImageStoreChars), out rootUri, out accountName, out accountPassword, out isManagedServiceAccount);
                }
                catch (ArgumentException ae)
                {
                    WriteError(ae.Message);
                    return false;
                }
            }
            else if (FabricValidatorUtility.StartsWithIgnoreCase(secureImageStoreChars, FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix))
            {
                if (!ValidateImageStoreServiceSettings(true)) { return false; }
            }
            else if (FabricValidatorUtility.StartsWithIgnoreCase(secureImageStoreChars, FabricValidatorConstants.FabricImageStoreConnectionStringPrefix))
            {
                if (!this.ValidateImageStoreServiceSettings()) { return false; }

                if (!this.IsRunAsPolicyEnabled)
                {
                    WriteError("RunAsPolicyEnabled should be set to true when using ImageStoreService");
                    return false;
                }
            }
            else
            {
                WriteError(
                    "Value for section {0} parameter {1} should start with {2}, {3} or {4}",
                    FabricValidatorConstants.SectionNames.Management,
                    FabricValidatorConstants.ParameterNames.ImageStoreConnectionString,
                    FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix,
                    FabricValidatorConstants.FileImageStoreConnectionStringPrefix,
                    FabricValidatorConstants.FabricImageStoreConnectionStringPrefix);
                return false;
            }

            return true;
        }

        public void ValidateSettings()
        {

            this.httpGatewaySettingsValidator = new HttpGatewaySettingsValidator(
                this.nodeTypes,
                this.windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.HttpGateway),
                this.infrastructureInformation);

            this.httpAppGatewaySettingsValidator = new HttpAppGatewaySettingsValidator(
                this.nodeTypes,
                this.windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.HttpAppGateway),
                this.infrastructureInformation,
                this.clusterManifest,
                this.windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.ServiceCommonNameAndIssuer));

            this.tokenValidationServiceValidator = new TokenValidationServiceSettingsValidator(
                this.GetAllTVSSettings(),
                this.windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.HttpGateway),
                this.nodeTypes,
                this.infrastructureInformation,
                this.IsRunAsPolicyEnabled);

            VerifyRequiredParameters();
            VerifyNoLeadingTrailingSpaces();
            VerifyVoteConfiguration();
            VerifyDependencies();
            VerifyNodeTypes();
            VerifyExpectedClusterSize();
            VerifyEncryptionFlag();
            VerifyClusterHealthPolicy();
            dcaSettingsValidator.ValidateDCASettings();
            RunOtherConfigurationValidatorsInTheAssembly();
            this.httpGatewaySettingsValidator.ValidateHttpGatewaySettings();
            this.httpAppGatewaySettingsValidator.ValidateHttpGatewaySettings();
            this.tokenValidationServiceValidator.ValidateSettings();
            VerifyRunAsSettings(FabricValidatorConstants.SectionNames.RunAs);
            VerifyRunAsSettings(FabricValidatorConstants.SectionNames.RunAs_Fabric);
            VerifyRunAsSettings(FabricValidatorConstants.SectionNames.RunAs_DCA);
            VerifyRunAsSettings(FabricValidatorConstants.SectionNames.RunAs_HttpGateway);
            VerifySecuritySettings(); // Must be placed after VerifyRunAsSettings()
            ValidateRunAsEnabledIfInfrastructureServiceEnabled();
            ValidateFabricHostSettings();
            ValidateAutomateUpdateEnabledSetting();
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public IEnumerable<KeyValuePair<string, string>> CompareSettings(ClusterManifestType newClusterManifest, string nodeTypeNameFilter)
        {
            List<KeyValuePair<string, string>> modifiedSettings = new List<KeyValuePair<string, string>>();

            CompareNodeTypeSettings(newClusterManifest, nodeTypeNameFilter, modifiedSettings);

            //Compute modified and removed settings
            var newWindowsFabricSettings = new WindowsFabricSettings(newClusterManifest);
            foreach (var section in this.windowsFabricSettings.SettingsMap)
            {
                if (windowsFabricSettings.IsDCASetting(section.Key))
                {
                    continue;
                }

                var matchingNewSection = newWindowsFabricSettings.SettingsMap.FirstOrDefault(newSection => FabricValidatorUtility.EqualsIgnoreCase(newSection.Key, section.Key));
                foreach (var parameter in section.Value)
                {
                    if (matchingNewSection.Value != null && matchingNewSection.Value.ContainsKey(parameter.Key))
                    {
                        if (HasParameterValueChanged(parameter.Value, matchingNewSection.Value[parameter.Key], newWindowsFabricSettings.StoreName))
                        {
                            modifiedSettings.Add(new KeyValuePair<string, string>(section.Key, parameter.Key));
                        }
                    }
                    else
                    {
                        modifiedSettings.Add(new KeyValuePair<string, string>(section.Key, parameter.Key));
                    }
                }
            }

            //Compute newly added settings
            foreach (var newSection in newWindowsFabricSettings.SettingsMap)
            {
                if (newWindowsFabricSettings.IsDCASetting(newSection.Key))
                {
                    continue;
                }

                var matchingSection = this.windowsFabricSettings.SettingsMap.FirstOrDefault(section => FabricValidatorUtility.EqualsIgnoreCase(newSection.Key, section.Key));
                foreach (var newParameter in newSection.Value)
                {
                    if (matchingSection.Value == null || !matchingSection.Value.ContainsKey(newParameter.Key))
                    {
                        modifiedSettings.Add(new KeyValuePair<string, string>(newSection.Key, newParameter.Key));
                    }
                }
            }

            List<KeyValuePair<string, string>> modifiedStaticSettings = new List<KeyValuePair<string, string>>();
            bool invalidChangeDetected = false;
            foreach (var changedSetting in modifiedSettings)
            {
                if (WindowsFabricSettings.IsSectionValidationDisabled(changedSetting.Key))
                {
                    if (!WindowsFabricSettings.IsSectionAssumedToBeDynamic(changedSetting.Key))
                    {
                        // For sections where validation is disabled, assume any change in parameter value
                        // will cause restart
                        modifiedStaticSettings.Add(changedSetting);

                        WriteInfo(
                            "FabricSetting Section:{0}, Parameter:{1} has changed; assumed an upgrade policy of Static since validation is disabled.",
                            changedSetting.Key,
                            changedSetting.Value);
                    }
                    continue;
                }

                try
                {
                    WindowsFabricSettings.SettingsValue matchingConfigDesc = null;
                    if (WindowsFabricSettings.IsPropertyGroupSection(changedSetting.Key))
                    {
                        // As this is property group section, it could happen that new parameters(user defined) can't be located in current cluster manifest when we add those sections,
                        // or they can't be located in new cluster manfiest if we remove them, so we get their config description from correct cluster manifest settings
                        var changedFabricPropertyGroupSettings = this.windowsFabricSettings;

                        // Check first do we have property group setting in the current cluster manfiest, or try to find it in new cluster manfiest
                        var matchingSection = changedFabricPropertyGroupSettings.SettingsMap.FirstOrDefault(
                            section => FabricValidatorUtility.EqualsIgnoreCase(section.Key, changedSetting.Key));
                        if(!matchingSection.Value.ContainsKey(changedSetting.Value))
                        {
                            changedFabricPropertyGroupSettings = newWindowsFabricSettings;
                        }

                        matchingConfigDesc = changedFabricPropertyGroupSettings.SettingsMap.First(
                            config => FabricValidatorUtility.EqualsIgnoreCase(config.Key, changedSetting.Key)).Value.First(
                            parameter => FabricValidatorUtility.EqualsIgnoreCase(parameter.Key, changedSetting.Value)).Value;
                    }
                    else
                    {
                        matchingConfigDesc = this.windowsFabricSettings.SettingsMap.First(
                            config => FabricValidatorUtility.EqualsIgnoreCase(config.Key, changedSetting.Key)).Value.First(
                            parameter => FabricValidatorUtility.EqualsIgnoreCase(parameter.Key, changedSetting.Value)).Value;
                    }

                    if (matchingConfigDesc.UpgradePolicy == UpgradePolicy.Static)
                    {
                        modifiedStaticSettings.Add(changedSetting);
                    }
                    else if (matchingConfigDesc.UpgradePolicy == UpgradePolicy.SingleChange)
                    {
                        WindowsFabricSettings.SettingsValue newMatchingConfigDesc = null;
                        var newMatchingSection = newWindowsFabricSettings.SettingsMap.FirstOrDefault(
                            section => FabricValidatorUtility.EqualsIgnoreCase(section.Key, changedSetting.Key));
                        if (newMatchingSection.Value.ContainsKey(changedSetting.Value))
                        {
                            newMatchingConfigDesc = newMatchingSection.Value.First(parameter => FabricValidatorUtility.EqualsIgnoreCase(parameter.Key, changedSetting.Value)).Value;
                        }

                        if (!IsSingleChangeAllowed(matchingConfigDesc, newMatchingConfigDesc))
                        {
                            WriteError("Section:{0}, Parameter:{1}, UpgradePolicy:{2} has changed. SingleChange policies can only be changed once.", changedSetting.Key, changedSetting.Value, matchingConfigDesc.UpgradePolicy);
                        }
                        else
                        {
                            modifiedStaticSettings.Add(changedSetting);
                        }
                    }
                    else if (matchingConfigDesc.UpgradePolicy == UpgradePolicy.NotAllowed)
                    {
                        invalidChangeDetected = true;
                    }

                    WriteInfo(
                        "FabricSetting Section:{0}, Parameter:{1} has changed. UpgradePolicy:{2}",
                        changedSetting.Key,
                        changedSetting.Value,
                        matchingConfigDesc.UpgradePolicy);
                }
                catch (InvalidOperationException)
                {
                    WriteError("The FabricSetting Section:{0}, Parameter:{1} is not valid.", changedSetting.Key, changedSetting.Value);
                }
            }

            // If NotAllowed then return false
            if (invalidChangeDetected)
            {
                WriteError("The upgrade policy is not allowed");
            }

            RunOtherConfigurationUpgradeValidatorsInTheAssembly(newWindowsFabricSettings);
            return modifiedStaticSettings;
        }

        private void CompareNodeTypeSettings(ClusterManifestType newClusterManifest, string nodeTypeNameFilter, List<KeyValuePair<string, string>> modifiedSettings)
        {
            if (string.IsNullOrEmpty(nodeTypeNameFilter))
            {
                // Compute changes for modified NodeType
                newClusterManifest.NodeTypes.ForEach(
                    newNodeType =>
                    {
                        var matchingExistingNodeType = this.clusterManifest.NodeTypes.FirstOrDefault(existingNodeType => ImageBuilderUtility.Equals(newNodeType.Name, existingNodeType.Name));
                        if (matchingExistingNodeType != null)
                        {
                            CompareNodeTypeSettings(newNodeType, matchingExistingNodeType, modifiedSettings);
                        }
                    });

                // Compute changes for removed NodeTypes
                this.clusterManifest.NodeTypes.ForEach(
                    existingNodeType =>
                    {
                        var matchingNewNodeType = newClusterManifest.NodeTypes.FirstOrDefault(newNodeType => ImageBuilderUtility.Equals(newNodeType.Name, existingNodeType.Name));
                        if(matchingNewNodeType == null)
                        {
                            CompareNodeTypeSettings(null, existingNodeType, modifiedSettings);
                        }
                    });
            }
            else
            {
                var nodeTypeInNewClusterManifest = newClusterManifest.NodeTypes.FirstOrDefault(nodeType => ImageBuilderUtility.Equals(nodeType.Name, nodeTypeNameFilter));
                var nodeTypeInExistingClusterManifest = this.clusterManifest.NodeTypes.FirstOrDefault(nodeType => ImageBuilderUtility.Equals(nodeType.Name, nodeTypeNameFilter));

                CompareNodeTypeSettings(nodeTypeInNewClusterManifest, nodeTypeInExistingClusterManifest, modifiedSettings);
            }
        }

        private void CompareNodeTypeSettings(
            ClusterManifestTypeNodeType nodeTypeInNewClusterManifest,
            ClusterManifestTypeNodeType nodeTypeInExistingClusterManifest,
            List<KeyValuePair<string, string>> modifiedSettings)
        {
            ReleaseAssert.AssertIf(nodeTypeInExistingClusterManifest == null && nodeTypeInNewClusterManifest == null, "Both nodeTypeInNewClusterManifest and nodeTypeInExistingClusterManifest cannot be null");

            CompareProperties(
                FabricValidatorConstants.SectionNames.NodeProperties,
                nodeTypeInNewClusterManifest != null ? nodeTypeInNewClusterManifest.PlacementProperties : null,
                nodeTypeInExistingClusterManifest != null ? nodeTypeInExistingClusterManifest.PlacementProperties : null,
                modifiedSettings);

            CompareProperties(
                FabricValidatorConstants.SectionNames.NodeCapacities,
                nodeTypeInNewClusterManifest != null ? nodeTypeInNewClusterManifest.Capacities : null,
                nodeTypeInExistingClusterManifest != null ? nodeTypeInExistingClusterManifest.Capacities : null,
                modifiedSettings);

            CompareCertificates(
                FabricValidatorConstants.SectionNames.FabricNode,
                "Cluster" /*parameterNamePrefix*/,
                nodeTypeInNewClusterManifest != null && nodeTypeInNewClusterManifest.Certificates != null ? nodeTypeInNewClusterManifest.Certificates.ClusterCertificate : null,
                nodeTypeInExistingClusterManifest != null && nodeTypeInExistingClusterManifest.Certificates != null? nodeTypeInExistingClusterManifest.Certificates.ClusterCertificate : null,
                modifiedSettings);

            CompareCertificates(
                FabricValidatorConstants.SectionNames.FabricNode,
                "ServerAuth" /*parameterNamePrefix*/,
                nodeTypeInNewClusterManifest != null && nodeTypeInNewClusterManifest.Certificates != null ? nodeTypeInNewClusterManifest.Certificates.ServerCertificate : null,
                nodeTypeInExistingClusterManifest != null && nodeTypeInExistingClusterManifest.Certificates != null ? nodeTypeInExistingClusterManifest.Certificates.ServerCertificate : null,
                modifiedSettings);

            CompareCertificates(
                FabricValidatorConstants.SectionNames.FabricNode,
                "ClientAuth" /*parameterNamePrefix*/,
                nodeTypeInNewClusterManifest != null && nodeTypeInNewClusterManifest.Certificates != null ? nodeTypeInNewClusterManifest.Certificates.ClientCertificate : null,
                nodeTypeInExistingClusterManifest != null && nodeTypeInExistingClusterManifest.Certificates != null ? nodeTypeInExistingClusterManifest.Certificates.ClientCertificate : null,
                modifiedSettings);

            CompareEndpoints(
                FabricValidatorConstants.SectionNames.FabricNode,
                nodeTypeInNewClusterManifest != null ? nodeTypeInNewClusterManifest.Endpoints : null,
                nodeTypeInExistingClusterManifest != null ? nodeTypeInExistingClusterManifest.Endpoints : null,
                modifiedSettings);
        }

        private void CompareProperties(
            string sectionName,
            KeyValuePairType[] newProperties,
            KeyValuePairType[] existingProperties,
            List<KeyValuePair<string, string>> modifiedSettings)
        {
            if(newProperties == null && existingProperties == null) { return; }

            if(newProperties == null)
            {
                existingProperties.ForEach(existingProperty => modifiedSettings.Add(new KeyValuePair<string,string>(sectionName, existingProperty.Name)));
            }
            else if(existingProperties == null)
            {
                newProperties.ForEach(newProperty => modifiedSettings.Add(new KeyValuePair<string,string>(sectionName, newProperty.Name)));
            }
            else
            {
                // Compute newly added or modified properties
                newProperties.ForEach(
                    newProperty =>
                    {
                        var matchingOldProperty = existingProperties.FirstOrDefault(oldProperty => ImageBuilderUtility.Equals(oldProperty.Name, newProperty.Name));
                        if(matchingOldProperty == null || ImageBuilderUtility.NotEquals(newProperty.Value, matchingOldProperty.Value))
                        {
                            modifiedSettings.Add(new KeyValuePair<string,string>(sectionName, newProperty.Name));
                        }
                    });

                // Compute removed properties
                existingProperties.ForEach(
                    existingProperty =>
                    {
                        var matchingNewProperty = newProperties.FirstOrDefault(newProperty => ImageBuilderUtility.Equals(existingProperty.Name, newProperty.Name));
                        if(matchingNewProperty == null)
                        {
                            modifiedSettings.Add(new KeyValuePair<string,string>(sectionName, existingProperty.Name));
                        }
                    });
            }
        }

        private void CompareCertificates(
            string sectionName,
            string parameterNamePrefix,
            FabricCertificateType newCertificate,
            FabricCertificateType existingCertificate,
            List<KeyValuePair<string, string>> modifiedSettings)
        {
            if (newCertificate == null && existingCertificate == null) { return; }

            CompareParameterValue(
                sectionName,
                parameterNamePrefix + "X509FindType",
                newCertificate != null ? newCertificate.X509FindType.ToString() : null,
                existingCertificate != null ? existingCertificate.X509FindType.ToString() : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                parameterNamePrefix + "X509FindValue",
                newCertificate != null ? newCertificate.X509FindValue : null,
                existingCertificate != null ? existingCertificate.X509FindValue : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                parameterNamePrefix + "X509StoreName",
                newCertificate != null ? newCertificate.X509StoreName : null,
                existingCertificate != null ? existingCertificate.X509StoreName : null,
                modifiedSettings);
        }

        private void CompareEndpoints(
            string sectionName,
            FabricEndpointsType newEndpoints,
            FabricEndpointsType existingEndpoints,
            List<KeyValuePair<string, string>> modifiedSettings)
        {
            if (newEndpoints == null && existingEndpoints == null) { return; }

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.ClientConnectionAddress,
                (newEndpoints != null && newEndpoints.ClientConnectionEndpoint != null) ? newEndpoints.ClientConnectionEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.ClientConnectionEndpoint != null) ? existingEndpoints.ClientConnectionEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.LeaseAgentAddress,
                (newEndpoints != null && newEndpoints.LeaseDriverEndpoint != null) ? newEndpoints.LeaseDriverEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.LeaseDriverEndpoint != null) ? existingEndpoints.LeaseDriverEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.NodeAddress,
                (newEndpoints != null && newEndpoints.ClusterConnectionEndpoint != null) ? newEndpoints.ClusterConnectionEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.ClusterConnectionEndpoint != null) ? existingEndpoints.ClusterConnectionEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.HttpGatewayListenAddress,
                (newEndpoints != null && newEndpoints.HttpGatewayEndpoint != null) ? newEndpoints.HttpGatewayEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.HttpGatewayEndpoint != null) ? existingEndpoints.HttpGatewayEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.HttpApplicationGatewayListenAddress,
                (newEndpoints != null && newEndpoints.HttpApplicationGatewayEndpoint != null) ? newEndpoints.HttpApplicationGatewayEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.HttpApplicationGatewayEndpoint != null) ? existingEndpoints.HttpApplicationGatewayEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.RuntimeServiceAddress,
                (newEndpoints != null && newEndpoints.ServiceConnectionEndpoint != null) ? newEndpoints.ServiceConnectionEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.ServiceConnectionEndpoint != null) ? existingEndpoints.ServiceConnectionEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.ClusterManagerReplicatorAddress,
                (newEndpoints != null && newEndpoints.ClusterManagerReplicatorEndpoint != null) ? newEndpoints.ClusterManagerReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.ClusterManagerReplicatorEndpoint != null) ? existingEndpoints.ClusterManagerReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.RepairManagerReplicatorAddress,
                (newEndpoints != null && newEndpoints.RepairManagerReplicatorEndpoint != null) ? newEndpoints.RepairManagerReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.RepairManagerReplicatorEndpoint != null) ? existingEndpoints.RepairManagerReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.NamingReplicatorAddress,
                (newEndpoints != null && newEndpoints.NamingReplicatorEndpoint != null) ? newEndpoints.NamingReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.NamingReplicatorEndpoint != null) ? existingEndpoints.NamingReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.FailoverManagerReplicatorAddress,
                (newEndpoints != null && newEndpoints.FailoverManagerReplicatorEndpoint != null) ? newEndpoints.FailoverManagerReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.FailoverManagerReplicatorEndpoint != null) ? existingEndpoints.FailoverManagerReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.ImageStoreServiceReplicatorAddress,
                (newEndpoints != null && newEndpoints.ImageStoreServiceReplicatorEndpoint != null) ? newEndpoints.ImageStoreServiceReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.ImageStoreServiceReplicatorEndpoint != null) ? existingEndpoints.ImageStoreServiceReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.UpgradeServiceReplicatorAddress,
                (newEndpoints != null && newEndpoints.UpgradeServiceReplicatorEndpoint != null) ? newEndpoints.UpgradeServiceReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.UpgradeServiceReplicatorEndpoint != null) ? existingEndpoints.UpgradeServiceReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.FaultAnalysisServiceReplicatorAddress,
                (newEndpoints != null && newEndpoints.FaultAnalysisServiceReplicatorEndpoint != null) ? newEndpoints.FaultAnalysisServiceReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.FaultAnalysisServiceReplicatorEndpoint != null) ? existingEndpoints.FaultAnalysisServiceReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.CentralSecretServiceReplicatorAddress,
                (newEndpoints != null && newEndpoints.CentralSecretServiceReplicatorEndpoint != null) ? newEndpoints.CentralSecretServiceReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.CentralSecretServiceReplicatorEndpoint != null) ? existingEndpoints.CentralSecretServiceReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.BackupRestoreServiceReplicatorAddress,
                (newEndpoints != null && newEndpoints.BackupRestoreServiceReplicatorEndpoint != null) ? newEndpoints.BackupRestoreServiceReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.BackupRestoreServiceReplicatorEndpoint != null) ? existingEndpoints.BackupRestoreServiceReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.GatewayResourceManagerReplicatorAddress,
                (newEndpoints != null && newEndpoints.GatewayResourceManagerReplicatorEndpoint != null) ? newEndpoints.GatewayResourceManagerReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.GatewayResourceManagerReplicatorEndpoint != null) ? existingEndpoints.GatewayResourceManagerReplicatorEndpoint.Port : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.StartApplicationPortRange,
                (newEndpoints != null && newEndpoints.ApplicationEndpoints != null) ? newEndpoints.ApplicationEndpoints.StartPort.ToString() : null,
                (existingEndpoints != null && existingEndpoints.ApplicationEndpoints != null) ? existingEndpoints.ApplicationEndpoints.StartPort.ToString() : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.EndApplicationPortRange,
                (newEndpoints != null && newEndpoints.ApplicationEndpoints != null) ? newEndpoints.ApplicationEndpoints.EndPort.ToString() : null,
                (existingEndpoints != null && existingEndpoints.ApplicationEndpoints != null) ? existingEndpoints.ApplicationEndpoints.EndPort.ToString() : null,
                modifiedSettings);

            CompareParameterValue(
                sectionName,
                FabricValidatorConstants.ParameterNames.EventStoreServiceReplicatorAddress,
                (newEndpoints != null && newEndpoints.EventStoreServiceReplicatorEndpoint != null) ? newEndpoints.EventStoreServiceReplicatorEndpoint.Port : null,
                (existingEndpoints != null && existingEndpoints.EventStoreServiceReplicatorEndpoint != null) ? existingEndpoints.EventStoreServiceReplicatorEndpoint.Port : null,
                modifiedSettings);
        }

        private void CompareParameterValue(
            string sectionName,
            string parameterName,
            string newParameterValue,
            string existingParameterValue,
            List<KeyValuePair<string, string>> modifiedSettings)
        {
            if (newParameterValue == null && existingParameterValue == null) { return; }

            if(newParameterValue == null || existingParameterValue == null || newParameterValue != existingParameterValue)
            {
                modifiedSettings.Add(new KeyValuePair<string, string>(sectionName, parameterName));
            }
        }

        private bool ValidateAutomateUpdateEnabledSetting()
        {
            bool validateResult = true;
            var fabricHostSetting = this.clusterManifest.FabricSettings.FirstOrDefault(setting => setting.Name == FabricValidatorConstants.SectionNames.FabricHost);
            if (fabricHostSetting != null)
            {
                var autoUpdateEnabledParam =
                    fabricHostSetting.Parameter.FirstOrDefault(
                        param => param.Name.Equals(FabricValidatorConstants.ParameterNames.EnableServiceFabricAutomaticUpdates, StringComparison.OrdinalIgnoreCase) &&
                                 param.Value.Equals("true", StringComparison.OrdinalIgnoreCase));

                var autoBaseUpgradeEnabledParam =
                    fabricHostSetting.Parameter.FirstOrDefault(
                        param => param.Name.Equals(FabricValidatorConstants.ParameterNames.EnableServiceFabricBaseUpgrade, StringComparison.OrdinalIgnoreCase) &&
                                 param.Value.Equals("true", StringComparison.OrdinalIgnoreCase));

                var restartManagementEnabledParam =
                    fabricHostSetting.Parameter.FirstOrDefault(
                        param => param.Name.Equals(FabricValidatorConstants.ParameterNames.EnableRestartManagement, StringComparison.OrdinalIgnoreCase)  &&
                                 param.Value.Equals("true", StringComparison.OrdinalIgnoreCase));

                if (autoUpdateEnabledParam != null || autoBaseUpgradeEnabledParam != null || restartManagementEnabledParam != null)
                {
#if DotNetCoreClrLinux
                    bool infraForServer = this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux;
#else
                    bool infraForServer = this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer;
#endif
                    if (!infraForServer)
                    {
                        if (autoUpdateEnabledParam != null)
                        {
                            WriteError(
                               "Setting {0} in FabricHost is supported only for window server",
                               FabricValidatorConstants.ParameterNames.EnableServiceFabricAutomaticUpdates);
                        }

                        if (autoBaseUpgradeEnabledParam != null)
                        {
                            WriteError(
                               "Setting {0} in FabricHost is supported only for window server",
                               FabricValidatorConstants.ParameterNames.EnableServiceFabricBaseUpgrade);
                        }

                        if (restartManagementEnabledParam != null)
                        {
                            WriteError(
                               "Setting {0} in FabricHost is supported only for window server",
                               FabricValidatorConstants.ParameterNames.EnableRestartManagement);
                        }

                        validateResult = false;
                    }
                }

                if (autoUpdateEnabledParam != null || autoBaseUpgradeEnabledParam != null)
                {
                    if (this.clusterManifest.FabricSettings.Any(setting =>
                        setting.Name.Equals(FabricValidatorConstants.SectionNames.UpgradeService, StringComparison.OrdinalIgnoreCase)))
                    {
                        WriteError(
                            "Service {0} should not be enabled when {1} or {2} is enabled in FabricHost section",
                            FabricValidatorConstants.SectionNames.UpgradeService,
                            FabricValidatorConstants.ParameterNames.EnableServiceFabricAutomaticUpdates,
                            FabricValidatorConstants.ParameterNames.EnableServiceFabricBaseUpgrade);
                        validateResult = false;
                    }
                }

                if (restartManagementEnabledParam != null)
                {
                    if (this.clusterManifest.FabricSettings.Any(setting =>
                        setting.Name.Equals(FabricValidatorConstants.SectionNames.RepairManager, StringComparison.OrdinalIgnoreCase)))
                    {
                        WriteError(
                            "Service {0} should not be enabled when {1} is enabled in FabricHost section",
                            FabricValidatorConstants.SectionNames.RepairManager,
                            FabricValidatorConstants.ParameterNames.EnableRestartManagement);
                        validateResult = false;
                    }

                    if (this.clusterManifest.FabricSettings.Any(setting =>
                        setting.Name.Equals(FabricValidatorConstants.SectionNames.InfrastructureService, StringComparison.OrdinalIgnoreCase)))
                    {
                        WriteError(
                            "Service {0} should not be enabled when {1} is enabled in FabricHost section",
                            FabricValidatorConstants.SectionNames.InfrastructureService,
                            FabricValidatorConstants.ParameterNames.EnableRestartManagement);
                        validateResult = false;
                    }
                }
            }

            return validateResult;
        }

        private bool ValidateRunAsEnabledIfInfrastructureServiceEnabled()
        {
            // Ensure RunAsPolicy is enabled when enabling infrastructure service
            if (this.IsRunAsPolicyEnabled)
            {
                return true;
            }

            if (this.windowsFabricSettings.SettingsMap.ContainsKey(FabricValidatorConstants.SectionNames.InfrastructureService)
            || this.windowsFabricSettings.SettingsMap.Any(item => item.Key.StartsWith(FabricValidatorConstants.SectionNames.InfrastructureService + "/", StringComparison.OrdinalIgnoreCase)))
            {
                WriteError("RunAsPolicyEnabled should be set to true when enabling InfrastructureService");
                return false;
            }

            return true;
        }

        private bool ValidateImageStoreServiceSettings(bool checkStandby = false)
        {
            Dictionary<string, WindowsFabricSettings.SettingsValue> imageStoreServiceSettingsSection = null;
            if (!this.windowsFabricSettings.SettingsMap.TryGetValue(FabricValidatorConstants.SectionNames.ImageStoreService, out imageStoreServiceSettingsSection))
            {
                imageStoreServiceSettingsSection = new Dictionary<string, WindowsFabricSettings.SettingsValue>();
            }

            if (checkStandby)
            {
                bool isEnabled = imageStoreServiceSettingsSection[FabricValidatorConstants.ParameterNames.Enabled].GetValue<bool>();
                if (!isEnabled) { return true; }
            }

            bool serviceAffinitizedToCM =
                imageStoreServiceSettingsSection[FabricValidatorConstants.ParameterNames.EnableClusterManagerAffinity]
                    .GetValue<bool>();

            if (!serviceAffinitizedToCM) { return true; }

            int imageStoreServiceTargetReplicaSetSize =
                imageStoreServiceSettingsSection[FabricValidatorConstants.ParameterNames.TargetReplicaSetSize]
                    .GetValue<int>();

            Dictionary<string, WindowsFabricSettings.SettingsValue> clusterManagerSettingsSection = null;
            if (!this.windowsFabricSettings.SettingsMap.TryGetValue(FabricValidatorConstants.SectionNames.ClusterManager, out clusterManagerSettingsSection))
            {
                clusterManagerSettingsSection = new Dictionary<string, WindowsFabricSettings.SettingsValue>();
            }

            int cmTargetReplicaSetSize = clusterManagerSettingsSection[FabricValidatorConstants.ParameterNames.TargetReplicaSetSize].GetValue<int>();

            if (imageStoreServiceTargetReplicaSetSize > cmTargetReplicaSetSize)
            {
                WriteError(
                    "The TargetReplicaSetSize of ImageStoreService should be less than or equal to the TargetReplicaSetSize of ClusterManager. ClusterManager={0}, ImageStoreService={1}",
                    cmTargetReplicaSetSize,
                    imageStoreServiceTargetReplicaSetSize);
                return false;
            }

            return true;
        }

        private bool HasParameterValueChanged(WindowsFabricSettings.SettingsValue oldParameter, WindowsFabricSettings.SettingsValue newParameter, string newX509StoreName)
        {
            char[] oldParameterSecureCharArray = null;
            char[] newParameterSecureCharArray = null;
            try
            {
                oldParameterSecureCharArray =
                    oldParameter.IsEncrypted ? FabricValidatorUtility.SecureStringToCharArray(oldParameter.GetSecureValue(this.storeName)) : oldParameter.Value.ToCharArray();
                newParameterSecureCharArray =
                    newParameter.IsEncrypted ? FabricValidatorUtility.SecureStringToCharArray(newParameter.GetSecureValue(newX509StoreName)) : newParameter.Value.ToCharArray();

                return !FabricValidatorUtility.Equals(oldParameterSecureCharArray, newParameterSecureCharArray);
            }
            finally
            {
                if (oldParameterSecureCharArray != null) { Array.Clear(oldParameterSecureCharArray, 0, oldParameterSecureCharArray.Length); }
                if (newParameterSecureCharArray != null) { Array.Clear(newParameterSecureCharArray, 0, newParameterSecureCharArray.Length); }
            }
        }

        private void RunOtherConfigurationValidatorsInTheAssembly()
        {
            Assembly currentAssembly = typeof(FabricSettingsValidator).GetTypeInfo().Assembly;

            IEnumerable<Type> allTypes;
            try
            {
#if !DotNetCoreClr
                allTypes = currentAssembly.DefinedTypes;
#else
                allTypes = currentAssembly.DefinedTypes as IEnumerable<Type>;
#endif
            }
            catch (ReflectionTypeLoadException ex)
            {
                foreach (Exception le in ex.LoaderExceptions)
                {
                    WriteError("Loader exception: {0}", le);
                }
                throw;
            }

            foreach (Type t in allTypes)
            {
                if (t.GetTypeInfo().IsSubclassOf(typeof(BaseFabricConfigurationValidator)))
                {
                    BaseFabricConfigurationValidator validator = Activator.CreateInstance(t) as BaseFabricConfigurationValidator;
                    if (validator == null)
                    {
                        continue;
                    }
                    if (this.windowsFabricSettings.SettingsMap.ContainsKey(validator.SectionName))
                    {
                        validator.ValidateConfiguration(this.windowsFabricSettings);
                    }
                }
            }
        }

        private void RunOtherConfigurationUpgradeValidatorsInTheAssembly(WindowsFabricSettings targetWindowsFabricSettings)
        {
            Assembly currentAssembly = typeof(FabricSettingsValidator).GetTypeInfo().Assembly;
#if !DotNetCoreClr
            IEnumerable<Type> allTypes = currentAssembly.DefinedTypes;
#else
            IEnumerable<Type> allTypes = currentAssembly.DefinedTypes as IEnumerable<Type>;
#endif
            foreach (Type t in allTypes)
            {

                if (t.GetTypeInfo().IsSubclassOf(typeof(BaseFabricConfigurationValidator)))
                {
                    BaseFabricConfigurationValidator validator = Activator.CreateInstance(t) as BaseFabricConfigurationValidator;
                    if (validator == null)
                    {
                        continue;
                    }
                    if (targetWindowsFabricSettings.SettingsMap.ContainsKey(validator.SectionName))
                    {
                        validator.ValidateConfigurationUpgrade(this.windowsFabricSettings, targetWindowsFabricSettings);
                    }
                }
            }
        }

        private void VerifyNodeTypes()
        {
            HashSet<string> nodeTypeDuplicateValidator = new HashSet<string>();
            foreach (var nodeType in this.nodeTypes)
            {
                if (nodeTypeDuplicateValidator.Contains(nodeType.Name))
                {
                    WriteError("Duplicate node type {0} found.", nodeType.Name);
                }
                else
                {
                    nodeTypeDuplicateValidator.Add(nodeType.Name);
                }

                CheckForNoDuplicatesInNodeTypeInfo(nodeType.PlacementProperties, FabricValidatorConstants.SectionNames.NodeProperties);
                CheckForNoDuplicatesInNodeTypeInfo(nodeType.Capacities, FabricValidatorConstants.SectionNames.NodeCapacities);
                CheckForUnsignedIntegerValuesInNodeTypeInfo(nodeType.Capacities, FabricValidatorConstants.SectionNames.NodeCapacities);

                string[] systemDefinedPlacementProperties =
                {
                    FabricValidatorConstants.NodeType,
                    FabricValidatorConstants.NodeName,
                    FabricValidatorConstants.UpgradeDomain,
                    FabricValidatorConstants.FaultDomain
                };

                CheckForSystemDefinedPlacementPropertyInNodeTypeInfo(
                    nodeType.PlacementProperties,
                    systemDefinedPlacementProperties,
                    FabricValidatorConstants.SectionNames.PlacementProperties);
            }
        }

        private void CheckForNoDuplicatesInNodeTypeInfo(KeyValuePairType[] keyValuePairs, string sectionName)
        {
            if (keyValuePairs == null)
            {
                return;
            }
            HashSet<string> duplicateKeyDetector = new HashSet<string>();
            foreach (KeyValuePairType keyValuePair in keyValuePairs)
            {
                if (duplicateKeyDetector.Contains(keyValuePair.Name))
                {
                    WriteError("Duplicate {0} {1} found.", sectionName, keyValuePair.Name);
                }
                else
                {
                    duplicateKeyDetector.Add(keyValuePair.Name);
                }
            }
        }

        private void CheckForUnsignedIntegerValuesInNodeTypeInfo(KeyValuePairType[] keyValuePairs, string sectionName)
        {
            if (keyValuePairs == null)
            {
                return;
            }
            foreach (KeyValuePairType keyValuePair in keyValuePairs)
            {
                uint value;
                if (!uint.TryParse(keyValuePair.Value, NumberStyles.Integer, CultureInfo.InvariantCulture, out value))
                {
                    WriteError("Invalid value {0} found for {1} under {2}; unsigned integer expected.", keyValuePair.Value, keyValuePair.Name, sectionName);
                }
            }
        }

        private void CheckForSystemDefinedPlacementPropertyInNodeTypeInfo(
            KeyValuePairType[] keyValuePairs,
            string[] systemDefinedPlacementProperty,
            string sectionName)
        {
            if (keyValuePairs == null)
            {
                return;
            }
            foreach (KeyValuePairType keyValuePair in keyValuePairs)
            {
                if (systemDefinedPlacementProperty.Contains(keyValuePair.Name))
                {
                    WriteError("Invalid property name {0} found under {1}; it is duplicating one of the system defined placement property.",
                        keyValuePair.Name, sectionName);
                }
            }
        }

        private void VerifySecuritySettings()
        {
            var securitySection = this.windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.Security];

            string clusterCredentialTypeString = FabricValidatorConstants.CredentialTypeNone;
            if (securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClusterCredentialType))
            {
                clusterCredentialTypeString = securitySection[FabricValidatorConstants.ParameterNames.ClusterCredentialType].Value;
            }

            CredentialType clusterCredentialType;
            if (!TryParseCredentialType(clusterCredentialTypeString, out clusterCredentialType))
            {
                WriteError("Invalid ClusterCredentialType: {0}", clusterCredentialTypeString);
            }

            string serverCredentialTypeString = FabricValidatorConstants.CredentialTypeNone;
            if (securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ServerAuthCredentialType))
            {
                serverCredentialTypeString = securitySection[FabricValidatorConstants.ParameterNames.ServerAuthCredentialType].Value;
            }

            CredentialType serverCredentialType;
            if (!TryParseCredentialType(serverCredentialTypeString, out serverCredentialType))
            {
                WriteError("Invalid ServerAuthCredentialType: {0}", serverCredentialTypeString);
            }

            VerifyClientRoleSettings(serverCredentialType);

            if ((clusterCredentialType == CredentialType.Windows) || (serverCredentialType == CredentialType.Windows))
            {
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
                if (clusterCredentialType == CredentialType.Windows)
                {
                    Version osVersionMin = new Version(6, 2);
                    if (Environment.OSVersion.Version < osVersionMin)
                    {
                        WriteError("Credential type '{0}' is not supported on {1}, {2} or above is required, because kernel mode encryption with Kerberos was added to OS at version {2}", clusterCredentialType, Environment.OSVersion, osVersionMin);
                    }
                }
#endif

                if (!IsKerberosEnabledForFabricAccountType())
                {
                    WriteError("Fabric account type '{0}' is not compatible with credential type '{1}'", this.fabricAccountType, clusterCredentialType);
                }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting fabricvalidator for windows.
                VerifyClusterSpn(securitySection);
#endif
                if (infrastructureInformation != null)
                {
                    foreach (var node in infrastructureInformation)
                    {
                        string address = node.IPAddressOrFQDN;
                        if ((address.Length > 2) && (address[0] == '[') && (address[address.Length - 1] == ']'))
                        {
                            address = address.Substring(1, address.Length - 2);
                        }

                        IPAddress ipAddress;
                        if (IPAddress.TryParse(address, out ipAddress))
                        {
                            if (this.FabricRunAsMachineAccount())
                            {
                                WriteError(
                                    "IP address is not allowed for credential type '{0}' when fabric runs as {1}, please use hostnames.",
                                    FabricValidatorConstants.CredentialTypeWindows,
                                    this.fabricAccountType);
                            }
                        }
                    }
                }
            }

            if (securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClientClaimAuthEnabled))
            {
                string claimsAuthEnabledValue = securitySection[FabricValidatorConstants.ParameterNames.ClientClaimAuthEnabled].Value;
                bool claimsAuthEnabled;
                if (!bool.TryParse(claimsAuthEnabledValue, out claimsAuthEnabled))
                {
                    WriteError("ClaimsAuthEnabled can only be true/false: Value found - {0}", claimsAuthEnabledValue);
                }

                if (claimsAuthEnabled)
                {
                    if (serverCredentialType != CredentialType.X509)
                    {
                        WriteError("ServerCredentialType should be set to X509 if ClaimsAuthEnabled is set to true");
                    }

                    //
                    // If ClaimsAuthEnabled is set to true, then both these fields are mandatory. Their values can
                    // be empty depending on the user's policy. For example, if admin roles need to be enforced just
                    // using certificates, then AdminClientClaims can be empty.
                    //
                    if (!securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClientClaims) ||
                        !securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.AdminClientClaims))
                    {
                        WriteError("ClaimsAuth is enabled but ClientClaims or AdminClientClaims is not set");
                    }
                    var tokenValidationSections = this.GetAllTVSSettings();
                    if(tokenValidationSections.Count == 0)
                    {
                        WriteError("ClaimsAuth is enabled but no DSTSTokenValidationService section is specified");
                    }
                }
            }

            // validate disabled firewall rules
            VerifyDisabledFirewallProfileSettings();
        }

        private void VerifyDisabledFirewallProfileSettings()
        {
            var securitySection = this.windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.Security];

            if (securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.DisableFirewallRuleForDomainProfile) &&
                securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.DisableFirewallRuleForPrivateProfile) &&
                securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.DisableFirewallRuleForPublicProfile))
            {
                if (securitySection[FabricValidatorConstants.ParameterNames.DisableFirewallRuleForDomainProfile].Value.Equals(bool.TrueString, StringComparison.OrdinalIgnoreCase) &&
                    securitySection[FabricValidatorConstants.ParameterNames.DisableFirewallRuleForPrivateProfile].Value.Equals(bool.TrueString, StringComparison.OrdinalIgnoreCase) &&
                    securitySection[FabricValidatorConstants.ParameterNames.DisableFirewallRuleForPublicProfile].Value.Equals(bool.TrueString, StringComparison.OrdinalIgnoreCase))
                {
                    WriteWarning("Current profile will be disabled by default for firewall rule");
                }
            }
        }

        private void ValidateFabricHostSettings()
        {
            if (!this.windowsFabricSettings.SettingsMap.ContainsKey(FabricValidatorConstants.SectionNames.FabricHost))
            {
                return;
            }
            var section = this.windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.FabricHost];
            if (section.ContainsKey(FabricValidatorConstants.ParameterNames.ActivatorServiceAddress))
            {
                string address = section[FabricValidatorConstants.ParameterNames.ActivatorServiceAddress].Value;
                if (address != null)
                {
                    string[] parts = address.Split(':');
                    Int32 port;

                    if (parts.Length != 2 ||
                        !parts[0].Equals(FabricValidatorConstants.LocalHostAddressPrefix, StringComparison.OrdinalIgnoreCase) ||
                        !Int32.TryParse(parts[1], out port))
                    {
                        WriteError(String.Format("Specified ActivatorServiceAddress is invalid {0}", address));
                    }

                }
            }
        }

        private bool IsKerberosEnabledForFabricAccountType()
        {
            return (this.fabricAccountType == SecurityPrincipalsTypeUserAccountType.DomainUser) ||
                (this.fabricAccountType == SecurityPrincipalsTypeUserAccountType.LocalSystem) ||
                (this.fabricAccountType == SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount) ||
                (this.fabricAccountType == SecurityPrincipalsTypeUserAccountType.NetworkService);
        }

        private bool FabricRunAsMachineAccount()
        {
            return (this.fabricAccountType == SecurityPrincipalsTypeUserAccountType.LocalSystem) ||
                (this.fabricAccountType == SecurityPrincipalsTypeUserAccountType.NetworkService);
        }

        private void VerifyExpectedClusterSize()
        {
            int intExpectedClusterSize = 0;
            try
            {
                intExpectedClusterSize = Convert.ToInt32(this.windowsFabricSettings.GetParameter(FabricValidatorConstants.SectionNames.FailoverManager, FabricValidatorConstants.ParameterNames.ExpectedClusterSize).Value);
            }
            catch (FormatException e)
            {
                WriteError(String.Format(StringResources.Warning_ExceptedClusterSizeIsNotDigits, e));
            }
            if (intExpectedClusterSize < 0)
            {
                WriteError(StringResources.Warning_ExpectedClusterSizeMustBeGreaterThanZero);
            }
            int nodeCount = 0;
#if DotNetCoreClrLinux
            if (this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureLinux)
            {
                var serverInfra = this.clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureLinux;
#else
            if (this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
            {
                var serverInfra = this.clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
#endif
                foreach (FabricNodeType node in serverInfra.NodeList)
                {
                    nodeCount++;
                }
            }
            else if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                ClusterManifestTypeInfrastructureWindowsAzureStaticTopology windowsAzureStaticTopologyServerInfra = this.clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzureStaticTopology;
                foreach (FabricNodeType node in windowsAzureStaticTopologyServerInfra.NodeList)
                {
                    nodeCount++;
                }
            }
            if (this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
            {
                ClusterManifestTypeInfrastructurePaaS paasInfra = this.clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructurePaaS;
                foreach (var role in paasInfra.Roles)
                {
                    nodeCount += role.RoleNodeCount;
                }
            }
            else if (this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure && this.infrastructureInformation != null)
            {
                foreach (InfrastructureNodeType infra in this.infrastructureInformation)
                {
                    nodeCount++;
                }
            }
            else if (this.clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure && this.infrastructureInformation == null)
            {
                return;
            }

            if (intExpectedClusterSize > nodeCount)
            {
                WriteError(String.Format(StringResources.Warning_ExpectedClusterSizeSmallerThanNodeCount, intExpectedClusterSize, nodeCount));
            }
        }

        private void VerifyRunAsSettings(string runAsSectionName)
        {
            if (!this.windowsFabricSettings.GetParameter(runAsSectionName, FabricValidatorConstants.ParameterNames.RunAsAccountType).IsFromClusterManifest)
            {
                return;
            }
            var runAsSection = this.windowsFabricSettings.GetSection(runAsSectionName);

            if (!runAsSection[FabricValidatorConstants.ParameterNames.RunAsAccountType].IsFromClusterManifest)
            {
                WriteError("Invalid RunAs section: parameter account type is missing");
            }

            string accountTypeString = runAsSection[FabricValidatorConstants.ParameterNames.RunAsAccountType].Value;
            if (!TryParseRunAsAccountType(accountTypeString, out fabricAccountType))
            {
                WriteError("Invalid RunAs section: parameter account type {0} is invalid", accountTypeString);
            }

            if (fabricAccountType == SecurityPrincipalsTypeUserAccountType.DomainUser)
            {
                if (!runAsSection[FabricValidatorConstants.ParameterNames.RunAsPassword].IsFromClusterManifest)
                {
                    WriteError("Invalid RunAs section: parameter password is missing for domain user");
                }
            }
            else
            {
                if (runAsSection[FabricValidatorConstants.ParameterNames.RunAsPassword].IsFromClusterManifest)
                {
                    WriteError("Invalid RunAs section: parameter password is present for non domain user");
                }
            }

            if (fabricAccountType == SecurityPrincipalsTypeUserAccountType.DomainUser || fabricAccountType == SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount)
            {
                if (!runAsSection[FabricValidatorConstants.ParameterNames.RunAsAccountName].IsFromClusterManifest)
                {
                    WriteError("Invalid RunAs section: parameter account name is missing");
                }

                string accountNameString = runAsSection[FabricValidatorConstants.ParameterNames.RunAsAccountName].Value;

                if (!ImageBuilderUtility.IsValidAccountName(accountNameString))
                {
                    WriteError("Invalid RunAs section: parameter account name {0} is invalid", accountNameString);
                }
            }
        }

        private void VerifyClusterHealthPolicy()
        {
            var policySection = this.windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy];

            foreach (var entry in policySection)
            {
                if (entry.Key == FabricValidatorConstants.ParameterNames.MaxPercentUnhealthyNodes ||
                    entry.Key == FabricValidatorConstants.ParameterNames.MaxPercentUnhealthyApplications)
                {
                    ValidatePercentage(entry.Key, entry.Value.Value);
                }
                else if (entry.Key.StartsWith(FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix))
                {
                    // Entries in the application type map
                    if (entry.Key.Length <= FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix.Length)
                    {
                        WriteError("Invalid application type name for entry {0}. Expected pattern: {1}<ApplicationTypeName>", entry.Key, FabricValidatorConstants.ParameterNames.ApplicationTypeMaxPercentUnhealthyApplicationsPrefix);
                    }

                    ValidatePercentage(entry.Key, entry.Value.Value);
                }
                else
                {
                    try
                    {
                        WindowsFabricSettings.ValidateConfigurationTypes(FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy, entry.Key, entry.Value.Type, entry.Value.Value);
                    }
                    catch (Exception)
                    {
                        WriteError("Section {0}, entry {1}: value {2} is invalid. Error: {3}", FabricValidatorConstants.SectionNames.HealthManager_ClusterHealthPolicy, entry.Key, entry.Value.Value);
                    }
                }
            }
        }

        private void ValidatePercentage(string key, string value)
        {
            int percent;
            if (!int.TryParse(value, out percent) || percent < 0 || percent > 100)
            {
                WriteError("{0} must be in the range [0, 100]: {1}", key, value);
            }
        }

        private void VerifyClientRoleSettings(CredentialType serverCredentialType)
        {
            if (serverCredentialType == CredentialType.None)
            {
                return;
            }

            var securitySection = this.windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.Security];
            if (!securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClientRoleEnabled))
            {
                return;
            }

            string clientRoleEnabledValue = securitySection[FabricValidatorConstants.ParameterNames.ClientRoleEnabled].Value;
            bool clientRoleEnabled;
            if (!bool.TryParse(clientRoleEnabledValue, out clientRoleEnabled))
            {
                WriteError("ClientRoleEnabled can only be true/false: Value found - {0}", clientRoleEnabledValue);
            }

            if (serverCredentialType == CredentialType.X509)
            {
                if (clientRoleEnabled)
                {
                    bool zeroAdminClientCommonNames =
                        !securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.AdminClientCommonNames) ||
                        securitySection[FabricValidatorConstants.ParameterNames.AdminClientCommonNames].Value.Trim() == string.Empty;
                    bool zeroAdminClientCertThumbprints =
                        !securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.AdminClientCertThumbprints) ||
                        securitySection[FabricValidatorConstants.ParameterNames.AdminClientCertThumbprints].Value.Trim() == string.Empty;
                    bool zeroAdminClientX509Names = true;
                    if (windowsFabricSettings.SettingsMap.ContainsKey(FabricValidatorConstants.SectionNames.AdminClientX509Names))
                    {
                        zeroAdminClientX509Names = windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.AdminClientX509Names].Count == 0;
                    }

                    if (zeroAdminClientCommonNames && zeroAdminClientCertThumbprints && zeroAdminClientX509Names)
                    {
                        WriteWarning("Default client certificate is not in admin role, future releases will fail on this, you must add it to {0} or {1}", FabricValidatorConstants.ParameterNames.AdminClientCertThumbprints, FabricValidatorConstants.SectionNames.AdminClientX509Names);
                    }
                }
                else
                {
                    bool zeroClientAuthAllowedCommonNames =
                        !securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClientAuthAllowedCommonNames) ||
                        securitySection[FabricValidatorConstants.ParameterNames.ClientAuthAllowedCommonNames].Value.Trim() == string.Empty;
                    bool zeroClientCertThumbprints =
                        !securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClientCertThumbprints) ||
                        securitySection[FabricValidatorConstants.ParameterNames.ClientCertThumbprints].Value.Trim() == string.Empty;
                    bool zeroClientX509Names = true;
                    if (windowsFabricSettings.SettingsMap.ContainsKey(FabricValidatorConstants.SectionNames.ClientX509Names))
                    {
                        zeroClientX509Names = windowsFabricSettings.SettingsMap[FabricValidatorConstants.SectionNames.ClientX509Names].Count == 0;
                    }

                    if (zeroClientAuthAllowedCommonNames && zeroClientCertThumbprints && zeroClientX509Names)
                    {
                        WriteWarning("Default client certificate is not in allow list, future releases will fail on this, you must add it to {0} or {1}", FabricValidatorConstants.ParameterNames.ClientCertThumbprints, FabricValidatorConstants.SectionNames.ClientX509Names);
                    }
                }

                return;
            }

            if (serverCredentialType == CredentialType.Windows)
            {
                if (clientRoleEnabled)
                {
                    if (!securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.AdminClientIdentities) ||
                        (securitySection[FabricValidatorConstants.ParameterNames.AdminClientIdentities].Value.Trim() == string.Empty))
                    {
                        WriteWarning("AdminClientIdentities is empty, thus only fabric gateway account is allowed to do admin operations");
                    }
                }
            }
        }

#if !DotNetCoreClr // Disable compiling on windows for now. Need to correct when porting fabricvalidator for windows.
        private void VerifyClusterSpn(Dictionary<string, WindowsFabricSettings.SettingsValue> securitySection)
        {
            string clusterSpn = string.Empty;
            if (securitySection.ContainsKey(FabricValidatorConstants.ParameterNames.ClusterSpn))
            {
                clusterSpn = securitySection[FabricValidatorConstants.ParameterNames.ClusterSpn].Value;
            }

            clusterSpn = clusterSpn.Trim();
            if (clusterSpn == string.Empty)
            {
                if (!this.FabricRunAsMachineAccount())
                {
                    WriteError("ClusterSpn cannot be empty when fabric runs as {0}", this.fabricAccountType);
                }

                WriteInfo("ClusterSpn is left empty for running fabric as {0}", this.fabricAccountType);
                this.ShouldRegisterSpnForMachineAccount = true;
                return;
            }

            WriteInfo("ClusterSpn is set to {0} for running fabric as {1}", clusterSpn, this.fabricAccountType);

            using (Domain thisDomain = Domain.GetCurrentDomain())
            {
                WriteInfo("Searching ClusterSpn {0} in domain {1} ...", clusterSpn, thisDomain.Name);

                using (DirectorySearcher search = new DirectorySearcher(thisDomain.GetDirectoryEntry()))
                {
                    search.Filter = string.Format(CultureInfo.InvariantCulture, "(ServicePrincipalName={0})", clusterSpn);
                    search.SearchScope = SearchScope.Subtree;

                    try
                    {
                        var matches = search.FindAll();
                        if (matches.Count < 1)
                        {
                            WriteError("ClusterSpn {0} is not found in domain {1}", clusterSpn, thisDomain.Name);
                        }

                        if (matches.Count == 1)
                        {
                            WriteInfo(
                                "ClusterSpn {0} is registered to {1} {2} in domain {2}",
                                clusterSpn,
                                matches[0].GetDirectoryEntry().Name,
                                matches[0].GetDirectoryEntry().Guid,
                                thisDomain.Name);

                            return;
                        }

                        // matches.Count > 1
                        WriteError(
                            "ClusterSpn {0} is registered to more than one objects in domain {1}",
                            clusterSpn, thisDomain.Name);

                        foreach (SearchResult match in matches)
                        {
                            WriteError(
                                "ClusterSpn {0} is registered to {1} {2}",
                                clusterSpn,
                                match.GetDirectoryEntry().Name,
                                match.GetDirectoryEntry().Guid);
                        }
                    }
                    catch (Exception e)
                    {
                        WriteError("Failed to search ClusterSpn {0}: {1}", clusterSpn, e);
                    }
                }
            }
        }
#endif
        private bool TryParseCredentialType(string stringValue, out CredentialType credentialType)
        {
            if (string.Compare(stringValue, FabricValidatorConstants.CredentialTypeNone, StringComparison.OrdinalIgnoreCase) == 0)
            {
                credentialType = CredentialType.None;
                return true;
            }

            if (string.Compare(stringValue, FabricValidatorConstants.CredentialTypeX509, StringComparison.OrdinalIgnoreCase) == 0)
            {
                credentialType = CredentialType.X509;
                return true;
            }

            if (string.Compare(stringValue, FabricValidatorConstants.CredentialTypeWindows, StringComparison.OrdinalIgnoreCase) == 0)
            {
                credentialType = CredentialType.Windows;
                return true;
            }

            credentialType = CredentialType.None;
            return false;
        }

        private bool TryParseRunAsAccountType(string stringValue, out SecurityPrincipalsTypeUserAccountType runAsAccountType)
        {
            if (string.Compare(stringValue, FabricValidatorConstants.RunAsAccountTypeDomainUser, StringComparison.OrdinalIgnoreCase) == 0)
            {
                runAsAccountType = SecurityPrincipalsTypeUserAccountType.DomainUser;
                return true;
            }

            if (string.Compare(stringValue, FabricValidatorConstants.RunAsAccountTypeLocalService, StringComparison.OrdinalIgnoreCase) == 0)
            {
                runAsAccountType = SecurityPrincipalsTypeUserAccountType.LocalService;
                return true;
            }

            if (string.Compare(stringValue, FabricValidatorConstants.RunAsAccountTypeManagedService, StringComparison.OrdinalIgnoreCase) == 0)
            {
                runAsAccountType = SecurityPrincipalsTypeUserAccountType.ManagedServiceAccount;
                return true;
            }

            if (string.Compare(stringValue, FabricValidatorConstants.RunAsAccountTypeNetworkService, StringComparison.OrdinalIgnoreCase) == 0)
            {
                runAsAccountType = SecurityPrincipalsTypeUserAccountType.NetworkService;
                return true;
            }

            if (string.Compare(stringValue, FabricValidatorConstants.RunAsAccountTypeLocalSystem, StringComparison.OrdinalIgnoreCase) == 0)
            {
                runAsAccountType = SecurityPrincipalsTypeUserAccountType.LocalSystem;
                return true;
            }

            runAsAccountType = SecurityPrincipalsTypeUserAccountType.LocalUser;
            return false;
        }

        private void VerifyRequiredParameters()
        {
            foreach (var sectionName in FabricSettingsValidator.RequiredParameters.Keys)
            {
                var settingsSection = this.windowsFabricSettings.GetSection(sectionName);
                foreach (var parameterName in FabricSettingsValidator.RequiredParameters[sectionName])
                {
                    if (!settingsSection[parameterName].IsFromClusterManifest)
                    {
                        WriteError("Parameter {0} in section {1} is required.", parameterName, sectionName);
                    }
                }
            }
        }

        private void VerifyNoLeadingTrailingSpaces()
        {
            foreach (var section in this.windowsFabricSettings.SettingsMap)
            {
                foreach (var parameterKey in section.Value)
                {
                    if (parameterKey.Key.StartsWith(" ") || parameterKey.Key.EndsWith(" "))
                    {
                        WriteWarning(StringResources.Warning_ParameterContainsLeadingTrailingSpace, parameterKey.Key, section.Key);
                        DeployerTrace.WriteWarning(StringResources.Warning_ParameterContainsLeadingTrailingSpace, parameterKey.Key, section.Key);
                    }
                }
            }
        }

        private void VerifyVoteConfiguration()
        {
            NonSeedNodeVoteCount = 0;
            foreach (var parameter in this.windowsFabricSettings.GetSection(FabricValidatorConstants.SectionNames.Votes).Values)
            {
                string[] voteInfo = parameter.Value.Split(',');
                if (voteInfo[0] != FabricValidatorConstants.SQLVoteType)
                {
                    WriteError("Invalid vote type {0}. Only SQL vote type is allowed here.", voteInfo[0]);
                }
                else
                {
                    NonSeedNodeVoteCount++;
                }
            }
        }

        private void VerifyEncryptionFlag()
        {
            foreach (var section in this.windowsFabricSettings.SettingsMap.Where(s => !this.windowsFabricSettings.IsDCASetting(s.Key)))
            {
                foreach (var parameter in section.Value)
                {
                    if (parameter.Value.IsEncrypted)
                    {
                        if (FabricSettingsValidator.AllowedSecureConfigurations.ContainsKey(section.Key))
                        {
                            if (!FabricSettingsValidator.AllowedSecureConfigurations[section.Key].Contains(parameter.Key))
                            {
                                WriteError("Section '{0}', parameter '{1}' cannot be encrypted.", section.Key, parameter.Key);
                            }
                            else if (string.IsNullOrEmpty(parameter.Value.Value))
                            {
                                WriteError("Section '{0}', parameter '{1}' is encrypted and hence cannot be empty.", section.Key, parameter.Key);
                            }
                        }
                        else
                        {
                            WriteError("Section '{0}' cannot contain encrypted parameters.", section.Key, parameter.Key);
                        }
                    }
                }
            }
        }

        private void VerifyDependencies()
        {
            foreach (var dependency in FabricSettingsValidator.Dependencies)
            {
                string[] source = dependency.Key.Split('|');
                foreach (string dest in dependency.Value)
                {
                    string[] destination = dest.Split('|');
                    if ((this.windowsFabricSettings.SettingsMap.ContainsKey(source[0]) && this.windowsFabricSettings.SettingsMap[source[0]].ContainsKey(source[1])))
                    {
                        bool valueComparisonResult = source[2].StartsWith("!", StringComparison.Ordinal) ?
                            this.windowsFabricSettings.SettingsMap[source[0]][source[1]].Value != source[2].Substring(1) :
                            source[2].StartsWith("~", StringComparison.Ordinal) ?
                            this.windowsFabricSettings.SettingsMap[source[0]][source[1]].Value.Equals(source[2].Substring(1), StringComparison.OrdinalIgnoreCase) :
                            this.windowsFabricSettings.SettingsMap[source[0]][source[1]].Value == source[2];
                        if (valueComparisonResult)
                        {
                            if (destination[0] != FabricValidatorConstants.NodeType)
                            {
                                if (!(this.windowsFabricSettings.SettingsMap.ContainsKey(destination[0]) && this.windowsFabricSettings.SettingsMap[destination[0]].ContainsKey(destination[1]) && this.windowsFabricSettings.SettingsMap[destination[0]][destination[1]].IsFromClusterManifest))
                                {
                                    WriteError(
                                        "Section {0} parameter {1} is required if section {2} parameter {3} is {4} set to {5}",
                                        destination[0],
                                        destination[1],
                                        source[0],
                                        source[1],
                                        source[2].StartsWith("!", StringComparison.Ordinal) ? "not" : "",
                                        source[2]);
                                }
                            }
                            else
                            {
                                foreach (var nodeType in nodeTypes)
                                {
                                    if (destination[1] == FabricValidatorConstants.Certificates)
                                    {
                                        VerifyCertificates(source, destination, nodeType);
                                    }
                                    else if (destination[1] == FabricValidatorConstants.Endpoints)
                                    {
                                        VerifyEndpoints(source, destination, nodeType);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        private void VerifyEndpoints(string[] source, string[] destination, ClusterManifestTypeNodeType nodeType)
        {
            if (destination[2] == FabricValidatorConstants.ApplicationEndpoints)
            {
                if (nodeType.Endpoints != null && nodeType.Endpoints.ApplicationEndpoints == null)
                {
                    WriteError(
                    "NodeType.{0}.{1} is required if section {2} parameter {3} is {4} set to {5}",
                    FabricValidatorConstants.Endpoints,
                    FabricValidatorConstants.ApplicationEndpoints,
                    source[0],
                    source[1],
                    source[2].StartsWith("!", StringComparison.Ordinal) ? "not" : "",
                    source[2]);
                }
            }
        }

        private void VerifyCertificates(string[] source, string[] destination, ClusterManifestTypeNodeType nodeType)
        {

            if (nodeType.Certificates == null)
            {
                WriteError(
                    "NodeType.Certificates is required if section {0} parameter {1} is {2} set to {3}",
                    source[0],
                    source[1],
                    source[2].StartsWith("!", StringComparison.Ordinal) ? "not" : "",
                    source[2]);
            }
            else if (destination[2] == FabricValidatorConstants.ClientCertificate)
            {
                if (nodeType.Certificates.ClientCertificate == null)
                {
                    WriteError(
                    "NodeType.Certificates.ClientCertificate is required if section {0} parameter {1} is {2} set to {3}",
                    source[0],
                    source[1],
                    source[2].StartsWith("!", StringComparison.Ordinal) ? "not" : "",
                    source[2]);
                }
                else
                {
                    ValidateCertificateSetting(nodeType.Certificates.ClientCertificate);
                }
            }
            else if (destination[2] == FabricValidatorConstants.ServerCertificate)
            {
                if (nodeType.Certificates.ServerCertificate == null)
                {
                    WriteError(
                    "NodeType.Certificates.ServerCertificate is required if section {0} parameter {1} is {2} set to {3}",
                    source[0],
                    source[1],
                    source[2].StartsWith("!", StringComparison.Ordinal) ? "not" : "",
                    source[2]);
                }
                else
                {
                    ValidateCertificateSetting(nodeType.Certificates.ServerCertificate);
                }
            }
            else if (destination[2] == FabricValidatorConstants.ClusterCertificate)
            {
                if (nodeType.Certificates.ClusterCertificate == null)
                {
                    WriteError(
                    "NodeType.Certificates.ClusterCertificate is required if section {0} parameter {1} is {2} set to {3}",
                    source[0],
                    source[1],
                    source[2].StartsWith("!", StringComparison.Ordinal) ? "not" : "",
                    source[2]);
                }
                else
                {
                    ValidateCertificateSetting(nodeType.Certificates.ClusterCertificate);
                }
            }
        }

        void ValidateCertificateSetting(FabricCertificateType fabricCertificateType)
        {
            if (fabricCertificateType.X509FindType != FabricCertificateTypeX509FindType.FindByThumbprint)
            {
                return;
            }

            for (int i = 0; i < fabricCertificateType.X509FindValue.Length; ++i)
            {
                if (!IsHexDigit(fabricCertificateType.X509FindValue[i]) &&
                    !char.IsWhiteSpace(fabricCertificateType.X509FindValue[i]))
                {
                    WriteError(
                        "{0}: thumbprint string {1} contains invalid HEX digit, [{2}] = 0x{3:x}",
                        fabricCertificateType.Name,
                        fabricCertificateType.X509FindValue,
                        i,
                        (int)(fabricCertificateType.X509FindValue[i]));
                }
            }
        }

        private static bool IsHexDigit(char character)
        {
            return ((character >= '0') && (character <= '9'))
                || ((character >= 'A') && (character <= 'F'))
                || ((character >= 'a') && (character <= 'f'));
        }

        private Dictionary<string, Dictionary<string, WindowsFabricSettings.SettingsValue>> GetAllTVSSettings()
        {
            var allTVSSettings = new Dictionary<string, Dictionary<string, WindowsFabricSettings.SettingsValue>>();
            foreach (string sectionName in windowsFabricSettings.SettingsMap.Keys)
            {
                if (sectionName.StartsWith(FabricValidatorConstants.SectionNames.TokenValidationService, StringComparison.OrdinalIgnoreCase) ||
                    sectionName.StartsWith(FabricValidatorConstants.SectionNames.DSTSTokenValidationService, StringComparison.OrdinalIgnoreCase))
                {
                    foreach (string paramName in windowsFabricSettings.SettingsMap[sectionName].Keys)
                    {
                        var param = windowsFabricSettings.SettingsMap[sectionName][paramName];
                        if (param.IsFromClusterManifest)
                        {
                            if (!allTVSSettings.ContainsKey(sectionName))
                            {
                                allTVSSettings.Add(sectionName, new Dictionary<string, WindowsFabricSettings.SettingsValue>());
                            }
                            allTVSSettings[sectionName].Add(paramName, param);
                        }
                    }
                }
            }
            return allTVSSettings;
        }

        private static bool IsSingleChangeAllowed(WindowsFabricSettings.SettingsValue oldSettings, WindowsFabricSettings.SettingsValue newSettings)
        {
            if (((oldSettings.Value == null) != (newSettings.Value == null)) /* Value is newly added/removed to/from Configurations.csv */  ||
                (oldSettings.Value != null
                && newSettings.Value != null
                && (!oldSettings.IsFromClusterManifest && newSettings.IsFromClusterManifest) || (oldSettings.IsFromClusterManifest && !newSettings.IsFromClusterManifest)))
                /* Value is changed from default -> some value or some value -> default*/ 
            {
                return true;
            }

            return false;
        }

        private static void WriteWarning(string format, params object[] args)
        {
            FabricValidator.TraceSource.WriteWarning(FabricValidatorUtility.TraceTag, format, args);
        }


        private static void WriteError(string format, params object[] args)
        {
            throw new ArgumentException(string.Format(format, args));
        }

        private static void WriteInfo(string format, params object[] args)
        {
            FabricValidator.TraceSource.WriteInfo(FabricValidatorUtility.TraceTag, format, args);
        }

        private string storeName;
        private WindowsFabricSettings windowsFabricSettings;
        private ClusterManifestTypeNodeType[] nodeTypes;
        private DCASettingsValidator dcaSettingsValidator;
        private HttpGatewaySettingsValidator httpGatewaySettingsValidator;
        private HttpAppGatewaySettingsValidator httpAppGatewaySettingsValidator;
        private TokenValidationServiceSettingsValidator tokenValidationServiceValidator;

        // Types to be used for type checking.
        private const string IntType = "int";
        private const string UIntType = "uint";
        private const string TimeSpanType = "TimeSpan";
        private const string CommonTimeSpanType = "Common::TimeSpan";
        private const string BoolType = "bool";
        private const string DoubleType = "double";

        /// <summary>
        /// This is a hashset of required parameters. We ensure that all these parameters are present in a deployment.
        /// In order to add new required parameters,
        ///     If the required parameters are part of sections already listed here, add the required parameters to these sections.
        ///     Else create new section (SectionName, HashSet and add the parameters there
        /// Either case follow the format of adding only constants here and actual value in the constants file.
        /// </summary>
        private static readonly Dictionary<string, HashSet<string>> RequiredParameters =
            new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase) {
                { FabricValidatorConstants.SectionNames.FailoverManager, new HashSet<string>(StringComparer.OrdinalIgnoreCase) {FabricValidatorConstants.ParameterNames.ExpectedClusterSize}},
                { FabricValidatorConstants.SectionNames.Security, new HashSet<string>(StringComparer.OrdinalIgnoreCase) {FabricValidatorConstants.ParameterNames.ClusterCredentialType, FabricValidatorConstants.ParameterNames.ServerAuthCredentialType }}};

        /// <summary>
        /// This is a set of paremeters that can be secured by a security certificate.
        /// Read instructions for RequiredParameters on how to add new securable configurations.
        /// </summary>
        private static readonly Dictionary<string, HashSet<string>> AllowedSecureConfigurations =
            new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase) {
                { FabricValidatorConstants.SectionNames.Management, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.ImageStoreConnectionString }},
                { FabricValidatorConstants.SectionNames.DiagnosticFileStore, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.StoreConnectionString }},
                { FabricValidatorConstants.SectionNames.DiagnosticTableStore, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.StoreConnectionString }},
                { FabricValidatorConstants.SectionNames.Hosting, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.NTLMAuthenticationPasswordSecret }},
                { FabricValidatorConstants.SectionNames.RunAs, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.RunAsPassword }},
                { FabricValidatorConstants.SectionNames.RunAs_Fabric, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.RunAsPassword }},
                { FabricValidatorConstants.SectionNames.RunAs_DCA, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.RunAsPassword }},
                { FabricValidatorConstants.SectionNames.RunAs_HttpGateway, new HashSet<string>(StringComparer.OrdinalIgnoreCase) { FabricValidatorConstants.ParameterNames.RunAsPassword }},
                { FabricValidatorConstants.SectionNames.FileStoreService,
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase)
                    {
                        FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountNTLMPasswordSecret,
                        FabricValidatorConstants.ParameterNames.FileStoreService.PrimaryAccountUserPassword,
                        FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountNTLMPasswordSecret,
                        FabricValidatorConstants.ParameterNames.FileStoreService.SecondaryAccountUserPassword
                    }
                }};

        /// <summary>
        /// This is a set of dependencies.
        /// The key of the hash table describes configurations which have dependencies on other other configurations.
        /// The format of the key "SectionName|ParameterName|[!]Value.
        /// This means the if the parameter in the section provided has a particular value the dependency condition holds.
        /// The optional ! sign indicates negation of value check. The option ~ sign shows case independent comparison.
        /// The HashSet describes the list of configurations that must be present based on the dependency.
        /// The format for elements in the hashset can be one of
        ///     "SectionName|ParameterName" - Indicates particular parameter in section should be present.
        ///     "NodeType|Endpoints|EndpointType" - Indicates particular endpoint type should be present in all node types.
        ///     "NodeType|Certificates|CertificateType" - Indicates particular certificate type should be present in all node types.
        /// </summary>
        private static readonly Dictionary<string, HashSet<string>> Dependencies =
            new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase){
                {
                    string.Format(CultureInfo.InvariantCulture, "{0}|{1}|~{2}", FabricValidatorConstants.SectionNames.Hosting, FabricValidatorConstants.ParameterNames.NTLMAuthenticationEnabled, "true"),
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase) {string.Format(CultureInfo.InvariantCulture, "{0}|{1}", FabricValidatorConstants.SectionNames.Hosting, FabricValidatorConstants.ParameterNames.NTLMAuthenticationPasswordSecret)}
                },
                {
                    string.Format(CultureInfo.InvariantCulture, "{0}|{1}|{2}", FabricValidatorConstants.SectionNames.Security, FabricValidatorConstants.ParameterNames.ServerAuthCredentialType, FabricValidatorConstants.CredentialTypeX509),
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase) { string.Format(CultureInfo.InvariantCulture, "{0}|{1}|{2}", FabricValidatorConstants.NodeType, FabricValidatorConstants.Certificates, FabricValidatorConstants.ClientCertificate),
                        string.Format(CultureInfo.InvariantCulture, "{0}|{1}|{2}", FabricValidatorConstants.NodeType, FabricValidatorConstants.Certificates, FabricValidatorConstants.ServerCertificate)}
                },
                {
                    string.Format(CultureInfo.InvariantCulture, "{0}|{1}|{2}", FabricValidatorConstants.SectionNames.Security, FabricValidatorConstants.ParameterNames.ClusterCredentialType, FabricValidatorConstants.CredentialTypeX509),
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase) {string.Format(CultureInfo.InvariantCulture, "{0}|{1}|{2}", FabricValidatorConstants.NodeType, FabricValidatorConstants.Certificates, FabricValidatorConstants.ClusterCertificate)}
                },
                {
                    string.Format(CultureInfo.InvariantCulture, "{0}|{1}|~{2}", FabricValidatorConstants.SectionNames.DiagnosticFileStore, FabricValidatorConstants.ParameterNames.IsEnabled, "true"),
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase) {string.Format(CultureInfo.InvariantCulture, "{0}|{1}", FabricValidatorConstants.SectionNames.DiagnosticFileStore, FabricValidatorConstants.ParameterNames.StoreConnectionString)}
                },
                {
                    string.Format(CultureInfo.InvariantCulture, "{0}|{1}|~{2}", FabricValidatorConstants.SectionNames.DiagnosticFileStore, FabricValidatorConstants.ParameterNames.IsAppLogCollectionEnabled, "true"),
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase) {string.Format(CultureInfo.InvariantCulture, "{0}|{1}", FabricValidatorConstants.SectionNames.DiagnosticFileStore, FabricValidatorConstants.ParameterNames.StoreConnectionString)}
                },
                {
                    string.Format(CultureInfo.InvariantCulture, "{0}|{1}|~{2}", FabricValidatorConstants.SectionNames.DiagnosticTableStore, FabricValidatorConstants.ParameterNames.IsEnabled, "true"),
                    new HashSet<string>(StringComparer.OrdinalIgnoreCase) {string.Format(CultureInfo.InvariantCulture, "{0}|{1}", FabricValidatorConstants.SectionNames.DiagnosticTableStore, FabricValidatorConstants.ParameterNames.StoreConnectionString)}
                }
            };
    }
}