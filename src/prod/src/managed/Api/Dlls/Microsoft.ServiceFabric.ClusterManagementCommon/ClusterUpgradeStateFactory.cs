// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using Newtonsoft.Json;

    internal class ClusterUpgradeStateFactory
    {
        private static readonly TraceType TraceType = new TraceType("ClusterUpgradeStateFactory");

        [JsonProperty(IsReference = true)]
        private ICluster clusterResource;

        [JsonProperty]
        private IUpgradeStateActivator upgradeStateActivator;

        private ITraceLogger traceLogger;

        [JsonConstructor]
        public ClusterUpgradeStateFactory(
            ICluster clusterResource,
            IUpgradeStateActivator upgradeStateActivator)
        {
            this.clusterResource = clusterResource;
            this.upgradeStateActivator = upgradeStateActivator;
        }

        public ClusterUpgradeStateFactory(
            ICluster clusterResource,
            IUpgradeStateActivator upgradeStateActivator,
            ITraceLogger traceLogger)
        {
            this.clusterResource = clusterResource;
            this.upgradeStateActivator = upgradeStateActivator;
            this.traceLogger = traceLogger;
        }

        public ClusterUpgradeStateBase CreateGatekeepingUpgradeState()
        {
            var clusterUpgradeState = this.upgradeStateActivator.CreateGatekeepingClusterUpgradeState(
                                          this.clusterResource.Current.CSMConfig,
                                          this.clusterResource.Current.WRPConfig,
                                          this.clusterResource.Current.NodeConfig,
                                          this.clusterResource,
                                          this.traceLogger);

            bool processingSuccess = clusterUpgradeState.StartProcessing();

            ReleaseAssert.AssertIfNot(
                processingSuccess,
                "GatekeepingUpgradeState should successfully start processing upgrade.");

            return clusterUpgradeState;
        }

        public bool TryCreateSeedNodeUpgradeState(
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            List<string> disabledNodes,
            List<string> removedNodes,
            out MultiphaseClusterUpgradeState seedNodeUpgradeState)
        {
            seedNodeUpgradeState = this.upgradeStateActivator.CreateSeedNodeUpgradeStateBase(
                                       disabledNodes,
                                       removedNodes,
                                       csmConfig,
                                       wrpConfig,
                                       nodeConfig,
                                       this.clusterResource,
                                       this.traceLogger);

            seedNodeUpgradeState.ValidateSettingChanges();
            return seedNodeUpgradeState.StartProcessing();
        }

        public bool TryCreateUpgradeState(
            IUserConfig csmConfig,
            IAdminConfig wrpConfig,
            ClusterNodeConfig nodeConfig,
            out ClusterUpgradeStateBase clusterUpgradeState)
        {
            csmConfig.MustNotBeNull("csmConfig");
            wrpConfig.MustNotBeNull("wrpConfig");
            nodeConfig.MustNotBeNull("nodeConfig");

            bool hasReliabilityScaledUp = false;
            clusterUpgradeState = null;

            if (this.clusterResource.Current == null)
            {
                clusterUpgradeState = this.upgradeStateActivator.CreateBaselineUpgradeState(
                                          csmConfig,
                                          wrpConfig,
                                          nodeConfig,
                                          this.clusterResource,
                                          this.traceLogger);
            }
            else
            {
                HashSet<string> certsAdded, certsRemoved;
                bool hasCertChanged = HasCertficateChanged(
                                          csmConfig,
                                          this.clusterResource.Current.CSMConfig,
                                          out certsAdded,
                                          out certsRemoved);

                hasReliabilityScaledUp = csmConfig.ReliabilityLevel > this.clusterResource.Current.CSMConfig.ReliabilityLevel;
                var hasReliabilityScaledDown = csmConfig.ReliabilityLevel < this.clusterResource.Current.CSMConfig.ReliabilityLevel;

                bool hasNodeStatusChanged = nodeConfig.Version != this.clusterResource.Current.NodeConfig.Version;

                if (hasCertChanged && (hasReliabilityScaledUp || hasReliabilityScaledDown))
                {
                    throw new ClusterManagementException(ClusterManagementErrorCode.CertificateAndScaleUpgradeTogetherNotAllowed);
                }

                if (hasCertChanged && hasNodeStatusChanged)
                {
                    throw new ClusterManagementException(ClusterManagementErrorCode.CertificateAndScaleUpgradeTogetherNotAllowed);
                }

                if (hasNodeStatusChanged && this.clusterResource.NodeTypeNodeStatusList == null)
                {
                    throw new ClusterManagementException(ClusterManagementErrorCode.ScaleUpAndScaleDownUpgradeNotAllowedForOlderClusters);
                }

                if (hasCertChanged)
                {
                    clusterUpgradeState = this.upgradeStateActivator.CreateCertificateClusterUpgradeState(
                                              csmConfig,
                                              wrpConfig,
                                              nodeConfig,
                                              this.clusterResource,
                                              this.traceLogger,
                                              certsAdded,
                                              certsRemoved);
                }
                else if (hasReliabilityScaledDown || hasReliabilityScaledUp)
                {
                    clusterUpgradeState = this.upgradeStateActivator.CreateAutoScaleClusterUpgradeStateBase(
                                              true /*initiatedByCsmRequest*/,
                                              csmConfig,
                                              wrpConfig,
                                              nodeConfig,
                                              this.clusterResource,
                                              this.traceLogger);
                }
                else if (hasNodeStatusChanged)
                {
                    clusterUpgradeState = this.upgradeStateActivator.CreateAutoScaleClusterUpgradeStateBase(
                                              false /*initiatedByCsmRequest*/,
                                              csmConfig,
                                              wrpConfig,
                                              nodeConfig,
                                              this.clusterResource,
                                              this.traceLogger);
                }
                else
                {
                    clusterUpgradeState = this.upgradeStateActivator.CreateSimpleUpgradeState(
                                              csmConfig,
                                              wrpConfig,
                                              nodeConfig,
                                              this.clusterResource,
                                              this.traceLogger);
                }
            }

            clusterUpgradeState.ValidateSettingChanges();
            bool processingSuccess = clusterUpgradeState.StartProcessing();

            if (!processingSuccess && hasReliabilityScaledUp)
            {
                //// If scaleup auto upgrade did not go through (only case now is enough nodes being available),
                //// then accept the request and complete the upgrade to ARM with success

                this.traceLogger.WriteInfo(TraceType, "TryCreateUpgradeState: StartProcessing returns false, so create SimpleUpgrade");
                clusterUpgradeState = this.upgradeStateActivator.CreateSimpleUpgradeState(
                                          csmConfig,
                                          wrpConfig,
                                          nodeConfig,
                                          this.clusterResource,
                                          this.traceLogger);

                clusterUpgradeState.ValidateSettingChanges();
                return clusterUpgradeState.StartProcessing();
            }

            this.traceLogger.WriteInfo(TraceType, "TryCreateUpgradeState: {0} is created.", clusterUpgradeState.GetType().Name);
            return processingSuccess;
        }

        private static bool HasCertficateChanged(
            IUserConfig csmConfig,
            IUserConfig currentCsmConfig,
            out HashSet<string> newCerts,
            out HashSet<string> removedCerts)
        {
            HashSet<string> existingCerts = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            if (currentCsmConfig.Security != null &&
                currentCsmConfig.Security.CertificateInformation != null &&
                currentCsmConfig.Security.CertificateInformation.ClusterCertificate != null)
            {
                existingCerts.Add(currentCsmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint);

                if (!string.IsNullOrEmpty(currentCsmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary))
                {
                    existingCerts.Add(currentCsmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary);
                }
            }

            removedCerts = new HashSet<string>(existingCerts, StringComparer.OrdinalIgnoreCase);
            newCerts = new HashSet<string>();
            if (csmConfig.Security != null &&
                csmConfig.Security.CertificateInformation != null &&
                csmConfig.Security.CertificateInformation.ClusterCertificate != null)
            {
                if (removedCerts.Contains(csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint))
                {
                    removedCerts.Remove(csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint);
                }
                else
                {
                    newCerts.Add(csmConfig.Security.CertificateInformation.ClusterCertificate.Thumbprint);
                }

                if (!string.IsNullOrEmpty(csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary))
                {
                    if (removedCerts.Contains(csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary))
                    {
                        removedCerts.Remove(csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary);
                    }
                    else
                    {
                        newCerts.Add(csmConfig.Security.CertificateInformation.ClusterCertificate.ThumbprintSecondary);
                    }
                }
            }

            // Remove certificates which are in the new config from existingCertThumbprint. The remaing thumbprints have
            // been removed
            bool certificateRemoved = removedCerts.Count > 0;
            bool certificateAdded = newCerts.Count > 0;

            bool result = certificateRemoved || certificateAdded;
            if (!result)
            {
                existingCerts.Clear();
                if (currentCsmConfig.Security != null &&
                    currentCsmConfig.Security.CertificateInformation != null &&
                    currentCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames != null)
                {
                    existingCerts = new HashSet<string>(currentCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames.Select(p => p.CertificateCommonName));                   
                }

                HashSet<string> targetCerts = new HashSet<string>();
                if (csmConfig.Security != null &&
                    csmConfig.Security.CertificateInformation != null &&
                    csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames != null)
                {
                    targetCerts = new HashSet<string>(csmConfig.Security.CertificateInformation.ClusterCertificateCommonNames.CommonNames.Select(p => p.CertificateCommonName));
                }

                removedCerts = new HashSet<string>(existingCerts.Except(targetCerts));
                newCerts = new HashSet<string>(targetCerts.Except(existingCerts));

                result = removedCerts.Any() || newCerts.Any();
            }

            return result;
        }
    }
}