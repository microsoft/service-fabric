// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Model;

    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    internal class StandaloneSettingsValidator
    {
        private static readonly TraceType TraceType = new TraceType("StandaloneSettingsValidator");
        private SettingsValidator baseValidator;
        private StandAloneInstallerJsonModelBase inputConfig;

        public StandaloneSettingsValidator(StandAloneInstallerJsonModelBase inputConfiguration)
        {
            SettingsType wrpSettings;
            IDictionary<string, HashSet<string>> requiredParameters;
            PullRequiredWrpSettings(out wrpSettings, out requiredParameters);
            var fabricSettingsMetadata = FabricSettingsMetadata.Create(StandaloneUtility.FindFabricResourceFile(DMConstants.ConfigurationsFileName));

            this.Topology = inputConfiguration.GetClusterTopology();
            ClusterManifestGeneratorSettings generatorSettings = StandAloneCluster.GenerateSettings(wrpSettings, this.Topology);

            this.ClusterProperties = inputConfiguration.GetUserConfig();
            this.baseValidator = new SettingsValidator(this.ClusterProperties, fabricSettingsMetadata, requiredParameters, generatorSettings);
            this.inputConfig = inputConfiguration;
            this.TraceLogger = new StandAloneTraceLogger("StandAloneDeploymentManager");
        }

        public StandAloneClusterTopology Topology
        {
            get;
            private set;
        }

        public StandAloneUserConfig ClusterProperties
        {
            get;
            private set;
        }

        public ITraceLogger TraceLogger
        {
            get;
            private set;
        }

        public void Validate(bool validateDownNodes)
        {
            // TODO: (maburlik) Validate conforms to JSON config model
            // i.e.
            // - For each specified fabricdataroot/fabriclogroot, should check disk space on associated drive on each machine (GetDiskFreeSpaceEx)

            // -----------------------------------------------------------------
            // Common validation
            // -----------------------------------------------------------------
            this.baseValidator.Validate(!this.ClusterProperties.IsScaleMin);

            this.ValidateDiagnosticStore();

            this.ValidateSecurity(validateDownNodes);

            this.ValidateTopology();
        }

        /// <summary>
        /// Validates if the provided configuration is valid for upgrading the current config to.
        /// </summary>
        /// <param name="currentConfig">The current IUserConfig.</param>
        public async Task ValidateUpdateFrom(IUserConfig currentConfig, IClusterTopology currentTopology, bool connectedToCluster)
        {
            SFDeployerTrace.WriteInfo(StringResources.Info_BPAUpgradeValidationStart);
            currentConfig.MustNotBeNull("currentConfig");

            var newCsmConfig = this.ClusterProperties;

            // Validate that node name has not changed
            if (currentConfig.ClusterName != newCsmConfig.ClusterName)
            {
                throw new ValidationException(ClusterManagementErrorCode.ClusterNameCantBeChanged);
            }

            // Case-insensitive string comparison. So, '1.0.1' is considered different from '1.00.1'
            if (currentConfig.Version.Equals(newCsmConfig.Version))
            {
                throw new ValidationException(ClusterManagementErrorCode.UpgradeWithIdenticalConfigVersionNotAllowed);
            }

            // Validate that endpoints have not changed
            foreach (var currentNodeType in currentConfig.NodeTypes)
            {
                var matchingNodeType =
                    newCsmConfig.NodeTypes.FirstOrDefault(otherNodeType => otherNodeType.Name == currentNodeType.Name);
                if (matchingNodeType == null)
                {
                    if (currentNodeType.IsPrimary)
                    {
                        throw new ValidationException(ClusterManagementErrorCode.PrimaryNodeTypeDeletionNotAllowed, currentNodeType.Name);
                    }

                    continue;
                }

                if (currentNodeType.ClientConnectionEndpointPort != matchingNodeType.ClientConnectionEndpointPort)
                {
                    throw new ValidationException(ClusterManagementErrorCode.UpdateNotAllowed, "ClientConnectionEndpoint");
                }

                if (currentNodeType.HttpGatewayEndpointPort != matchingNodeType.HttpGatewayEndpointPort)
                {
                    throw new ValidationException(ClusterManagementErrorCode.UpdateNotAllowed, "HttpGatewayEndpoint");
                }

                if (currentNodeType.IsPrimary)
                {
                    if (!matchingNodeType.IsPrimary)
                    {
                        throw new ValidationException(ClusterManagementErrorCode.PrimaryNodeTypeModificationNotAllowed, currentNodeType.Name);
                    }
                }
            }

            foreach (var newCsmConfigNodeType in newCsmConfig.NodeTypes)
            {
                var matchingExistingNodeType = currentConfig.NodeTypes.FirstOrDefault(existingNodeType => existingNodeType.Name.Equals(newCsmConfigNodeType.Name, StringComparison.OrdinalIgnoreCase));
                if (matchingExistingNodeType != null && matchingExistingNodeType.VMInstanceCount != newCsmConfigNodeType.VMInstanceCount)
                {
                    throw new ValidationException(ClusterManagementErrorCode.ManualScaleUpOrDownNotAllowed);
                }
            }

            if (currentConfig.PrimaryNodeTypes.Count() == 1 && newCsmConfig.PrimaryNodeTypes.Count() != 1)
            {
                throw new ValidationException(ClusterManagementErrorCode.PrimaryNodeTypeModificationNotAllowed, currentConfig.PrimaryNodeTypes.ElementAt(0).Name);
            }

            if ((currentConfig.Security != null && currentConfig.Security.CertificateInformation != null) && (newCsmConfig.Security == null || newCsmConfig.Security.CertificateInformation == null))
            {
                throw new ValidationException(ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedByRemovingCertificate);
            }

            if ((currentConfig.Security == null || currentConfig.Security.CertificateInformation == null) && (newCsmConfig.Security != null && newCsmConfig.Security.CertificateInformation != null))
            {
                throw new ValidationException(ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedByAddingCertificate);
            }

            if ((currentConfig.Security != null && currentConfig.Security.WindowsIdentities != null) && (newCsmConfig.Security == null || newCsmConfig.Security.WindowsIdentities == null))
            {
                throw new ValidationException(ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedFromWindowsToUnsecured);
            }

            if ((currentConfig.Security == null || currentConfig.Security.WindowsIdentities == null) && (newCsmConfig.Security != null && newCsmConfig.Security.WindowsIdentities != null))
            {
                throw new ValidationException(ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedFromUnsecuredToWindows);
            }

            if ((currentConfig.Security != null && currentConfig.Security.CertificateInformation != null)
                || (newCsmConfig.Security != null && newCsmConfig.Security.CertificateInformation != null))
            {
                StandaloneSettingsValidator.ValidateClusterCertificateUpdate(
                    currentConfig.Security.CertificateInformation,
                    newCsmConfig.Security.CertificateInformation,
                    this.TraceLogger);
            }

            if (connectedToCluster)
            {
                await this.ValidateTopologyAsync(currentConfig, currentTopology).ConfigureAwait(false);
            }
            else
            {
                this.VerifyNodePropertiesUnchanged(currentTopology.Nodes);
            }

            StandaloneSettingsValidator.ValidateRepairManager(currentConfig, newCsmConfig);

            await this.ValidateCodeVersionAsync(newCsmConfig.CodeVersion);
        }

        public List<NodeDescription> GetAddedNodes(IClusterTopology currentTopology)
        {
            List<NodeDescription> addedNodes = new List<NodeDescription>();
            foreach (var nodeName in this.Topology.Nodes.Keys)
            {
                if (!currentTopology.Nodes.ContainsKey(nodeName))
                {
                    addedNodes.Add(this.Topology.Nodes[nodeName]);
                }
            }

            return addedNodes;
        }

        public List<NodeDescription> GetRemovedNodes(IClusterTopology currentTopology)
        {
            var nodesToBeRemovedSection = this.ClusterProperties.FabricSettings.FirstOrDefault(section => section.Name.Equals(StringConstants.SectionName.Setup, StringComparison.OrdinalIgnoreCase)
                                                                                    && section.Parameters != null
                                                                                    && section.Parameters.Any(param => param.Name.Equals(StringConstants.ParameterName.NodesToBeRemoved, StringComparison.OrdinalIgnoreCase)));

            List<string> nodesToRemove = new List<string>();
            if (nodesToBeRemovedSection != null)
            {
                nodesToRemove = nodesToBeRemovedSection.Parameters.First(parameter => parameter.Name.Equals(StringConstants.ParameterName.NodesToBeRemoved, StringComparison.OrdinalIgnoreCase)).Value.Split(',').Select(p => p.Trim()).ToList();
            }

            List<NodeDescription> result = new List<NodeDescription>();
            foreach (var nodeName in currentTopology.Nodes.Keys)
            {
                if (!this.Topology.Nodes.ContainsKey(nodeName))
                {
                    if (nodesToRemove.Contains(nodeName))
                    {
                        result.Add(currentTopology.Nodes[nodeName]);
                    }
                    else
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFNodesToRemoveNotContainAllRemovedNodes);
                    }
                }
            }

            foreach (var nodeName in nodesToRemove)
            {
                if (!result.Any(removedNode => removedNode.NodeName == nodeName))
                {
                    throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFNodesToRemoveContainsUnknownNodes);
                }
            }

            return result;
        }

        public async Task<bool> IsCodeVersionChangedAsync(string targetCodeVersion)
        {
            if (!string.IsNullOrEmpty(targetCodeVersion))
            {
                var currentCodeVersion = await StandaloneUtility.GetCurrentCodeVersion().ConfigureAwait(false);
                if (!currentCodeVersion.Equals(targetCodeVersion.Trim()))
                {
                    return true;
                }
            }

            return false;
        }

        internal static void ValidateRepairManager(IUserConfig currentCsmConfig, IUserConfig newCsmConfig)
        {
            if (newCsmConfig.AddonFeatures == null)
            {
                newCsmConfig.AddonFeatures = new List<AddonFeature>();
            }

            bool removeRm = currentCsmConfig.AddonFeatures != null
                && currentCsmConfig.AddonFeatures.Except(newCsmConfig.AddonFeatures).Contains(AddonFeature.RepairManager);
            if (removeRm)
            {
                throw new ValidationException(ClusterManagementErrorCode.RemoveRepairManagerUnsupported);
            }
        }

        [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "Words like ip are valid to use.")]
        internal static void ValidateCertificateInstallation(
            X509 certInfo,
            IEnumerable<string> nodeIpAddresses,
            ITraceLogger traceLogger)
        {
            List<Tuple<CertificateDescription, ServerCertificateCommonNames>> certificatesToCheck = new List<Tuple<CertificateDescription, ServerCertificateCommonNames>>()
            {
                new Tuple<CertificateDescription, ServerCertificateCommonNames>(certInfo.ClusterCertificate, certInfo.ClusterCertificateCommonNames),
                new Tuple<CertificateDescription, ServerCertificateCommonNames>(certInfo.ServerCertificate, certInfo.ServerCertificateCommonNames),
                new Tuple<CertificateDescription, ServerCertificateCommonNames>(certInfo.ReverseProxyCertificate, certInfo.ReverseProxyCertificateCommonNames)
            };

            foreach (Tuple<CertificateDescription, ServerCertificateCommonNames> currCert in certificatesToCheck)
            {
                if (currCert.Item1 == null && (currCert.Item2 == null || !currCert.Item2.Any()))
                {
                    continue;
                }

                List<string> thumbprintsOrCns = StandaloneSettingsValidator.GetUniqueThumbprintsOrCommonNames(currCert.Item1, currCert.Item2);
                X509FindType findType = currCert.Item1 != null ? X509FindType.FindByThumbprint : X509FindType.FindBySubjectName;
                StoreName storeName = findType == X509FindType.FindByThumbprint ?
                    currCert.Item1.X509StoreName : currCert.Item2.X509StoreName;

                IEnumerable<X509Certificate2> installedCerts = StandaloneSettingsValidator.ValidateAllCertsInstalledOnAllNodes(
                    thumbprintsOrCns,
                    storeName,
                    findType,
                    nodeIpAddresses,
                    traceLogger);

                IEnumerable<X509Certificate2> invalidCerts = installedCerts.Where(cert => !X509CertificateUtility.IsCertificateValid(cert));
                if (invalidCerts.Any())
                {
                    string invalidCertStrings = string.Join(
                            ",",
                            findType == X509FindType.FindByThumbprint ? invalidCerts.Select(p => p.Thumbprint) : invalidCerts.Select(p => p.Subject));
                    throw new ValidationException(ClusterManagementErrorCode.CertificateInvalid, invalidCertStrings);
                }
            }
        }

        /// <summary>
        /// validate cluster cert update
        /// </summary>
        /// <param name="currentCertInfo">current cert</param>
        /// <param name="updatedCertInfo">updated cert</param>
        [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "Words like ip are valid to use.")]
        internal static void ValidateClusterCertificateUpdate(
            X509 currentCertInfo,
            X509 updatedCertInfo,
            ITraceLogger traceLogger)
        {
            string currentConfig = StandaloneSettingsValidator.GetClusterCertConfig(currentCertInfo);
            string updatedConfig = StandaloneSettingsValidator.GetClusterCertConfig(updatedCertInfo);
            if (currentConfig == null && updatedConfig == null)
            {
                return;
            }

            traceLogger.WriteInfo(
                StandaloneSettingsValidator.TraceType,
                "validating cluster certs. Original: {0}. Updated: {1}.",
                currentConfig,
                updatedConfig);
            
            StandaloneSettingsValidator.ValidateClusterCertificateIssuerThumbprintUpdate(
                currentCertInfo.ClusterCertificateCommonNames,
                updatedCertInfo.ClusterCertificateCommonNames,
                traceLogger);

            List<string> currentThumbprintsOrCns = StandaloneSettingsValidator.GetClusterCertUniqueThumbprintsOrCommonNames(currentCertInfo);
            List<string> updatedThumbprintsOrCns = StandaloneSettingsValidator.GetClusterCertUniqueThumbprintsOrCommonNames(updatedCertInfo);
            bool isCurrentCertThumbprint = currentCertInfo.ClusterCertificate != null;
            bool isUpdatedCertThumbprint = updatedCertInfo.ClusterCertificate != null;
            if (!StandaloneSettingsValidator.ValidateClusterCertificateThumbprintAndCnUpdate(currentThumbprintsOrCns, updatedThumbprintsOrCns, isCurrentCertThumbprint, isUpdatedCertThumbprint))
            {
                return;
            }

            StandaloneSettingsValidator.ValidateClusterCertificateThumbprintAndCnInstallation(currentCertInfo, updatedCertInfo, traceLogger);
            StandaloneSettingsValidator.ValidateClusterIssuerStoreUpdate(currentCertInfo.ClusterCertificateIssuerStores, updatedCertInfo.ClusterCertificateIssuerStores, traceLogger);
        }

        internal static void ValidateClusterCertificateThumbprintAndCnInstallation(
            X509 currentCertInfo,
            X509 updatedCertInfo,
            ITraceLogger traceLogger)
        {
            List<string> currentThumbprintsOrCns = StandaloneSettingsValidator.GetClusterCertUniqueThumbprintsOrCommonNames(currentCertInfo);
            List<string> updatedThumbprintsOrCns = StandaloneSettingsValidator.GetClusterCertUniqueThumbprintsOrCommonNames(updatedCertInfo);

            X509FindType currentFindType = currentCertInfo.ClusterCertificate != null ? X509FindType.FindByThumbprint : X509FindType.FindBySubjectName;
            StoreName currentStoreName = currentFindType == X509FindType.FindByThumbprint ?
                currentCertInfo.ClusterCertificate.X509StoreName : currentCertInfo.ClusterCertificateCommonNames.X509StoreName;

            // at lease 1 existing certificate should be installed on at least 1 node
            IEnumerable<X509Certificate2> currentCerts = StandaloneSettingsValidator.ValidateAtLeastOneCertInstalledOnCurrentNode(
                    currentThumbprintsOrCns,
                    currentStoreName,
                    currentFindType,
                    traceLogger);

            // at least 1 existing certificate should be valid
            if (currentCerts.All(cert => !X509CertificateUtility.IsCertificateValid(cert)))
            {
                throw new ValidationException(ClusterManagementErrorCode.CertificateInvalid, string.Join(",", currentThumbprintsOrCns));
            }

            IEnumerable<string> unchangedThumbprintsOrCns = currentThumbprintsOrCns.Intersect(updatedThumbprintsOrCns);
            IEnumerable<string> addedThumbprintsOrCns = updatedThumbprintsOrCns.Except(currentThumbprintsOrCns);

            if (unchangedThumbprintsOrCns.Any())
            {
                // every unchanged certificates should be installed on at least 1 node
                IEnumerable<X509Certificate2> unchangedCerts = StandaloneSettingsValidator.ValidateEachCertInstalledOnCurrentNode(
                    unchangedThumbprintsOrCns,
                    currentStoreName,
                    currentFindType,
                    traceLogger);

                // at lease 1 unchanged certificate should be valid 
                if (unchangedCerts.All(cert => !X509CertificateUtility.IsCertificateValid(cert)))
                {
                    throw new ValidationException(ClusterManagementErrorCode.CertificateInvalid, string.Join(",", unchangedThumbprintsOrCns));
                }
            }

            if (addedThumbprintsOrCns.Any())
            {
                X509FindType updatedFindType = updatedCertInfo.ClusterCertificate != null ? X509FindType.FindByThumbprint : X509FindType.FindBySubjectName;
                StoreName updatedStoreName = updatedFindType == X509FindType.FindByThumbprint ?
                    updatedCertInfo.ClusterCertificate.X509StoreName : updatedCertInfo.ClusterCertificateCommonNames.X509StoreName;

                // every added certificates should be installed on at least 1 node
                IEnumerable<X509Certificate2> addedCerts = StandaloneSettingsValidator.ValidateEachCertInstalledOnCurrentNode(
                    addedThumbprintsOrCns,
                    updatedStoreName,
                    updatedFindType,
                    traceLogger);

                // all added certs should be valid
                if (!addedCerts.All(cert => X509CertificateUtility.IsCertificateValid(cert)))
                {
                    throw new ValidationException(ClusterManagementErrorCode.CertificateInvalid, string.Join(",", addedThumbprintsOrCns));
                }
            }
        }

        internal static List<string> GetClusterCertUniqueThumbprintsOrCommonNames(X509 certInfo)
        {
            return GetUniqueThumbprintsOrCommonNames(certInfo.ClusterCertificate, certInfo.ClusterCertificateCommonNames);
        }

        internal static List<string> GetUniqueThumbprintsOrCommonNames(CertificateDescription certTPDescription, ServerCertificateCommonNames certCNDescription)
        {
            List<string> thumbprints = StandaloneSettingsValidator.GetUniqueThumbprints(certTPDescription);
            List<string> cns = StandaloneSettingsValidator.GetCommonNames(certCNDescription)
                .Select(p => p.CertificateCommonName).ToList();
            return thumbprints.Any() ? thumbprints : cns;
        }

        internal static string GetClusterCertConfig(X509 certInfo)
        {
            if (certInfo.ClusterCertificate == null
                && (certInfo.ClusterCertificateCommonNames == null
                    || !certInfo.ClusterCertificateCommonNames.Any()))
            {
                return null;
            }

            if (certInfo.ClusterCertificate != null)
            {
                return string.Format(
                    "primary thumbprint: {0}. secondary thumbprint: {1}",
                    certInfo.ClusterCertificate.Thumbprint,
                    certInfo.ClusterCertificate.ThumbprintSecondary);
            }
            else
            {
                IEnumerable<string> pairs = certInfo.ClusterCertificateCommonNames.CommonNames.Select(
                            cn => string.Format("{0}:{1}", cn.CertificateCommonName, cn.CertificateIssuerThumbprint));
                return string.Format(
                    "CNs and IssuerThumbprints: {0}",
                    string.Join(";", pairs));
            }
        }

        internal static void ValidateClusterCertificateIssuerThumbprintUpdate(
            ServerCertificateCommonNames currentClusterCns,
            ServerCertificateCommonNames updatedClusterCns,
            ITraceLogger traceLogger)
        {
            List<CertificateCommonNameBase> currentCns = StandaloneSettingsValidator.GetCommonNames(currentClusterCns);
            List<CertificateCommonNameBase> updatedCns = StandaloneSettingsValidator.GetCommonNames(updatedClusterCns);
            if (!currentCns.Any() && !updatedCns.Any())
            {
                return;
            }

            foreach (CertificateCommonNameBase currentCn in currentCns)
            {
                foreach (CertificateCommonNameBase updatedCn in updatedCns)
                {
                    if (currentCn.CertificateCommonName == updatedCn.CertificateCommonName
                        && !string.IsNullOrWhiteSpace(currentCn.CertificateIssuerThumbprint)
                        && !string.IsNullOrWhiteSpace(updatedCn.CertificateIssuerThumbprint))
                    {
                        string[] currentThumbprints = currentCn.CertificateIssuerThumbprint.Split(',');
                        string[] updatedThumbprints = updatedCn.CertificateIssuerThumbprint.Split(',');

                        // IT upgrade must have intersection
                        IEnumerable<string> unchangedThumbprints = currentThumbprints.Intersect(updatedThumbprints, StringComparer.OrdinalIgnoreCase);
                        if (!unchangedThumbprints.Any())
                        {
                            throw new ValidationException(ClusterManagementErrorCode.IssuerThumbprintUpgradeWithNoIntersection);
                        }

                        IEnumerable<string> addedThumbprints = updatedThumbprints.Except(currentThumbprints, StringComparer.OrdinalIgnoreCase);
                        if (addedThumbprints.Any())
                        {
                            StandaloneSettingsValidator.ValidateNoCertInstalledOnCurrentNode(
                                updatedCn.CertificateCommonName,
                                addedThumbprints,
                                updatedClusterCns.X509StoreName,
                                traceLogger);
                        }

                        IEnumerable<string> removedThumbprints = currentThumbprints.Except(updatedThumbprints, StringComparer.OrdinalIgnoreCase);
                        if (removedThumbprints.Any())
                        {
                            StandaloneSettingsValidator.ValidateNoCertInstalledOnCurrentNode(
                                currentCn.CertificateCommonName,
                                removedThumbprints,
                                currentClusterCns.X509StoreName,
                                traceLogger);
                        }
                    }
                }
            }
        }

        internal static bool ValidateClusterCertificateThumbprintAndCnUpdate(
            List<string> currentValues,
            List<string> updatedValues,
            bool isCurrentCertThumbprint,
            bool isUpdatedCertThumbprint)
        {
            if (currentValues.Count == updatedValues.Count)
            {
                bool valueChanged = false;

                for (int i = 0; i < currentValues.Count; i++)
                {
                    valueChanged = !string.Equals(
                        currentValues[i],
                        updatedValues[i],
                        isCurrentCertThumbprint ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
                    if (valueChanged)
                    {
                        break;
                    }
                }

                if (!valueChanged)
                {
                    return false;
                }
            }

            bool isCertTypechanged = isCurrentCertThumbprint != isUpdatedCertThumbprint;

            // Block upgrading from 2 different certs to 2 different certs. Even swap is not allowed.
            if (currentValues.Count() > 1 && updatedValues.Count() > 1 && !isCertTypechanged)
            {
                throw new ValidationException(ClusterManagementErrorCode.TwoCertificatesToTwoCertificatesNotAllowed);
            }

            // If there's no value intersection before and after the upgrade, only 1 cert -> 1 cert is allowed.
            IEnumerable<string> unchangedValues = currentValues.Intersect(updatedValues, isCurrentCertThumbprint ? StringComparer.OrdinalIgnoreCase : StringComparer.Ordinal);
            if (!unchangedValues.Any()
                && !(currentValues.Count() == 1 && updatedValues.Count() == 1) && !isCertTypechanged)
            {
                throw new ValidationException(ClusterManagementErrorCode.CertificateUpgradeWithNoIntersectionNotAllowed);
            }

            return true;
        }

        internal static void ValidateClusterIssuerStoreUpdate(
            List<CertificateIssuerStore> currentClusterIssuerStores,
            List<CertificateIssuerStore> updatedClusterIssuerStores,
            ITraceLogger traceLogger)
        {
            List<string> currentCns = StandaloneSettingsValidator.GetCommonNames(currentClusterIssuerStores);
            List<string> updatedCns = StandaloneSettingsValidator.GetCommonNames(updatedClusterIssuerStores);
            if (!currentCns.Any() && !updatedCns.Any())
            {
                return;
            }

            // Cluster issuer CN upgrade must have intersection
            if (currentCns.Any())
            {
                IEnumerable<string> unchangedCns = currentCns.Intersect(updatedCns);
                if (!unchangedCns.Any())
                {
                    throw new ValidationException(ClusterManagementErrorCode.IssuerStoreCNUpgradeWithNoIntersection);
                }
            }
            
            foreach (var currentCn in currentCns)
            {
                foreach (var updatedCn in updatedCns)
                {
                    if (currentCn == updatedCn)
                    {
                        var currentX509Stores = currentClusterIssuerStores.Find(p => p.IssuerCommonName == currentCn).X509StoreNames.Split(',').ToList().Select(p => p.Trim());
                        var updatedX509Stores = updatedClusterIssuerStores.Find(p => p.IssuerCommonName == updatedCn).X509StoreNames.Split(',').ToList().Select(p => p.Trim());

                        // X509Stores upgrade must have intersection
                        IEnumerable<string> unchangedX509Stores = currentX509Stores.Intersect(updatedX509Stores);
                        if (!unchangedX509Stores.Any())
                        {
                            throw new ValidationException(ClusterManagementErrorCode.IssuerStoreX509StoreNameUpgradeWithNoIntersection);
                        }
                    }
                }
            }
        }

        [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly", Justification = "Words like ip are valid to use.")]
        internal static IEnumerable<X509Certificate2> ValidateAllCertsInstalledOnAllNodes(
            IEnumerable<string> thumbprintsOrCns,
            StoreName storeName,
            X509FindType findType,
            IEnumerable<string> nodeIpAddresses,
            ITraceLogger traceLogger)
        {
            IEnumerable<X509Certificate2> result = Enumerable.Empty<X509Certificate2>();

            foreach (string address in nodeIpAddresses)
            {
                IEnumerable<string> uninstalledCerts = X509CertificateUtility.TryFindCertificate(
                    address,
                    storeName,
                    thumbprintsOrCns,
                    findType,
                    out result,
                    traceLogger);

                if (uninstalledCerts.Any())
                {
                    throw new ValidationException(
                        ClusterManagementErrorCode.CertificateNotInstalledOnNode,
                        string.Join(",", uninstalledCerts),
                        address,
                        storeName);
                }
            }

            return result;
        }

        internal static void ValidateNoCertInstalledOnCurrentNode(
            string cn,
            IEnumerable<string> issuerThumbprints,
            StoreName storeName,
            ITraceLogger traceLogger)
        {
            X509Store store = null;

            try
            {
                store = new X509Store(storeName.ToString(), StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
                X509Certificate2Collection certs = store.Certificates.Find(X509FindType.FindBySubjectName, cn, validOnly: false);
                if (certs != null)
                {
                    foreach (X509Certificate2 cert in certs)
                    {
                        string installedIssuerThumbprint = cert.FindIssuer(issuerThumbprints);
                        if (installedIssuerThumbprint != null)
                        {
                            throw new ValidationException(
                                ClusterManagementErrorCode.CertIssuedByIssuerInstalled,
                                cn,
                                installedIssuerThumbprint);
                        }
                    }
                }
            }
            finally
            {
                // In .net 4.6, X509Store is changed to implement IDisposable.
                // But unfortunately, SF today is built on .net 4.5
                if (store != null)
                {
                    store.Close();
                }
            }
        }

        /// <summary>
        /// validate if each certificate is installed on the current node
        /// </summary>
        /// <param name="thumbprints">thumbprints of certificates</param>
        /// <param name="storeName">certificate store name</param>
        /// <returns>installed certificates</returns>
        internal static IEnumerable<X509Certificate2> ValidateEachCertInstalledOnCurrentNode(
            IEnumerable<string> findValues,
            StoreName storeName,
            X509FindType findType,
            ITraceLogger traceLogger)
        {
            IEnumerable<X509Certificate2> result;
            IEnumerable<string> uninstalledCerts = X509CertificateUtility.TryFindCertificate(
                storeName,
                findValues,
                findType,
                out result,
                traceLogger);

            if (uninstalledCerts.Any())
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.CertificateNotInstalled,
                    string.Join(",", uninstalledCerts),
                    storeName);
            }

            return result;
        }

        /// <summary>
        /// validate if at least 1 certificate is installed on the current node
        /// </summary>
        /// <param name="findValues">thumbprints or common names of certificates</param>
        /// <param name="storeName">certificate store name</param>
        /// <returns>installed certificates</returns>
        internal static IEnumerable<X509Certificate2> ValidateAtLeastOneCertInstalledOnCurrentNode(
            IEnumerable<string> findValues,
            StoreName storeName,
            X509FindType findType,
            ITraceLogger traceLogger)
        {
            IEnumerable<X509Certificate2> result;
            IEnumerable<string> uninstalledCerts = X509CertificateUtility.TryFindCertificate(
                storeName,
                findValues,
                findType,
                out result,
                traceLogger);

            if (!result.Any())
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.CertificateNotInstalled,
                    string.Join(",", uninstalledCerts),
                    storeName);
            }

            return result;
        }

        /// <summary>
        /// get primary and secondary thumbprints from a certificate description
        /// </summary>
        /// <param name="certDescription">certificate description</param>
        /// <returns>unique thumbprints</returns>
        internal static List<string> GetUniqueThumbprints(CertificateDescription certDescription)
        {
            HashSet<string> result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (certDescription != null && !string.IsNullOrWhiteSpace(certDescription.Thumbprint))
            {
                result.Add(certDescription.Thumbprint);

                if (!string.IsNullOrWhiteSpace(certDescription.ThumbprintSecondary))
                {
                    result.Add(certDescription.ThumbprintSecondary);
                }
            }

            return result.ToList();
        }

        internal static List<CertificateCommonNameBase> GetCommonNames(ServerCertificateCommonNames commonNames)
        {
            List<CertificateCommonNameBase> result = new List<CertificateCommonNameBase>();
            if (commonNames != null && commonNames.Any())
            {
                return result.Concat(commonNames.CommonNames).ToList();
            }

            return result;
        }

        internal static List<string> GetCommonNames(List<CertificateIssuerStore> issuerStores)
        {
            List<string> result = new List<string>();
            if (issuerStores != null && issuerStores.Any())
            {
                result = issuerStores.Select(p => p.IssuerCommonName).ToList();
            }

            return result;
        }

        internal void ValidateTopology()
        {
            HashSet<string> fdset = new HashSet<string>();
            HashSet<string> udset = new HashSet<string>();

            foreach (var node in this.Topology.Nodes)
            {
                if (!StandaloneUtility.IsValidFileName(node.Key))
                {
                    throw new ValidationException(ClusterManagementErrorCode.NodeNameContainsInvalidChars, node.Key);
                }

                if (!StandaloneUtility.IsValidIPAddressOrFQDN(node.Value.IPAddress))
                {
                    throw new ValidationException(ClusterManagementErrorCode.MalformedIPAddress, node.Value.IPAddress, node.Key);
                }

                fdset.Add(node.Value.FaultDomain);
                udset.Add(node.Value.UpgradeDomain);
            }

            if (!this.inputConfig.IsTestCluster())
            {
                // Block Scale-min for multi-box cluster for non test clusters.
                if (this.Topology.Machines.Count() > 1 && this.Topology.Machines.Count() < this.ClusterProperties.TotalNodeCount)
                {
                    throw new ValidationException(ClusterManagementErrorCode.ScaleMinNotAllowed);
                }
            }

            // FD count needs to be greater than 2 for multi-box deployment.
            if (this.ClusterProperties.TotalNodeCount > 2 && fdset.Count() <= 2)
            {
                throw new ValidationException(ClusterManagementErrorCode.FDMustBeGreaterThan2, fdset.Count());
            }

            // UD count needs to be greater than 2 for multi-box deployment.
            if (this.ClusterProperties.TotalNodeCount > 2 && udset.Count() <= 2)
            {
                throw new ValidationException(ClusterManagementErrorCode.UDMustBeGreaterThan2, udset.Count());
            }
        }

        internal void ValidateSecurity(bool validateDownNodes)
        {
            Security security = this.ClusterProperties.Security;
            if (security != null)
            {
                // Windows security
                if (security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.Windows)
                {
                    if (security.WindowsIdentities == null)
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAWindowsSecurityNeedsIdentities);
                    }

                    if (security.WindowsIdentities.ClustergMSAIdentity != null && security.WindowsIdentities.ClusterSPN == null)
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAWindowsSecurityNoSpnFoundWithgMSAIdentity);
                    }

                    // TODO: (maburlik) check for typos i.e. WindowsIdentities section validates against nodes.
                }
                else if (security.ClusterCredentialType == Microsoft.ServiceFabric.ClusterManagementCommon.CredentialType.None)
                {
                    // gMSA security will take affect only if ClusterCredentialType is set to Windows
                    if (security.WindowsIdentities != null && security.WindowsIdentities.ClustergMSAIdentity != null)
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_BPAWindowsGMSAWithNoneSecurity);
                    }
                }
            }

            if (validateDownNodes)
            {
                if (security != null && security.CertificateInformation != null)
                {
                    StandaloneSettingsValidator.ValidateCertificateInstallation(
                        security.CertificateInformation,
                        this.Topology.Nodes.Values.Select(node => node.IPAddress),
                        this.TraceLogger);
                }
            }
        }

        internal void ValidateDiagnosticStore()
        {
            // Emit warnning if using local folder for 'FileShare' in multi-machine cluster
            if (this.Topology.Machines.Count() > 1 && this.ClusterProperties.DiagnosticsStoreInformation != null && this.ClusterProperties.DiagnosticsStoreInformation.StoreType == DiagnosticStoreType.FileShare)
            {
                if (!new Uri(this.ClusterProperties.DiagnosticsStoreInformation.Connectionstring).IsUnc)
                {
                    SFDeployerTrace.WriteWarning(StringResources.MultiMachineClusterDCALocalFileWarning);
                }
            }
        }

        private static void PullRequiredWrpSettings(out SettingsType wrpSettings, out IDictionary<string, HashSet<string>> requiredParameters)
        {
            string wrpSettingsPath = StandaloneUtility.FindFabricResourceFile(DMConstants.WrpSettingsFilename);
            wrpSettings = System.Fabric.Interop.Utility.ReadXml<SettingsType>(wrpSettingsPath);
            requiredParameters = new Dictionary<string, HashSet<string>>();

            for (var i = 0; i < wrpSettings.Section.Count(); i++)
            {
                if (wrpSettings.Section[i].Parameter == null)
                {
                    continue;
                }

                if (wrpSettings.Section[i].Name.Equals("RequiredParameters"))
                {
                    for (var k = 0; k < wrpSettings.Section[i].Parameter.Count(); k++)
                    {
                        string[] values = wrpSettings.Section[i].Parameter[k].Value.Split(',');
                        HashSet<string> hashset = new HashSet<string>(values);
                        requiredParameters.Add(wrpSettings.Section[i].Parameter[k].Name, hashset);
                    }
                }
            }
        }

        private void VerifyFMNodesMatchTargetNodes(NodeList nodesFromFM)
        {
            if (this.Topology.Nodes.Count != nodesFromFM.Count)
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFNodesFromFMDontMatch);
            }

            foreach (var node in this.Topology.Nodes)
            {
                if (!nodesFromFM.Any(n => n.NodeName == node.Key))
                {
                    throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFAddNodeNotKnowntoFM);
                }
            }
        }

        private void VerifyNodePropertiesUnchanged(NodeList nodesFromFM)
        {
            Dictionary<string, NodeDescription> currentNode = new Dictionary<string, NodeDescription>();
            foreach (var node in nodesFromFM)
            {
                currentNode.Add(
                    node.NodeName,
                    new NodeDescription
                    {
                        NodeName = node.NodeName,
                        NodeTypeRef = node.NodeType,
                        IPAddress = node.IpAddressOrFQDN,
                        UpgradeDomain = node.UpgradeDomain,
                        FaultDomain = node.FaultDomain.ToString()
                    });
            }

            this.VerifyNodePropertiesUnchanged(currentNode);
        }

        /** Provides validation on static topology for Test-Configuration. UOS upgrade diff validation will call ValidateTopologyAsync to
            query nodes from FM. Current Test-Configuration doesn't connect to the cluster. So we diff the two clusterConfigs in this method. */
        private void VerifyNodePropertiesUnchanged(Dictionary<string, NodeDescription> currentNodes)
        {
            foreach (var node in currentNodes)
            {
                // If the node names are the same, check if IP address and node type ref are the same.
                if (this.Topology.Nodes.Keys.Contains(node.Key) && (node.Value.IPAddress != this.Topology.Nodes[node.Key].IPAddress || node.Value.NodeTypeRef != this.Topology.Nodes[node.Key].NodeTypeRef))
                {
                    throw new ValidationException(ClusterManagementErrorCode.NodeIPNodeTypeRefCantBeChanged, node.Key);
                }
            }

            // If node IPs are the same, check if node name and node type ref have changed. For scale-min we don't support add or remove node so the checks are valid for scale min as well.
            foreach (var targetNode in this.Topology.Nodes.Values)
            {
                var potentialIPMatch = currentNodes.Values.Where(n => n.IPAddress == targetNode.IPAddress);
                var currentTargetSharedIpNodes = currentNodes.Values.Where(n => n.IPAddress == targetNode.IPAddress);
                if (potentialIPMatch.Count() > 0 &&
                    (!currentTargetSharedIpNodes.Select(n => n.NodeName).Contains(targetNode.NodeName)
                    || !currentTargetSharedIpNodes.Select(n => n.NodeTypeRef).Contains(targetNode.NodeTypeRef)))
                {
                    throw new ValidationException(ClusterManagementErrorCode.NodeNameNodeTypeRefCantBeChanged, targetNode.NodeName);
                }
            }
        }

        private void ValidateAddedNodes(NodeList nodesFromFM, List<NodeDescription> addedNodes, string fabricDataRoot)
        {
            var machineList = addedNodes.Select(node => node.IPAddress);
            MachineHealthContainer machineHealthContainer = new MachineHealthContainer(machineList);
            if (StandaloneUtility.IsMsiInstalled(machineHealthContainer))
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFAddNodeFailedDueToPreviousInstall);
            }

            foreach (var node in addedNodes)
            {
                if (!nodesFromFM.Any(n => n.NodeName == node.NodeName) && !StandaloneUtility.CheckForCleanInstall(node.IPAddress, node.NodeName, fabricDataRoot))
                {
                    throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFAddNodeFailedDueToPreviousInstall);
                }
            }
        }

        private async Task<bool> CheckUOSandFMStateConsistentAsync(Dictionary<string, NodeDescription> currentUOSKnownNodes)
        {
            var nodesFromFM = await StandaloneUtility.GetNodesFromFMAsync(new FabricClient(), CancellationToken.None).ConfigureAwait(false);
            foreach (var node in nodesFromFM)
            {
                if (!currentUOSKnownNodes.Keys.Any(nodeName => nodeName == node.NodeName))
                {
                    return false;
                }
            }

            return true;
        }

        private async Task ValidateTopologyAsync(IUserConfig currentConfig, IClusterTopology currentTopology)
        {
            bool isUOSandFMStateConsistent = await this.CheckUOSandFMStateConsistentAsync(currentTopology.Nodes).ConfigureAwait(false);

            NodeList nodesFromFM = await StandaloneUtility.GetNodesFromFMAsync(new FabricClient(), CancellationToken.None).ConfigureAwait(false);
            this.VerifyNodePropertiesUnchanged(nodesFromFM);

            if (!isUOSandFMStateConsistent && !StandaloneUtility.CheckFabricRunningAsGMSA(currentConfig))
            {
                this.VerifyFMNodesMatchTargetNodes(nodesFromFM);
            }
            else
            {
                var addedNodes = this.GetAddedNodes(currentTopology);

                // Todo : Make sure nodesToRemove is in the diff.
                var removedNodes = this.GetRemovedNodes(currentTopology);
                if (removedNodes.Any() && addedNodes.Any())
                {
                    throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFAddAndRemoveNodeTogether);
                }

                if (removedNodes.Any())
                {
                    if (this.ClusterProperties.ReliabilityLevel != currentConfig.ReliabilityLevel)
                    {
                        foreach (var node in removedNodes)
                        {
                            Node nodeFromFM = nodesFromFM.SingleOrDefault(n => n.NodeName == node.NodeName);
                            if (nodeFromFM != null && !nodeFromFM.IsSeedNode)
                            {
                                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFRemoveNonSeedAndReliabilityChangeInvalid);
                            }
                        }
                    }

                    if (this.ClusterProperties.IsScaleMin)
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFRemoveNodeInvalidScaleMin);
                    }

                    var machineList = removedNodes.Select(node => node.IPAddress);
                    MachineHealthContainer machineHealthContainer = new MachineHealthContainer(machineList);
                    if (!StandaloneUtility.CheckRequiredPorts(machineHealthContainer))
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFPortsUnreachable);
                    }
                }

                if (addedNodes.Any())
                {
                    if (StandaloneUtility.CheckFabricRunningAsGMSA(currentConfig))
                    {
                        var setupSection = this.ClusterProperties.FabricSettings.FirstOrDefault(config => config.Name.Equals("Setup")).Parameters;
                        string fabricDataRoot = setupSection.FirstOrDefault(parameter => parameter.Name.Equals("FabricDataRoot")).Value;
                        this.ValidateAddedNodes(nodesFromFM, addedNodes, fabricDataRoot);
                    }
                    else
                    {
                        throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFAddNodeNotKnowntoFM);
                    }
                }
            }
        }

        private async Task ValidateCodeVersionAsync(string targetCodeVersion)
        {
            bool codeVersionChanged = await this.IsCodeVersionChangedAsync(targetCodeVersion).ConfigureAwait(false);

            if (!codeVersionChanged)
            {
                return;
            }

            // Executed only if code version has been modified.
            bool autoUpgradeEnabled = this.ClusterProperties.AutoupgradeEnabled;
            if (autoUpgradeEnabled && targetCodeVersion != DMConstants.AutoupgradeCodeVersion)
            {
                throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFCodeVersionChangedForAutoUpgrades);
            }
            else
            {
                var targetPackage = await StandaloneUtility.ValidateCodeVersionAsync(targetCodeVersion);
                if (targetPackage == null)
                {
                    throw new ValidationException(ClusterManagementErrorCode.BestPracticesAnalyzerModelInvalid, StringResources.Error_SFCodePackagesUnreachable);
                }
            }
        }
    }
}