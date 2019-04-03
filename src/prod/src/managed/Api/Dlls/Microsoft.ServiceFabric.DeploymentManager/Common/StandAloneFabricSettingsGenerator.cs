// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;
    using System.Text;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    using DMConstants = ServiceFabric.DeploymentManager.Constants;

    internal class StandAloneFabricSettingsGenerator : FabricSettingsGeneratorBase
    {
        private string currentPrimaryAccountNtlmPassword;
        private string currentSecondaryAccountNtlmPassword;
        private string currentCommonNameNtlmPassword;

        public StandAloneFabricSettingsGenerator(
            string clusterIdOrName,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger,
            string currentPrimaryAccountNtlmPassword,
            string currentSecondaryAccountNtlmPassword,
            string currentCommonNameNtlmPassword)
            : base(clusterIdOrName, targetCsmConfig, targetWrpConfig, settingsMetadata, existingClusterManifest, clusterManifestGeneratorSettings, traceLogger)
        {
            this.currentPrimaryAccountNtlmPassword = currentPrimaryAccountNtlmPassword;
            this.currentSecondaryAccountNtlmPassword = currentSecondaryAccountNtlmPassword;
            this.currentCommonNameNtlmPassword = currentCommonNameNtlmPassword;
        }

        protected static List<SettingsTypeSection> GenerateSecretStoreSections(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            if (!targetCsmConfig.AddonFeatures.Contains(AddonFeature.SecretStoreService))
            {
                return result;
            }

            result.Add(
                new SettingsTypeSection
                {
                    Name = StringConstants.SectionName.CentralSecretService,
                    Parameter = StandAloneFabricSettingsGenerator.GenerateStatefulSvcReplicaParameters(targetCsmConfig),
                });

            return result;
        }

        protected static List<SettingsTypeSection> GenerateRepairManagerSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.RepairManager);
            if (intendToCreate)
            {
                List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>()
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.EnableHealthChecks,
                        Value = true.ToString()
                    }
                };
                parameterList.AddRange(StandAloneFabricSettingsGenerator.GenerateStatefulSvcReplicaParameters(targetCsmConfig));

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.RepairManager,
                        Parameter = parameterList.ToArray(),
                    });
            }

            return result;
        }

        protected static List<SettingsTypeSection> GenerateDnsServiceSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.DnsService);
            if (intendToCreate)
            {
                List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>()
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString(),
                    }
                };

                int instanceCount = targetCsmConfig.IsScaleMin ? 1 : -1;
                parameterList.AddRange(StandAloneFabricSettingsGenerator.GenerateStatelessSvcInstanceParameters(instanceCount));

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.DnsService,
                        Parameter = parameterList.ToArray(),
                    });
            }

            return result;
        }

        protected static List<SettingsTypeSection> GenerateSFVolumeDiskServiceSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.SFVolumeDiskService);
            if (intendToCreate)
            {
                List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>()
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsSFVolumeDiskServiceEnabled,
                        Value = true.ToString(),
                    }
                };

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.Hosting,
                        Parameter = parameterList.ToArray(),
                    });
            }

            return result;
        }

        protected static List<SettingsTypeSection> GenerateResourceMonitorServiceSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.ResourceMonitorService);
            if (intendToCreate)
            {
                List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>()
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString(),
                    }
                };

                int instanceCount = -1;
                parameterList.AddRange(StandAloneFabricSettingsGenerator.GenerateStatelessSvcInstanceParameters(instanceCount));

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.ResourceMonitorService,
                        Parameter = parameterList.ToArray(),
                    });
            }

            return result;
        }

        protected static List<SettingsTypeSection> GenerateBackupRestoreServiceSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.BackupRestoreService);
            if (intendToCreate)
            {
                var parameterListArray = StandAloneFabricSettingsGenerator.GenerateStatefulSvcReplicaParameters(targetCsmConfig);

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.BackupRestoreService,
                        Parameter = parameterListArray,
                    });
            }

            return result;
        }

        protected static List<SettingsTypeSection> GenerateEventStoreServiceSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.EventStoreService);
            if (intendToCreate)
            {
                var parameterListArray = StandAloneFabricSettingsGenerator.GenerateStatefulSvcReplicaParameters(targetCsmConfig);

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.EventStoreService,
                        Parameter = parameterListArray,
                    });
            }

            return result;
        }

        protected static List<SettingsTypeSection> GenerateGatewayResourceManagerSection(IUserConfig targetCsmConfig)
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();

            bool intendToCreate = targetCsmConfig.AddonFeatures.Contains(AddonFeature.GatewayResourceManager);
            if (intendToCreate)
            {
                List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>()
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.WindowsProxyImageName,
                        Value = "seabreeze/service-fabric-reverse-proxy:windows-0.20.0",
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.LinuxProxyImageName,
                        Value = "microsoft/service-fabric-reverse-proxy:0.20.0",
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProxyReplicaCount,
                        Value = "1",
                    },
                };

                parameterList.AddRange(StandAloneFabricSettingsGenerator.GenerateStatefulSvcReplicaParameters(targetCsmConfig));

                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.GatewayResourceManager,
                        Parameter = parameterList.ToArray(),
                    });
            }

            return result;
        }

        protected static SettingsTypeSectionParameter[] GenerateStatefulSvcReplicaParameters(IUserConfig targetCsmConfig)
        {
            var replicaSetSize = targetCsmConfig.ReliabilityLevel.GetReplicaSetSize();
            return new SettingsTypeSectionParameter[]
            {
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.TargetReplicaSetSize,
                    Value = replicaSetSize.TargetReplicaSetSize.ToString()
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.MinReplicaSetSize,
                    Value = replicaSetSize.MinReplicaSetSize.ToString()
                }
            };
        }

        protected static SettingsTypeSectionParameter[] GenerateStatelessSvcInstanceParameters(int instanceCount)
        {
            return new SettingsTypeSectionParameter[]
            {
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.InstanceCount,
                    Value = instanceCount.ToString()
                },
            };
        }

        protected override List<SettingsTypeSection> OnGenerateFabricSettings(
            Security security,
            CertificateDescription thumbprintFileStoreCert,
            List<string> thumbprintClusterCertList,
            ServerCertificateCommonNames commonNameFileStoreCert,
            Dictionary<string, string> commonNameClusterCertList,
            int currentClusterSize = -1)
        {
            var result = new List<SettingsTypeSection>();

            // GenerateSecuritySection always returns a non-null value
            var securitySections = this.GenerateSecuritySection(security, thumbprintClusterCertList, commonNameClusterCertList);
            result.AddRange(securitySections);

            var runAsSection = this.GenerateRunAsSection(security);
            if (runAsSection != null)
            {
                result.Add(runAsSection);
            }

            var httpApplicationGatewaySection = this.GenerateHttpApplicationGatewaySection();
            if (httpApplicationGatewaySection != null)
            {
                result.Add(httpApplicationGatewaySection);
            }

            if (security == null || security.WindowsIdentities == null || security.WindowsIdentities.ClustergMSAIdentity == null)
            {
                var fileStoreSection = this.GenerateFileStoreServiceSection(
                    thumbprintFileStoreCert,
                    commonNameFileStoreCert,
                    this.currentPrimaryAccountNtlmPassword,
                    this.currentSecondaryAccountNtlmPassword,
                    this.currentCommonNameNtlmPassword);

                if (fileStoreSection != null)
                {
                    result.Add(fileStoreSection);
                }
            }

            var failoverManagerSection = currentClusterSize == -1 ? this.GenerateFailoverManagerSection() : this.GenerateFailoverManagerSection(currentClusterSize);
            result.Add(failoverManagerSection);

            if (this.TargetCsmConfig.DiagnosticsStoreInformation != null)
            {
                var dcaSections = this.GenerateDcaSections();
                if (dcaSections != null)
                {
                    result.AddRange(dcaSections);
                }
            }

            result.AddRange(this.GenerateClusterManagerSection());

            var upgradeOrchestrationServiceSection = this.GenerateUpgradeOrchestrationServiceSection();
            result.Add(upgradeOrchestrationServiceSection);

            result.AddRange(this.GenerateCommonSection());

            result.AddRange(StandAloneFabricSettingsGenerator.GenerateSecretStoreSections(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateRepairManagerSection(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateDnsServiceSection(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateBackupRestoreServiceSection(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateResourceMonitorServiceSection(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateSFVolumeDiskServiceSection(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateEventStoreServiceSection(this.TargetCsmConfig));
            result.AddRange(StandAloneFabricSettingsGenerator.GenerateGatewayResourceManagerSection(this.TargetCsmConfig));

            return result;
        }

        protected List<SettingsTypeSection> GenerateClusterManagerSection()
        {
            /* For new clusters "EnableAutomaticBaseline" should be set to true so that baseline upgrade is automatically invoked by CM. UOS does not invoke baseline upgrade.
             * For any subsequent upgrades, we don't need to set this flag. */
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();
            if (this.ExistingClusterManifest == null)
            {
                result.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.ClusterManager,
                    Parameter = new SettingsTypeSectionParameter[1]
                    {
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.EnableAutomaticBaseline, Value = true.ToString()
                        }
                    }
                });
            }

            return result;
        }

        protected bool ShouldEnableUnsupportedPreviewFeatureSupportForCluster()
        {
            bool enableUnsupportedPreviewFeatureSupport = false;
            
            // Add a line item for the various unsupported preview feature that will enable the required mode for the cluster.
            enableUnsupportedPreviewFeatureSupport = enableUnsupportedPreviewFeatureSupport || this.TargetCsmConfig.AddonFeatures.Contains(AddonFeature.SFVolumeDiskService);

            return enableUnsupportedPreviewFeatureSupport;
        }

        protected List<SettingsTypeSection> GenerateCommonSection()
        {
            List<SettingsTypeSection> result = new List<SettingsTypeSection>();
            List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>();

            if (this.ExistingClusterManifest == null
                || this.ExistingClusterManifest.FabricSettings.Any(
                    section => section.Name == StringConstants.SectionName.Common
                    && section.Parameter != null && section.Parameter.Any(
                        parameter => parameter.Name == StringConstants.ParameterName.EnableEndpointV2)))
            {
                parameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.EnableEndpointV2, 
                        Value = true.ToString()
                    });
            }

            // Enable preview feature supported in the cluster if applicable.
            if (this.ShouldEnableUnsupportedPreviewFeatureSupportForCluster())
            {
                parameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.EnableUnsupportedPreviewFeatures,
                        Value = true.ToString()
                    });
            }

            if (parameterList.Count > 0)
            {
                result.Add(
                    new SettingsTypeSection
                    {
                        Name = StringConstants.SectionName.Common,
                        Parameter = parameterList.ToArray()
                    });
            }

            return result;
        }

        protected List<SettingsTypeSection> GenerateSecuritySection(Security security, List<string> thumbprintClusterCertList, Dictionary<string, string> commonNameClusterCertList)
        {
            if (security == null)
            {
                var securityDefaultParameters = new List<SettingsTypeSectionParameter>
                {
                    new SettingsTypeSectionParameter { Name = StringConstants.ParameterName.ClusterCredentialType, Value = CredentialType.None.ToString() },
                    new SettingsTypeSectionParameter { Name = StringConstants.ParameterName.ServerAuthCredentialType, Value = CredentialType.None.ToString() },
                    new SettingsTypeSectionParameter { Name = StringConstants.ParameterName.ClientRoleEnabled, Value = "false" }
                };

                return new List<SettingsTypeSection>()
                {
                    new SettingsTypeSection()
                    {
                        Name = StringConstants.SectionName.Security, Parameter = securityDefaultParameters.ToArray()
                    }
                };
            }

            var result = new List<SettingsTypeSection>();
            var securityParameterList = new List<SettingsTypeSectionParameter>
            {
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.ClientRoleEnabled,
                    Value = "true"
                },
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ClusterCredentialType,
                    Value = security.ClusterCredentialType.ToString()
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.ServerAuthCredentialType,
                    Value = security.ServerCredentialType.ToString()
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.AllowDefaultClient,
                    Value = false.ToString()
                }
            };

            if (security.CertificateInformation != null)
            {
                this.AddCertificateSecuritySections(result, securityParameterList, security, thumbprintClusterCertList, commonNameClusterCertList);
            }

            if (security.WindowsIdentities != null)
            {
                this.AddWindowsSecuritySections(securityParameterList, security);
            }

            var securitySectionToAdd = new SettingsTypeSection()
            {
                Name = StringConstants.SectionName.Security,
                Parameter = securityParameterList.ToArray()
            };

            result.Add(securitySectionToAdd);
            return result;
        }

        protected SettingsTypeSection GenerateRunAsSection(Security security)
        {
            if (security == null || security.WindowsIdentities == null || security.WindowsIdentities.ClustergMSAIdentity == null)
            {
                return null;
            }

            var runAsParameterList = new List<SettingsTypeSectionParameter>
            {
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.RunAsAccountType,
                    Value = StringConstants.ManagedServiceAccount
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.RunAsAccountName,
                    Value = security.WindowsIdentities.ClustergMSAIdentity
                }
            };

            var runAsSection = new SettingsTypeSection()
            {
                Name = StringConstants.SectionName.RunAs,
                Parameter = runAsParameterList.ToArray()
            };

            return runAsSection;
        }

        protected void AddCertificateSecuritySections(
            List<SettingsTypeSection> securitySections,
            List<SettingsTypeSectionParameter> securityParameterList,
            Security security,
            List<string> thumbprintClusterCertList,
            Dictionary<string, string> commonNameClusterCertList)
        {
            // security section: thumbprint based
            var certificateInformation = security.CertificateInformation;
            var serverCertificate = certificateInformation.ServerCertificate;
            string clusterCertificateThumbprint = thumbprintClusterCertList != null && thumbprintClusterCertList.Any() ?
                string.Join(",", thumbprintClusterCertList) : null;
            string serverCertificateThumbprint = null;

            if (serverCertificate != null)
            {
                serverCertificateThumbprint = string.IsNullOrEmpty(serverCertificate.ThumbprintSecondary) ?
                                              serverCertificate.Thumbprint :
                                              string.Format(CultureInfo.InvariantCulture, "{0},{1}", serverCertificate.Thumbprint, serverCertificate.ThumbprintSecondary);
            }

            var defaultClientCertThumbprint = !string.IsNullOrWhiteSpace(clusterCertificateThumbprint) ? clusterCertificateThumbprint : serverCertificateThumbprint;
            var nonAdminClientCertThumbprints = new StringBuilder(defaultClientCertThumbprint);
            var adminClientCertThumbprints = new StringBuilder(defaultClientCertThumbprint);
            bool isAdminEmpty = string.IsNullOrWhiteSpace(adminClientCertThumbprints.ToString());
            bool isNonAdminEmpty = string.IsNullOrWhiteSpace(nonAdminClientCertThumbprints.ToString());

            if (certificateInformation.ClientCertificateThumbprints != null)
            {
                certificateInformation.ClientCertificateThumbprints.ForEach(
                    clientThumbprint =>
                    {
                        if (clientThumbprint.IsAdmin)
                        {
                            if (!isAdminEmpty)
                            {
                                adminClientCertThumbprints.Append(",");
                            }

                            adminClientCertThumbprints.Append(clientThumbprint.CertificateThumbprint);
                            isAdminEmpty = false;
                        }
                        else
                        {
                            if (!isNonAdminEmpty)
                            {
                                nonAdminClientCertThumbprints.Append(",");
                            }

                            nonAdminClientCertThumbprints.Append(clientThumbprint.CertificateThumbprint);
                            isNonAdminEmpty = false;
                        }
                    });
            }

            if (clusterCertificateThumbprint != null)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.ClusterCertThumbprints,
                        Value = clusterCertificateThumbprint
                    });
            }

            if (serverCertificateThumbprint != null)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.ServerCertThumbprints,
                        Value = serverCertificateThumbprint
                    });
            }

            if (!isNonAdminEmpty)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.ClientCertThumbprints,
                        Value = nonAdminClientCertThumbprints.ToString()
                    });
            }

            if (!isAdminEmpty)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.AdminClientCertThumbprints,
                        Value = adminClientCertThumbprints.ToString()
                    });
            }

            securityParameterList.Add(
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.IgnoreCrlOfflineError,
                    Value = "true"
                });

            securityParameterList.Add(
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.UseSecondaryIfNewer,
                    Value = true.ToString()
                });

            // 'Security/XXXX509Names' sections: cn based
            var securityNonAdminClientX509ParameterList = new Collection<SettingsTypeSectionParameter>();
            var securityAdminClientX509ParameterList = new Collection<SettingsTypeSectionParameter>();

            if ((commonNameClusterCertList != null && commonNameClusterCertList.Any())
                || (certificateInformation.ServerCertificateCommonNames != null && certificateInformation.ServerCertificateCommonNames.Any()))
            {
                Dictionary<string, string> defaultClientCertCommonNames = commonNameClusterCertList != null && commonNameClusterCertList.Any()
                    ? commonNameClusterCertList : certificateInformation.ServerCertificateCommonNames.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);

                foreach (var cert in defaultClientCertCommonNames)
                {
                    securityNonAdminClientX509ParameterList.Add(
                        new SettingsTypeSectionParameter()
                        {
                            Name = cert.Key,
                            Value = cert.Value == null ? string.Empty : cert.Value
                        });
                    securityAdminClientX509ParameterList.Add(
                        new SettingsTypeSectionParameter()
                        {
                            Name = cert.Key,
                            Value = cert.Value == null ? string.Empty : cert.Value
                        });
                }
            }

            if (commonNameClusterCertList != null && commonNameClusterCertList.Any())
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityClusterX509Names,
                    Parameter = commonNameClusterCertList.Select(
                        p => new SettingsTypeSectionParameter()
                        {
                            Name = p.Key,
                            Value = p.Value == null ? string.Empty : p.Value
                        }).ToArray()
                });
            }

            if (certificateInformation.ClientCertificateCommonNames != null)
            {
                certificateInformation.ClientCertificateCommonNames.ForEach(
                    clientCommonName =>
                    {
                        if (clientCommonName.IsAdmin)
                        {
                            securityAdminClientX509ParameterList.Add(
                                new SettingsTypeSectionParameter()
                                {
                                    Name = clientCommonName.CertificateCommonName,
                                    Value = clientCommonName.CertificateIssuerThumbprint == null ? string.Empty : clientCommonName.CertificateIssuerThumbprint
                                });
                        }
                        else
                        {
                            securityNonAdminClientX509ParameterList.Add(
                                new SettingsTypeSectionParameter()
                                {
                                    Name = clientCommonName.CertificateCommonName,
                                    Value = clientCommonName.CertificateIssuerThumbprint == null ? string.Empty : clientCommonName.CertificateIssuerThumbprint
                                });
                        }
                    });
            }

            var combinedAdminAndNonAdminX509ParameterList = new HashSet<SettingsTypeSectionParameter>(securityNonAdminClientX509ParameterList);
            combinedAdminAndNonAdminX509ParameterList.UnionWith(securityAdminClientX509ParameterList);
            if (securityNonAdminClientX509ParameterList.Count > 0)
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityClientX509Names,
                    Parameter = combinedAdminAndNonAdminX509ParameterList.ToArray()
                });
            }

            if (securityAdminClientX509ParameterList.Count > 0)
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityAdminClientX509Names,
                    Parameter = securityAdminClientX509ParameterList.ToArray()
                });
            }

            if (certificateInformation.ServerCertificateCommonNames != null && certificateInformation.ServerCertificateCommonNames.Any())
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityServerX509Names,
                    Parameter = certificateInformation.ServerCertificateCommonNames.CommonNames.Select(
                        p => new SettingsTypeSectionParameter()
                        {
                            Name = p.CertificateCommonName,
                            Value = p.CertificateIssuerThumbprint == null ? string.Empty : p.CertificateIssuerThumbprint
                        }).ToArray()
                });
            }

            if (certificateInformation.ClusterCertificateIssuerStores != null && certificateInformation.ClusterCertificateIssuerStores.Count() != 0)
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityClusterCertificateIssuerStores,
                    Parameter = certificateInformation.ClusterCertificateIssuerStores.Select(
                        p => new SettingsTypeSectionParameter()
                        {
                            Name = p.IssuerCommonName,
                            Value = p.X509StoreNames
                        }).ToArray()
                });
            }

            if (certificateInformation.ServerCertificateIssuerStores != null && certificateInformation.ServerCertificateIssuerStores.Count() != 0)
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityServerCertificateIssuerStores,
                    Parameter = certificateInformation.ServerCertificateIssuerStores.Select(
                        p => new SettingsTypeSectionParameter()
                        {
                            Name = p.IssuerCommonName,
                            Value = p.X509StoreNames
                        }).ToArray()
                });
            }

            if (certificateInformation.ClientCertificateIssuerStores != null && certificateInformation.ClientCertificateIssuerStores.Count() != 0)
            {
                securitySections.Add(new SettingsTypeSection()
                {
                    Name = StringConstants.SectionName.SecurityClientCertificateIssuerStores,
                    Parameter = certificateInformation.ClientCertificateIssuerStores.Select(
                          p => new SettingsTypeSectionParameter()
                          {
                              Name = p.IssuerCommonName,
                              Value = p.X509StoreNames
                          }).ToArray()
                });
            }
        }

        private void AddWindowsSecuritySections(List<SettingsTypeSectionParameter> securityParameterList, Security security)
        {
            if (security.WindowsIdentities.ClusterSPN != null)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.ClusterSpn,
                        Value = security.WindowsIdentities.ClusterSPN
                    });
            }

            if (security.WindowsIdentities.ClusterIdentity != null)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.ClusterIdentities,
                        Value = security.WindowsIdentities.ClusterIdentity
                    });
            }

            if (security.WindowsIdentities.ClientIdentities != null)
            {
                var clientIdentities = new StringBuilder();
                var adminClientIdentities = new StringBuilder();
                security.WindowsIdentities.ClientIdentities.ForEach(
                    ci =>
                    {
                        if (ci.IsAdmin)
                        {
                            adminClientIdentities.AppendFormat(CultureInfo.InvariantCulture, "{0},", ci.Identity);
                        }
                        else
                        {
                            clientIdentities.AppendFormat(CultureInfo.InvariantCulture, "{0},", ci.Identity);
                        }
                    });

                if (adminClientIdentities.Length != 0)
                {
                    adminClientIdentities.Length--;
                    securityParameterList.Add(
                        new SettingsTypeSectionParameter()
                        {
                            Name = StringConstants.ParameterName.AdminClientIdentities,
                            Value = adminClientIdentities.ToString()
                        });
                }

                if (clientIdentities.Length != 0)
                {
                    clientIdentities.Length--;
                    securityParameterList.Add(
                        new SettingsTypeSectionParameter()
                        {
                            Name = StringConstants.ParameterName.ClientIdentities,
                            Value = clientIdentities.ToString()
                        });
                }
            }

            if (security.WindowsIdentities.FabricHostSpn != null)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter()
                    {
                        Name = StringConstants.ParameterName.FabricHostSpn,
                        Value = security.WindowsIdentities.FabricHostSpn
                    });
            }
        }

        private SettingsTypeSection GenerateUpgradeOrchestrationServiceSection()
        {
            List<SettingsTypeSectionParameter> parameterList = new List<SettingsTypeSectionParameter>()
            {
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.ClusterId,
                    Value = this.ClusterId
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.AutoupgradeEnabled,
                    Value = this.TargetCsmConfig.AutoupgradeEnabled.ToString()
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.AutoupgradeInstallEnabled,
                    Value = (this.TargetCsmConfig != null && this.TargetCsmConfig.CodeVersion == DMConstants.AutoupgradeCodeVersion) ? "True" : "False"
                },
                new SettingsTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.GoalStateExpirationReminderInDays,
                    Value = (this.TargetCsmConfig != null && this.TargetCsmConfig.GoalStateExpirationReminderInDays > 0) ? this.TargetCsmConfig.GoalStateExpirationReminderInDays.ToString() : DMConstants.DefaultGoalStateExpirationReminderInDays.ToString()
                }
            };
            parameterList.AddRange(StandAloneFabricSettingsGenerator.GenerateStatefulSvcReplicaParameters(this.TargetCsmConfig));

            return new SettingsTypeSection()
            {
                Name = StringConstants.SectionName.UpgradeOrchestrationService,
                Parameter = parameterList.ToArray(),
            };
        }
    }
}