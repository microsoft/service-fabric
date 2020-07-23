// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;

    internal enum StorageServiceType
    {
        // ReSharper disable once InconsistentNaming - Enum.ToString is used for generating URI.
        blob,
        // ReSharper disable once InconsistentNaming - Enum.ToString is used for generating URI.
        table,
        // ReSharper disable once InconsistentNaming - Enum.ToString is used for generating URI.
        queue
    }

    internal class SettingsValidator
    {
        private readonly IUserConfig clusterProperties;
        private readonly ClusterManifestGeneratorSettings clusterManifestGeneratorSettings;
        private readonly FabricSettingsMetadata fabricSettingsMetadata;
        private readonly Dictionary<string, HashSet<string>> requiredParameters;

        public SettingsValidator(
            IUserConfig clusterProperties,
            FabricSettingsMetadata fabricSettingsMetadata,
            IDictionary<string, HashSet<string>> requiredParameters,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings)
        {
            clusterProperties.MustNotBeNull("clusterProperties");
            requiredParameters.MustNotBeNull("fabricSettingsMetadata");
            requiredParameters.MustNotBeNull("requiredParameters");

            this.clusterProperties = clusterProperties;
            this.fabricSettingsMetadata = fabricSettingsMetadata;
            this.clusterManifestGeneratorSettings = clusterManifestGeneratorSettings;
            this.requiredParameters = new Dictionary<string, HashSet<string>>(
                requiredParameters,
                StringComparer.OrdinalIgnoreCase);
        }

        public virtual void Validate(bool isNewResource)
        {
            this.ValidatNodeTypes(isNewResource);
            this.ValidateFabricSettings();
            this.ValidateCertificates();
            this.ValidateDiagnosticsStorageAccountConfig();
            this.ValidateDiagnosticsStoreInformation();
            this.ValidateAzureActiveDirectory();
        }
        
        internal static void ValidateCommonNames(X509 certInfo)
        {
            // CNs must not be null
            IEnumerable<CertificateCommonNameBase> allCns = SettingsValidator.GetAllCommonNames(certInfo);
            if (allCns.Any(cn => string.IsNullOrWhiteSpace(cn.CertificateCommonName)))
            {
                throw new ValidationException(ClusterManagementErrorCode.InvalidCommonName);
            }

            IEnumerable<CertificateCommonNameBase> allCnsWithoutItSupport = SettingsValidator.GetAllCommonNames(certInfo, false);
            if (allCnsWithoutItSupport.Any(cn => !string.IsNullOrWhiteSpace(cn.CertificateIssuerThumbprint)))
            {
                throw new ValidationException(ClusterManagementErrorCode.UnsupportedIssuerThumbprintPair);
            }

            // Issuer thumbprints must be separated by comma, and should not dupe under the same cn
            IEnumerable<CertificateCommonNameBase> allCnsWithItSupport = SettingsValidator.GetAllCommonNames(certInfo, true);
            foreach (CertificateCommonNameBase cn in allCnsWithItSupport)
            {
                if (!string.IsNullOrWhiteSpace(cn.CertificateIssuerThumbprint)
                    && !SettingsValidator.AreValidIssuerThumbprints(cn.CertificateIssuerThumbprint))
                {
                    throw new ValidationException(ClusterManagementErrorCode.InvalidCertificateIssuerThumbprints, cn.CertificateIssuerThumbprint);
                }
            }

            // Up to 2 CNs are supported, except client CN
            List<ServerCertificateCommonNames> cnsWithCountCap = new List<ServerCertificateCommonNames>()
            {
                certInfo.ClusterCertificateCommonNames,
                certInfo.ReverseProxyCertificateCommonNames,
                certInfo.ServerCertificateCommonNames
            };
            if (cnsWithCountCap.Any(cns => cns != null && cns.CommonNames != null && cns.CommonNames.Count > 2))
            {
                throw new ValidationException(ClusterManagementErrorCode.InvalidCommonNameCount);
            }

            // no dupe CN is allowed for the same cert type, except client CN
            foreach (List<CertificateCommonNameBase> cns in cnsWithCountCap
                .Where(p => p != null && p.CommonNames != null)
                .Select(p => p.CommonNames))
            {
                if (cns.Any(current =>
                        cns.Any(other => current != other && current.CertificateCommonName == other.CertificateCommonName)))
                {
                    throw new ValidationException(ClusterManagementErrorCode.DupeCommonNameNotAllowedForClusterCert);
                }
            }
        }

        internal static void ValidateCommonNamesAgainstThumbprints(X509 certInfo)
        {
            // for any cert except client cert, CN and thumbprint are not allowed to be configured together
            KeyValuePair<ServerCertificateCommonNames, CertificateDescription>[] pairs = new KeyValuePair<ServerCertificateCommonNames, CertificateDescription>[]
            {
                new KeyValuePair<ServerCertificateCommonNames, CertificateDescription>(certInfo.ClusterCertificateCommonNames, certInfo.ClusterCertificate),
                new KeyValuePair<ServerCertificateCommonNames, CertificateDescription>(certInfo.ServerCertificateCommonNames, certInfo.ServerCertificate),
                new KeyValuePair<ServerCertificateCommonNames, CertificateDescription>(certInfo.ReverseProxyCertificateCommonNames, certInfo.ReverseProxyCertificate),
            };
            
            foreach (var pair in pairs)
            {
                ServerCertificateCommonNames cns = pair.Key;
                CertificateDescription thumbprints = pair.Value;
                if (cns != null && thumbprints != null)
                {
                    throw new ValidationException(ClusterManagementErrorCode.InvalidCommonNameThumbprintPair);
                }
            }
        }

        internal static void ValidateIssuerCertStore(X509 certInfo)
        {
            // Issuercertstore must not exist if corresponding cn does not exist for cert
            if (certInfo.ClusterCertificateCommonNames == null && certInfo.ClusterCertificateIssuerStores != null && certInfo.ClusterCertificateIssuerStores.Count() != 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreSpecifiedWithoutCommonNameCertificate);
            }

            if (certInfo.ServerCertificateCommonNames == null && certInfo.ServerCertificateIssuerStores != null && certInfo.ServerCertificateIssuerStores.Count() != 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreSpecifiedWithoutCommonNameCertificate);
            }

            if (certInfo.ClientCertificateCommonNames == null && certInfo.ClientCertificateIssuerStores != null && certInfo.ClientCertificateIssuerStores.Count() != 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreSpecifiedWithoutCommonNameCertificate);
            }

            // Issuer thumbprint pinning and issuer store cannot be specified together
            SettingsValidator.ValidateIssuerPinningAndIssuerStoresCannotExistTogether(certInfo);

            if (certInfo.ClusterCertificateIssuerStores != null && certInfo.ClusterCertificateIssuerStores.Count != 0)
            {
                SettingsValidator.ValidateClusterServerIssuerCertStoreProperties(certInfo.ClusterCertificateIssuerStores);
            }

            if (certInfo.ServerCertificateIssuerStores != null && certInfo.ServerCertificateIssuerStores.Count != 0)
            {
                SettingsValidator.ValidateClusterServerIssuerCertStoreProperties(certInfo.ServerCertificateIssuerStores);
            }

            if (certInfo.ClientCertificateIssuerStores != null && certInfo.ClientCertificateIssuerStores.Count != 0)
            {
                SettingsValidator.ValidateClientIssuerCertStoreProperties(certInfo.ClientCertificateIssuerStores);
            }
        }

        internal static bool IsValidThumbprint(string thumbprint)
        {
            return thumbprint.ToLower().All(ch => (
                (ch == ' ') || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')));
        }

        internal static bool IsValidThumbprintList(List<string> thumbprints)
        {
            foreach (string thumbprint in thumbprints)
            {
                if (!IsValidThumbprint(thumbprint))
                {
                    return false;
                }
            }

            return true;
        }

        internal static bool AreValidIssuerThumbprints(string issuerThumbprints)
        {
            string[] thumbprints = issuerThumbprints.Split(',');
            if (!thumbprints.All(p => SettingsValidator.IsValidThumbprint(p)))
            {
                return false;
            }

            for (int i = 0; i < thumbprints.Length; i++)
            {
                for (int j = i + 1; j < thumbprints.Length; j++)
                {
                    if (string.Equals(thumbprints[i], thumbprints[j], StringComparison.OrdinalIgnoreCase))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        internal static IEnumerable<CertificateCommonNameBase> GetAllCommonNames(X509 certInfo)
        {
            return new List<IEnumerable<CertificateCommonNameBase>>
                {
                    certInfo.ClientCertificateCommonNames,
                    certInfo.ClusterCertificateCommonNames != null ? certInfo.ClusterCertificateCommonNames.CommonNames : null,
                    certInfo.ServerCertificateCommonNames != null ? certInfo.ServerCertificateCommonNames.CommonNames : null,
                    certInfo.ReverseProxyCertificateCommonNames != null ? certInfo.ReverseProxyCertificateCommonNames.CommonNames : null,
                }.Where(p => p != null).SelectMany(q => q);
        }

        internal static IEnumerable<CertificateCommonNameBase> GetAllCommonNames(X509 certInfo, bool isIssuerThumbprintSupported)
        {
            if (isIssuerThumbprintSupported)
            {
                return new List<IEnumerable<CertificateCommonNameBase>>
                {
                    certInfo.ClientCertificateCommonNames,
                    certInfo.ClusterCertificateCommonNames != null ? certInfo.ClusterCertificateCommonNames.CommonNames : null,
                    certInfo.ServerCertificateCommonNames != null ? certInfo.ServerCertificateCommonNames.CommonNames : null,
                }.Where(p => p != null).SelectMany(q => q);
            }
            else
            {
                return new List<IEnumerable<CertificateCommonNameBase>>
                {
                    certInfo.ReverseProxyCertificateCommonNames != null ? certInfo.ReverseProxyCertificateCommonNames.CommonNames : null,
                }.Where(p => p != null).SelectMany(q => q);
            }
        }

        internal static void ValidateThumbprints(X509 certInfo)
        {
            List<ClientCertificateThumbprint> clientCertificateThumbprints = certInfo.ClientCertificateThumbprints;
            Dictionary<List<string>, ClusterManagementErrorCode> thumbprintListsToValidate = new Dictionary<List<string>, ClusterManagementErrorCode>();

            if (clientCertificateThumbprints != null)
            {
                thumbprintListsToValidate.Add(clientCertificateThumbprints.Select(ct => ct.CertificateThumbprint).ToList(), ClusterManagementErrorCode.InvalidClientCertificateThumbprint);
            }

            if (certInfo.ServerCertificate != null)
            {
                thumbprintListsToValidate.Add(certInfo.ServerCertificate.ToThumbprintList(), ClusterManagementErrorCode.InvalidServerCertificateThumbprint);
            }

            if (certInfo.ClusterCertificate != null)
            {
                thumbprintListsToValidate.Add(certInfo.ClusterCertificate.ToThumbprintList(), ClusterManagementErrorCode.InvalidClusterCertificateThumbprint);
            }

            if (certInfo.ReverseProxyCertificate != null)
            {
                thumbprintListsToValidate.Add(certInfo.ReverseProxyCertificate.ToThumbprintList(), ClusterManagementErrorCode.InvalidReverseProxyCertificateThumbprint);
            }

            foreach (KeyValuePair<List<string>, ClusterManagementErrorCode> entry in thumbprintListsToValidate)
            {
                if (!IsValidThumbprintList(entry.Key))
                {
                    throw new ValidationException(entry.Value);
                }
            }
        }

        internal void ValidateCertificates()
        {
            if (this.clusterProperties.Security == null || this.clusterProperties.Security.CertificateInformation == null)
            {
                return;
            }

            X509 certInfo = this.clusterProperties.Security.CertificateInformation;
            List<ClientCertificateThumbprint> clientCertificateThumbprints = certInfo.ClientCertificateThumbprints;
            List<ClientCertificateCommonName> clientCertificateCommonNames = certInfo.ClientCertificateCommonNames;

            bool serverCertificatePresent = certInfo.ServerCertificate != null || (certInfo.ServerCertificateCommonNames != null && certInfo.ServerCertificateCommonNames.Any());
            bool clientCertificatInfoPresent = (clientCertificateCommonNames != null && clientCertificateCommonNames.Count > 0)
                                               || (clientCertificateThumbprints != null && clientCertificateThumbprints.Count > 0);

            if (clientCertificatInfoPresent && !serverCertificatePresent)
            {
                throw new ValidationException(ClusterManagementErrorCode.ClientCertDefinedWithoutServerCert);
            }
            
            SettingsValidator.ValidateThumbprints(certInfo);
            SettingsValidator.ValidateCommonNames(certInfo);
            SettingsValidator.ValidateCommonNamesAgainstThumbprints(certInfo);
            SettingsValidator.ValidateIssuerCertStore(certInfo);
        }

        private static void ValidateIssuerPinningAndIssuerStoresCannotExistTogether(X509 certInfo)
        {
            if (certInfo.ClusterCertificateCommonNames != null &&
                certInfo.ClusterCertificateCommonNames.CommonNames != null &&
                certInfo.ClusterCertificateCommonNames.CommonNames.Any(commonName => !string.IsNullOrEmpty(commonName.CertificateIssuerThumbprint)) &&
                certInfo.ClusterCertificateIssuerStores != null &&
                certInfo.ClusterCertificateIssuerStores.Count() != 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreAndIssuerPinningCannotBeTogether);
            }

            if (certInfo.ServerCertificateCommonNames != null &&
                certInfo.ServerCertificateCommonNames.CommonNames != null &&
                certInfo.ServerCertificateCommonNames.CommonNames.Any(commonName => !string.IsNullOrEmpty(commonName.CertificateIssuerThumbprint)) &&
                certInfo.ServerCertificateIssuerStores != null &&
                certInfo.ServerCertificateIssuerStores.Count() != 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreAndIssuerPinningCannotBeTogether);
            }

            if (certInfo.ClientCertificateCommonNames != null &&
                certInfo.ClientCertificateCommonNames.Any(clientCert => !string.IsNullOrEmpty(clientCert.CertificateIssuerThumbprint)) &&
                certInfo.ClientCertificateIssuerStores != null &&
                certInfo.ClientCertificateIssuerStores.Count() != 0)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreAndIssuerPinningCannotBeTogether);
            }
        }

        private static void ValidateClusterServerIssuerCertStoreProperties(List<CertificateIssuerStore> issuerStore)
        {
            SettingsValidator.ValidateIssuerCertStoreCommonProperties(issuerStore);

            if (issuerStore.Count() > 2)
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCertStoreCNMoreThan2);
            }

            // Validate duplicate issuer CN for server and cluster issuer certs
            if (issuerStore.Count() == 2)
            {
                if (string.Compare(issuerStore[0].IssuerCommonName, issuerStore[1].IssuerCommonName, true) == 0)
                {
                    throw new ValidationException(ClusterManagementErrorCode.DupIssuerCertificateCN);
                }
            }     
        }

        private static void ValidateClientIssuerCertStoreProperties(List<CertificateIssuerStore> clientIssuerStore)
        {
            SettingsValidator.ValidateIssuerCertStoreCommonProperties(clientIssuerStore);

            // Validate duplicate issuer CN
            bool hasDuplicate = clientIssuerStore.Any(current => clientIssuerStore.Any(other => current != other && string.Compare(current.IssuerCommonName, other.IssuerCommonName, true) == 0));

            if (hasDuplicate)
            {
                throw new ValidationException(ClusterManagementErrorCode.DupIssuerCertificateCN);
            }
        }

         private static void ValidateIssuerCertStoreCommonProperties(List<CertificateIssuerStore> issuerStore)
        {
            // Validate null entries for issuerCN
            if (issuerStore.Any(issuerCert => issuerCert.IssuerCommonName == null))
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerCNCannotBeNull);
            }

            // Validate null entries for issuer store names
            if (issuerStore.Any(issuerCert => string.IsNullOrEmpty(issuerCert.X509StoreNames)))
            {
                throw new ValidationException(ClusterManagementErrorCode.IssuerStoreNameCannotBeNull);
            }

            // Verify duplicate cert store entries - For linux store names are case sensitive, for windows store names are case insensitive
            foreach (var issuerCert in issuerStore)
            {
                var x509StoreNames = issuerCert.X509StoreNames.Split(',').ToList().Select(p => p.Trim());
#if !DotNetCoreClrLinux
                var hasDuplicate = x509StoreNames.GroupBy(item => item, StringComparer.OrdinalIgnoreCase).Where(g => g.Count() > 1).Any();
#else
                var hasDuplicate = x509StoreNames.GroupBy(item => item).Where(g => g.Count() > 1).Any();
#endif

                if (hasDuplicate)
                {
                    throw new ValidationException(ClusterManagementErrorCode.DupIssuerCertificateStore);
                }
            }
        }

        private void ValidateFabricSettings()
        {
            IEnumerable<SettingsSectionDescription> userDefinedClusterSettings = this.clusterProperties.FabricSettings;

            if (userDefinedClusterSettings == null && this.requiredParameters.Count > 0)
            {
                // If userDefinedClusterSettings is null then fail with the first RequiredParameter
                throw new ValidationException(ClusterManagementErrorCode.RequiredSectionNotFound, this.requiredParameters.ElementAt(0));
            }

            // Ensure required parameters are specified in fabricSettings
            foreach (var requiredSection in this.requiredParameters)
            {
                // ReSharper disable once AssignNullToNotNullAttribute - This value cannot be null because we check null and count of requiredParameters above. Code only gets here if Count of requiredParameters is greater than 0, which means that the userDefinedClusterSettings is not null based on the if condition above.
                var matchingSection = userDefinedClusterSettings.FirstOrDefault(
                                          section => string.Equals(section.Name, requiredSection.Key, StringComparison.OrdinalIgnoreCase));
                if (matchingSection == null)
                {
                    throw new ValidationException(ClusterManagementErrorCode.RequiredSectionNotFound, requiredSection.Key);
                }

                foreach (var requiredParameter in requiredSection.Value)
                {
                    var matchingParameter =
                        matchingSection.Parameters.FirstOrDefault(
                            parameter =>
                            string.Equals(parameter.Name, requiredParameter, StringComparison.OrdinalIgnoreCase));
                    if (matchingParameter == null || string.IsNullOrEmpty(matchingParameter.Value))
                    {
                        throw new ValidationException(
                            ClusterManagementErrorCode.RequiredParameterNotFound,
                            requiredSection.Key,
                            requiredParameter);
                    }
                }
            }

            if (userDefinedClusterSettings != null)
            {
                var hashSet = new HashSet<string>();

                foreach (var userDefinedSection in userDefinedClusterSettings)
                {
                    foreach (var userDefinedParameter in userDefinedSection.Parameters)
                    {
                        // Ensure only allowed parameters are specified in fabricSettings
                        if (!this.fabricSettingsMetadata.IsValidConfiguration(userDefinedSection.Name, userDefinedParameter.Name))
                        {
                            throw new ValidationException(ClusterManagementErrorCode.ParameterNotAllowed, userDefinedSection.Name, userDefinedParameter.Name);
                        }

                        // Validate that userdefined section shpuld not have duplicate parameters
                        string paramLower = userDefinedParameter.Name;
                        if (!hashSet.Add(paramLower.ToLower()))
                        {
                            throw new ValidationException(ClusterManagementErrorCode.DuplicateParameterNotAllowed, userDefinedParameter.Name, userDefinedSection.Name);
                        }
                    }

                    hashSet.Clear();
                }
            }
        }

        private void ValidatNodeTypes(bool isNewResource)
        {
            IEnumerable<NodeTypeDescription> nodeTypes = this.clusterProperties.NodeTypes;

            if (nodeTypes == null || !nodeTypes.Any())
            {
                throw new ValidationException(ClusterManagementErrorCode.NodeTypesCannotBeNull);
            }

            HashSet<string> duplicateNodeTypeDetectory = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        
            foreach (var nodeType in nodeTypes)
            {
                if (duplicateNodeTypeDetectory.Contains(nodeType.Name))
                {
                    throw new ValidationException(ClusterManagementErrorCode.DuplicateNodeTypeNotAllowed, nodeType.Name);
                }

                duplicateNodeTypeDetectory.Add(nodeType.Name);

                this.ValidateEndpointRange(nodeType.ApplicationPorts, "ApplicationPorts");

                if (nodeType.EphemeralPorts != null)
                {
                    this.ValidateDynamicPorts(nodeType.EphemeralPorts);
                }

                this.ValidateEndpoint("HttpGatewayEndpoint", nodeType.HttpGatewayEndpointPort);
                this.ValidateEndpoint("ClientConnectionEndpoint", nodeType.ClientConnectionEndpointPort);

                if (nodeType.HttpApplicationGatewayEndpointPort != null)
                {
                    this.ValidateEndpoint("HttpApplicationGatewayEndpoint", nodeType.HttpApplicationGatewayEndpointPort.Value);
                }

                if (nodeType.KtlLogger != null)
                {
                    this.ValidateKtlLogger(nodeType.KtlLogger);
                }

                if (nodeType.LogicalDirectories != null)
                {
                    HashSet<string> duplicateLogicalDirectoriesNameDetectory = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

                    foreach (var dir in nodeType.LogicalDirectories)
                    {
                        if (string.IsNullOrEmpty(dir.LogicalDirectoryName))
                        {
                            throw new ValidationException(ClusterManagementErrorCode.LogicalDirectoryNameCannotBeNull);
                        }

                        if (string.IsNullOrEmpty(dir.MappedTo))
                        {
                            throw new ValidationException(ClusterManagementErrorCode.LogicalDirectoryMountedToValueCannotBeNull);
                        }

                        if (!duplicateLogicalDirectoriesNameDetectory.Add(dir.LogicalDirectoryName))
                        {
                            throw new ValidationException(ClusterManagementErrorCode.DuplicateLogicalDirectoriesNameDetected);
                        }                 
                    }
                }
            }

            if (this.clusterProperties.TotalNodeCount != 1 && (this.clusterProperties.TotalNodeCount < this.clusterManifestGeneratorSettings.MinClusterSize
            || this.clusterProperties.TotalNodeCount > this.clusterManifestGeneratorSettings.MaxClusterSize))
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.ClusterSizeNotSupported,
                    this.clusterProperties.TotalNodeCount,
                    this.clusterManifestGeneratorSettings.MinClusterSize,
                    this.clusterManifestGeneratorSettings.MaxClusterSize);
            }

            if (isNewResource && this.clusterProperties.PrimaryNodeTypes.Count() != 1)
            {
                // Ensure that all new resources created have exactly one NodeType marked as primary
                throw new ValidationException(ClusterManagementErrorCode.InvalidNumberofPrimaryNodeTypes, this.clusterProperties.PrimaryNodeTypes.Count());
            }

            if (isNewResource &&
                this.clusterProperties.TotalPrimaryNodeTypeNodeCount < this.clusterProperties.ReliabilityLevel.GetSeedNodeCount())
            {
                var primaryNodeType = this.clusterProperties.PrimaryNodeTypes.First();
                throw new ValidationException(
                    ClusterManagementErrorCode.VMInstanceCountNotSufficientForReliability,
                    primaryNodeType.VMInstanceCount,
                    primaryNodeType.Name,
                    this.clusterProperties.ReliabilityLevel.GetSeedNodeCount(),
                    this.clusterProperties.ReliabilityLevel);
            }
        }

        private void ValidateKtlLogger(KtlLogger ktlLogger)
        {
            if (!string.IsNullOrEmpty(ktlLogger.SharedLogFileId) && string.IsNullOrEmpty(ktlLogger.SharedLogFilePath))
            {
                throw new ValidationException(ClusterManagementErrorCode.SharedLogFilePathCannotBeNull);
            }

            if (!string.IsNullOrEmpty(ktlLogger.SharedLogFilePath) && string.IsNullOrEmpty(ktlLogger.SharedLogFileId))
            {
                throw new ValidationException(ClusterManagementErrorCode.SharedLogFileIdCannotBeNull);
            }
        }

        private void ValidateEndpoint(string endpointName, int portNumber)
        {
            if (portNumber < this.clusterManifestGeneratorSettings.MinAllowedPortNumber
                || portNumber > this.clusterManifestGeneratorSettings.MaxAllowedPortNumber)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.PortOutOfRange,
                    endpointName,
                    portNumber,
                    this.clusterManifestGeneratorSettings.MinAllowedPortNumber,
                    this.clusterManifestGeneratorSettings.MaxAllowedPortNumber);
            }

            if (portNumber >= this.clusterManifestGeneratorSettings.StartReservedPortNumber
                && portNumber <= this.clusterManifestGeneratorSettings.EndReservedPortNumber)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.PortInReservedRange,
                    endpointName,
                    portNumber,
                    this.clusterManifestGeneratorSettings.StartReservedPortNumber,
                    this.clusterManifestGeneratorSettings.EndReservedPortNumber);
            }
        }

        private void ValidateDynamicPorts(EndpointRangeDescription endpointRange)
        {
            this.ValidateEndpointRange(endpointRange, "DynamicPorts");

            if (endpointRange.EndPort - endpointRange.StartPort < this.clusterManifestGeneratorSettings.MinDynamicPortCount)
            {
                throw new ValidationException(ClusterManagementErrorCode.InsufficientDynamicPorts, this.clusterManifestGeneratorSettings.MinDynamicPortCount);
            }
        }

        private void ValidateEndpointRange(EndpointRangeDescription endpointRange, string endpointRangeName)
        {
            if (endpointRange.StartPort < this.clusterManifestGeneratorSettings.MinAllowedPortNumber
                || endpointRange.StartPort > this.clusterManifestGeneratorSettings.MaxAllowedPortNumber)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.PortOutOfRange,
                    endpointRangeName,
                    endpointRange.StartPort,
                    this.clusterManifestGeneratorSettings.MinAllowedPortNumber,
                    this.clusterManifestGeneratorSettings.MaxAllowedPortNumber);
            }

            if (endpointRange.EndPort < this.clusterManifestGeneratorSettings.MinAllowedPortNumber
                || endpointRange.EndPort > this.clusterManifestGeneratorSettings.MaxAllowedPortNumber)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.PortOutOfRange,
                    endpointRangeName,
                    endpointRange.EndPort,
                    this.clusterManifestGeneratorSettings.MinAllowedPortNumber,
                    this.clusterManifestGeneratorSettings.MaxAllowedPortNumber);
            }

            if (endpointRange.StartPort > endpointRange.EndPort)
            {
                throw new ValidationException(ClusterManagementErrorCode.StartPortGreaterThanEnd, endpointRangeName);
            }
        }

        private void ValidateDiagnosticsStoreInformation()
        {
            EncryptableDiagnosticStoreInformation dcaInfo = this.clusterProperties.DiagnosticsStoreInformation;
            if (dcaInfo != null)
            {
                if (dcaInfo.IsEncrypted && dcaInfo.StoreType == DiagnosticStoreType.FileShare)
                {
                    throw new ValidationException(ClusterManagementErrorCode.EncryptionForFileShareStoreTypeNotSupported);
                }
            }
        }

        private void ValidateDiagnosticsStorageAccountConfig()
        {
            DiagnosticsStorageAccountConfig diagnosticsStorageAccountConfig = this.clusterProperties.DiagnosticsStorageAccountConfig;
            if (diagnosticsStorageAccountConfig != null)
            {
                if (!this.clusterManifestGeneratorSettings.AllowUnprotectedDiagnosticsStorageAccountKeys &&
                    (!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.PrimaryAccessKey) ||
                     !string.IsNullOrEmpty(diagnosticsStorageAccountConfig.SecondaryAccessKey)))
                {
                    throw new ValidationException(ClusterManagementErrorCode.UnprotectedDiagnosticsStorageAccountKeysNotAllowed);
                }

                ProtectedAccountKeyName protectedAccountKeyName;
                if (!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.ProtectedAccountKeyName) && !Enum.TryParse(diagnosticsStorageAccountConfig.ProtectedAccountKeyName, out protectedAccountKeyName))
                {
                    throw new ValidationException(ClusterManagementErrorCode.InvalidProtectedDiagnosticsStorageAccountKeyName);
                }

                if ((!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.PrimaryAccessKey) ||
                     !string.IsNullOrEmpty(diagnosticsStorageAccountConfig.SecondaryAccessKey)) &&
                    !string.IsNullOrEmpty(diagnosticsStorageAccountConfig.ProtectedAccountKeyName))
                {
                    throw new ValidationException(ClusterManagementErrorCode.ProtectedAndUnprotectedDiagnosticsStorageAccountKeysNotCompatible);
                }

                if (string.IsNullOrEmpty(diagnosticsStorageAccountConfig.PrimaryAccessKey) &&
                    string.IsNullOrEmpty(diagnosticsStorageAccountConfig.SecondaryAccessKey) &&
                    string.IsNullOrEmpty(diagnosticsStorageAccountConfig.ProtectedAccountKeyName))
                {
                    throw new ValidationException(ClusterManagementErrorCode.ProtectedDiagnosticsStorageAccountKeyNotSpecified);
                }

                if (!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.BlobEndpoint) && !this.ValidateStorageServiceEndpoint(diagnosticsStorageAccountConfig.StorageAccountName, StorageServiceType.blob, diagnosticsStorageAccountConfig.BlobEndpoint))
                {
                    throw new ValidationException(ClusterManagementErrorCode.InvalidDiagnosticsStorageAccountBlobEndpoint, diagnosticsStorageAccountConfig.BlobEndpoint, diagnosticsStorageAccountConfig.StorageAccountName);
                }

                if (!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.TableEndpoint) && !this.ValidateStorageServiceEndpoint(diagnosticsStorageAccountConfig.StorageAccountName, StorageServiceType.table, diagnosticsStorageAccountConfig.TableEndpoint))
                {
                    throw new ValidationException(ClusterManagementErrorCode.InvalidDiagnosticsStorageAccountTableEndpoint, diagnosticsStorageAccountConfig.TableEndpoint, diagnosticsStorageAccountConfig.StorageAccountName);
                }

                /*
                if ((!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.BlobEndpoint) ? 1 : 0 ) + (!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.TableEndpoint) ? 1 : 0) == 1)
                {
                    throw new ValidationException(ClusterManagementErrorCode.InvalidDiagnosticsStorageAccountStorageServiceEndpointCombination);
                }*/
            }
        }

        private void ValidateAzureActiveDirectory()
        {
            if (this.clusterProperties.Security != null && this.clusterProperties.Security.AzureActiveDirectory != null)
            {
                //// Server certificate is needed at a minimum for Transport Layer Security
                //// (SSL/TCP, HTTPS)

                if (this.clusterProperties.Security.CertificateInformation == null)
                {
                    throw new ValidationException(ClusterManagementErrorCode.ServerAuthCredentialTypeRequired);
                }
            }
        }

        private bool ValidateStorageServiceEndpoint(string storageAccountName, StorageServiceType storageServiceType, string storageServiceEndpoint)
        {
            Uri storageServiceEndpointUri;
            if (!Uri.TryCreate(storageServiceEndpoint, UriKind.Absolute, out storageServiceEndpointUri))
            {
                return false;
            }

            // Custom domain name for storage services is not supported.
            string storageServiceEndpointPublicAzure = string.Format(CultureInfo.InvariantCulture, "https://{0}.{1}.{2}/", storageAccountName, storageServiceType, "core.windows.net");
            string storageServiceEndpointMooncake = string.Format(CultureInfo.InvariantCulture, "https://{0}.{1}.{2}/", storageAccountName, storageServiceType, "core.chinacloudapi.cn");

            if (string.Compare(storageServiceEndpoint.TrimEnd('/'), storageServiceEndpointPublicAzure.TrimEnd('/'), StringComparison.OrdinalIgnoreCase) != 0 &&
                string.Compare(storageServiceEndpoint.TrimEnd('/'), storageServiceEndpointMooncake.TrimEnd('/'), StringComparison.OrdinalIgnoreCase) != 0)
            {
                return false;
            }

            return true;
        }
    }
}