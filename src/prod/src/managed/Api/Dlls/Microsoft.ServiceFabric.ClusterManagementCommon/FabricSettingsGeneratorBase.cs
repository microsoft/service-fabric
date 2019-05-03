// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;

    internal abstract class FabricSettingsGeneratorBase
    {
        private static readonly TraceType TraceType = new TraceType("FabricSettingsGenerator");

        private readonly DiagnosticsStorageAccountConfig diagnosticsStorageAccountConfig;
        private readonly EncryptableDiagnosticStoreInformation diagnosticsStoreInformation;
        private bool isDiagnosticsDisabled;
        private bool isDepreciatedDiagnosticsSectionNamesUsed;
        private ClusterManifestGeneratorSettings clusterManifestGeneratorSettings;

        protected FabricSettingsGeneratorBase(
            string clusterIdOrName,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        {
            this.ClusterId = clusterIdOrName;
            this.TargetCsmConfig = targetCsmConfig;
            this.TargetWrpConfig = targetWrpConfig;
            this.TargetSettingsMetadata = targetWrpConfig.GetFabricSettingsMetadata();
            this.CurrentSettingsMetadata = currentFabricSettingsMetadata;
            this.ExistingClusterManifest = existingClusterManifest;
            this.TraceLogger = traceLogger;
            this.clusterManifestGeneratorSettings = clusterManifestGeneratorSettings;
            this.diagnosticsStoreInformation = this.TargetCsmConfig.DiagnosticsStoreInformation;
            this.diagnosticsStorageAccountConfig = this.TargetCsmConfig.DiagnosticsStorageAccountConfig;
            this.isDiagnosticsDisabled = (this.diagnosticsStorageAccountConfig == null) && (this.diagnosticsStoreInformation == null);
            this.isDepreciatedDiagnosticsSectionNamesUsed = this.TargetWrpConfig.GetAdminFabricSettings().CommonClusterSettings.Section.FirstOrDefault(
                        section => section.Name.Equals(StringConstants.SectionName.WinFabEtlFile, StringComparison.OrdinalIgnoreCase)) != null;
        }

        protected string ClusterId
        {
            get;
            private set;
        }

        protected IUserConfig TargetCsmConfig
        {
            get;
            private set;
        }

        protected IAdminConfig TargetWrpConfig
        {
            get;
            private set;
        }

        protected FabricSettingsMetadata TargetSettingsMetadata
        {
            get;
            private set;
        }

        protected FabricSettingsMetadata CurrentSettingsMetadata
        {
            get;
            private set;
        }

        protected ClusterManifestType ExistingClusterManifest
        {
            get;
            private set;
        }

        protected ITraceLogger TraceLogger
        {
            get;
            private set;
        }

        public SettingsOverridesTypeSection[] GenerateFabricSettings(Security security, int currentClusterSize = -1)
        {
            CertificateDescription thumbprintClusterCert = null;
            List<string> thumbprintClusterCertList = new List<string>();
            ServerCertificateCommonNames commonNameClusterCert = null;
            Dictionary<string, string> commonNameClusterCertList = new Dictionary<string, string>();

            if (security != null && security.CertificateInformation != null)
            {
                if (security.CertificateInformation.ClusterCertificate != null)
                {
                    thumbprintClusterCert = security.CertificateInformation.ClusterCertificate;
                    thumbprintClusterCertList = thumbprintClusterCert.ToThumbprintList();
                }

                if (security.CertificateInformation.ClusterCertificateCommonNames != null
                    && security.CertificateInformation.ClusterCertificateCommonNames.Any())
                {
                    commonNameClusterCert = security.CertificateInformation.ClusterCertificateCommonNames;
                    commonNameClusterCertList = commonNameClusterCert.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);
                }
            }

            return currentClusterSize != -1 ?
                this.GenerateFabricSettings(
                    security,
                    thumbprintClusterCert,
                    thumbprintClusterCertList,
                    commonNameClusterCert,
                    commonNameClusterCertList,
                    currentClusterSize)
                : this.GenerateFabricSettings(
                    security,
                    thumbprintClusterCert,
                    thumbprintClusterCertList,
                    commonNameClusterCert,
                    commonNameClusterCertList);
        }

        public SettingsOverridesTypeSection[] GenerateFabricSettings(
            Security security,
            CertificateDescription thumbprintFileStoreCert,
            List<string> thumbprintClusterCertList,
            ServerCertificateCommonNames commonNameFileStoreCert,
            Dictionary<string, string> commonNameClusterCertList,
            int currentClusterSize = -1)
        {
            var wrpGeneratedSections = this.OnGenerateFabricSettings(
                security,
                thumbprintFileStoreCert,
                thumbprintClusterCertList,
                commonNameFileStoreCert,
                commonNameClusterCertList,
                currentClusterSize);
            this.AddPlacementConstraintsForSystemServices(wrpGeneratedSections);
            this.AddReplicaSetSizeForSystemServices(wrpGeneratedSections);

            SettingsType wrpGeneratedClusterSettings = new SettingsType
            {
                Section = wrpGeneratedSections.ToArray()
            };

            SettingsType userDefinedClusterSettings = new SettingsType
            {
                Section = this.TargetCsmConfig.FabricSettings.Select(section => section.ToSettingsTypeSectionParameter()).ToArray()
            };

            var clusterManifestSettings = this.TargetWrpConfig.GetAdminFabricSettings();
            var fabricSettings = this.MergeSettings(
                                     clusterManifestSettings.CommonClusterSettings,
                                     userDefinedClusterSettings,
                                     wrpGeneratedClusterSettings);

            return fabricSettings;
        }

        protected abstract List<SettingsTypeSection> OnGenerateFabricSettings(
            Security security,
            CertificateDescription thumbprintFileStoreCert,
            List<string> thumbprintClusterCertList,
            ServerCertificateCommonNames commonNameFileStoreCert,
            Dictionary<string, string> commonNameClusterCertList,
            int currentClusterSize);

        // In standalone add node and reliability level multiphase upgrade, seed node is added one by one. We can't set expectedClusterSize to the final target total node count.
        // FabricDeployer will throw validation expection when node count is smaller than expectedClusterSize.
        // Therefore here for other scenarios, it can set expectedClusterSize to to targetCsmConfig.totalNodeCount. For standalone, it uses the infrastructure value from standaloneClusterManifestBuilder.
        protected SettingsTypeSection GenerateFailoverManagerSection(int currentClusterSize = -1)
        {
            var expectedClusterSize = currentClusterSize == -1 ? this.TargetCsmConfig.TotalNodeCount : currentClusterSize;
            List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>();
            parameters.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ExpectedClusterSize,
                    Value = GetExpectedClusterSize(expectedClusterSize).ToString()
                });

            if (this.TargetCsmConfig.TotalNodeCount == 1)
            {
                parameters.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsSingletonReplicaMoveAllowedDuringUpgrade,
                        Value = "false"
                    });
            }

            return new SettingsTypeSection
            {
                Name = StringConstants.SectionName.FailoverManager,
                Parameter = parameters.ToArray()
            };
        }

        protected List<SettingsTypeSection> GenerateInfrastructureService()
        {
            if (this.TargetCsmConfig.IsVMSS)
            {
                List<SettingsTypeSection> infrastructureServiceSections = new List<SettingsTypeSection>();
                foreach (var nodeType in this.TargetCsmConfig.NodeTypes)
                {
                    if (nodeType.DurabilityLevel != DurabilityLevel.Bronze)
                    {
                        // Enable IS only if DurabilityLevel is not Bronze
                        var infrastructureServiceSection = this.GenerateInfrastructureServiceSection(nodeType.Name, nodeType.VMInstanceCount);
                        infrastructureServiceSections.Add(infrastructureServiceSection);
                    }
                }

                return infrastructureServiceSections;
            }

            return null;
        }

        protected virtual List<SettingsTypeSection> GenerateSecuritySection(CertificateDescription certificate)
        {
            if (certificate == null)
            {
                return null;
            }

            string certificateThumbprint = certificate.Thumbprint;
            if (!string.IsNullOrEmpty(certificate.ThumbprintSecondary))
            {
                certificateThumbprint = string.Format("{0},{1}", certificate.Thumbprint, certificate.ThumbprintSecondary);
            }

            var securitySections = new List<SettingsTypeSection>();

            var securityParameterList = new Collection<SettingsTypeSectionParameter>
            {
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ClusterCredentialType,
                    Value = StringConstants.X509CredentialType
                },
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ServerAuthCredentialType,
                    Value = StringConstants.X509CredentialType
                }
            };

            var nonAdminClientCertThumbprints = new StringBuilder(certificateThumbprint);
            var adminClientCertThumbprints = new StringBuilder(certificateThumbprint);

            this.TargetCsmConfig.Security.CertificateInformation.ClientCertificateThumbprints.ForEach(
                clientThumbprint =>
                {
                    if (clientThumbprint.IsAdmin)
                    {
                        adminClientCertThumbprints.Append(",");
                        adminClientCertThumbprints.Append(clientThumbprint.CertificateThumbprint);
                    }
                    else
                    {
                        nonAdminClientCertThumbprints.Append(",");
                        nonAdminClientCertThumbprints.Append(clientThumbprint.CertificateThumbprint);
                    }
                });

            securityParameterList.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ClusterCertThumbprints,
                    Value = certificateThumbprint
                });
            securityParameterList.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ServerCertThumbprints,
                    Value = certificateThumbprint
                });
            securityParameterList.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.ClientCertThumbprints,
                    Value = nonAdminClientCertThumbprints.ToString()
                });
            securityParameterList.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.AdminClientCertThumbprints,
                    Value = adminClientCertThumbprints.ToString()
                });
            securityParameterList.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.IgnoreCrlOfflineError,
                    Value = "true"
                });

            if (this.TargetCsmConfig.Security.AzureActiveDirectory != null)
            {
                securityParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.AadTenantId,
                        Value = this.TargetCsmConfig.Security.AzureActiveDirectory.TenantId
                    });
                securityParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.AadClusterApplication,
                        Value = this.TargetCsmConfig.Security.AzureActiveDirectory.ClusterApplication
                    });
                securityParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.AadClientApplication,
                        Value = this.TargetCsmConfig.Security.AzureActiveDirectory.ClientApplication
                    });
            }

            var securitySection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.Security,
                Parameter = securityParameterList.ToArray()
            };

            securitySections.Add(securitySection);

            var securityNonAdminClientX509ParameterList = new Collection<SettingsTypeSectionParameter>();
            var securityAdminClientX509ParameterList = new Collection<SettingsTypeSectionParameter>();

            this.TargetCsmConfig.Security.CertificateInformation.ClientCertificateCommonNames.ForEach(
                clientCommonName =>
                {
                    if (clientCommonName.IsAdmin)
                    {
                        securityAdminClientX509ParameterList.Add(
                            new SettingsTypeSectionParameter
                            {
                                Name = clientCommonName.CertificateCommonName,
                                Value = clientCommonName.CertificateIssuerThumbprint
                            });
                    }
                    else
                    {
                        securityNonAdminClientX509ParameterList.Add(
                            new SettingsTypeSectionParameter
                            {
                                Name = clientCommonName.CertificateCommonName,
                                Value = clientCommonName.CertificateIssuerThumbprint
                            });
                    }
                });

            if (securityNonAdminClientX509ParameterList.Count > 0)
            {
                var securityClientX509NamesSection = new SettingsTypeSection
                {
                    Name = StringConstants.SectionName.SecurityClientX509Names,
                    Parameter = securityNonAdminClientX509ParameterList.ToArray()
                };

                securitySections.Add(securityClientX509NamesSection);
            }

            if (securityAdminClientX509ParameterList.Count > 0)
            {
                var securityAdminClientX509NamesSection = new SettingsTypeSection
                {
                    Name = StringConstants.SectionName.SecurityAdminClientX509Names,
                    Parameter = securityAdminClientX509ParameterList.ToArray()
                };

                securitySections.Add(securityAdminClientX509NamesSection);
            }

            return securitySections;
        }

        protected SettingsTypeSection GenerateInfrastructureServiceSection(string nodeTypeName, int nodeTypeSize)
        {
            var infraServiceParameterList = new Collection<SettingsTypeSectionParameter>
            {
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.EnableRepairExecution,
                    Value = "true"
                }
            };

            var replicaSetSize = this.TargetCsmConfig.ReliabilityLevel.GetReplicaSetSize();

            // InfrastructureService name is not well known like other system services. Hence the logic for
            // adding placement constraints and replica set size is special cased here
            infraServiceParameterList.Add(new SettingsTypeSectionParameter
            {
                Name = StringConstants.ParameterName.PlacementConstraints,
                Value = string.Format("{0}=={1}", StringConstants.DefaultPlacementConstraintsKey, nodeTypeName)
            });

            infraServiceParameterList.Add(new SettingsTypeSectionParameter
            {
                Name = StringConstants.ParameterName.MinReplicaSetSize,
                Value = replicaSetSize.MinReplicaSetSize.ToString()
            });

            infraServiceParameterList.Add(new SettingsTypeSectionParameter
            {
                Name = StringConstants.ParameterName.TargetReplicaSetSize,
                Value = replicaSetSize.TargetReplicaSetSize.ToString()
            });

            var infraServiceSection = new SettingsTypeSection
            {
                Name = string.Format("{0}/{1}", StringConstants.SectionName.InfrastructureServicePrefix, nodeTypeName),
                Parameter = infraServiceParameterList.ToArray()
            };

            return infraServiceSection;
        }

        protected List<SettingsTypeSection> GenerateDcaSections()
        {
            if (this.diagnosticsStorageAccountConfig != null)
            {
                return this.GenerateBlobStoreDcaSections();
            }

            if (this.diagnosticsStoreInformation != null)
            {
                if (this.diagnosticsStoreInformation.StoreType == DiagnosticStoreType.AzureStorage)
                {
                    return this.GenerateBlobStoreDcaSections();
                }
                else if (this.diagnosticsStoreInformation.StoreType == DiagnosticStoreType.FileShare)
                {
                    return this.GenerateFileShareDcaSections();
                }
            }

            return null;
        }

        protected string GenerateDcaConnectionString(DiagnosticsStorageAccountConfig diagnosticsStorageAccountConfig, EncryptableDiagnosticStoreInformation diagnosticsStoreInformation)
        {
            if (diagnosticsStoreInformation != null)
            {
                return diagnosticsStoreInformation.Connectionstring;
            }

            var accountKeySubString = !string.IsNullOrEmpty(diagnosticsStorageAccountConfig.ProtectedAccountKeyName) ? string.Format(CultureInfo.InvariantCulture, "ProtectedAccountKeyName={0}", diagnosticsStorageAccountConfig.ProtectedAccountKeyName) : string.Format(CultureInfo.InvariantCulture, "AccountKey={0}", !string.IsNullOrEmpty(diagnosticsStorageAccountConfig.PrimaryAccessKey) ? diagnosticsStorageAccountConfig.PrimaryAccessKey : diagnosticsStorageAccountConfig.SecondaryAccessKey);

            // If explicit endpoints are used for storage accounts, then endpoints for blob/table services are all required. Otherwise, we would use default endpoints + https.
            if (!string.IsNullOrEmpty(diagnosticsStorageAccountConfig.BlobEndpoint) && !string.IsNullOrEmpty(diagnosticsStorageAccountConfig.TableEndpoint))
            {
                return string.Format(
                           CultureInfo.InvariantCulture,
                           "xstore:BlobEndpoint={0};TableEndpoint={1};AccountName={2};{3}",
                           diagnosticsStorageAccountConfig.BlobEndpoint,
                           diagnosticsStorageAccountConfig.TableEndpoint,
                           diagnosticsStorageAccountConfig.StorageAccountName,
                           accountKeySubString);
            }

            return string.Format(
                       CultureInfo.InvariantCulture,
                       "xstore:DefaultEndpointsProtocol=https;AccountName={0};{1}",
                       diagnosticsStorageAccountConfig.StorageAccountName,
                       accountKeySubString);
        }

        protected SettingsTypeSection GenerateHttpApplicationGatewaySection()
        {
            var httpApplicationGatewayParameterList = new Collection<SettingsTypeSectionParameter>();

            var isHttpApplicationGatewayEnabled = this.TargetCsmConfig.NodeTypes.Any(nodeType => nodeType.HttpApplicationGatewayEndpointPort != null);

            if (!isHttpApplicationGatewayEnabled)
            {
                return null;
            }

            httpApplicationGatewayParameterList.Add(
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.IsEnabled,
                    Value = "true"
                });

            if (this.TargetCsmConfig.Security != null &&
                this.TargetCsmConfig.Security.CertificateInformation != null &&
                (this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate != null ||
                    (this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames != null
                        && this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.Any())))
            {
                httpApplicationGatewayParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.GatewayAuthCredentialType,
                        Value = StringConstants.X509CredentialType
                    });

                if (this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate != null)
                {
                    httpApplicationGatewayParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.GatewayX509CertificateStoreName,
                            Value = this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate.X509StoreName.ToString()
                        });

                    httpApplicationGatewayParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.GatewayX509CertificateFindType,
                            Value = X509FindType.FindByThumbprint.ToString()
                        });

                    httpApplicationGatewayParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.GatewayX509CertificateFindValue,
                            Value = this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate.Thumbprint
                        });

                    if (!string.IsNullOrEmpty(this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate.ThumbprintSecondary))
                    {
                        httpApplicationGatewayParameterList.Add(
                            new SettingsTypeSectionParameter
                            {
                                Name = StringConstants.ParameterName.GatewayX509CertificateFindValueSecondary,
                                Value = this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate.ThumbprintSecondary
                            });
                    }
                }
                else
                {
                    httpApplicationGatewayParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.GatewayX509CertificateStoreName,
                            Value = this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.X509StoreName.ToString()
                        });

                    httpApplicationGatewayParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.GatewayX509CertificateFindType,
                            Value = X509FindType.FindBySubjectName.ToString()
                        });

                    httpApplicationGatewayParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.GatewayX509CertificateFindValue,
                            Value = this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames[0].CertificateCommonName
                        });

                    if (this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames.Count > 1)
                    {
                        httpApplicationGatewayParameterList.Add(
                            new SettingsTypeSectionParameter
                            {
                                Name = StringConstants.ParameterName.GatewayX509CertificateFindValueSecondary,
                                Value = this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.CommonNames[1].CertificateCommonName
                            });
                    }
                }
            }

            var httpApplicationGatewaySection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.HttpApplicationGateway,
                Parameter = httpApplicationGatewayParameterList.ToArray()
            };

            return httpApplicationGatewaySection;
        }

        protected SettingsTypeSection GenerateFileStoreServiceSection(
            CertificateDescription thumbprintFileStoreCert,
            ServerCertificateCommonNames commonNameFileStoreCert,
            string currentPrimaryAccountNtlmSecret,
            string currentSecondaryAccountNtlmSecret,
            string currentCommonNameNtlmSecret)
        {
            var primaryAccountNtlmPassword = string.IsNullOrEmpty(currentPrimaryAccountNtlmSecret) ? Guid.NewGuid().ToString() : currentPrimaryAccountNtlmSecret;
            var secondaryAccountNtlmPassword = string.IsNullOrEmpty(currentSecondaryAccountNtlmSecret) ? Guid.NewGuid().ToString() : currentSecondaryAccountNtlmSecret;
            var commonNameNtlmPassword = string.IsNullOrEmpty(currentCommonNameNtlmSecret) ? Guid.NewGuid().ToString() : currentCommonNameNtlmSecret;
            
            bool useCertficateForNtlmAuth = true;
            bool thumbprintCertConfigured = thumbprintFileStoreCert != null && !string.IsNullOrEmpty(thumbprintFileStoreCert.Thumbprint);
            bool commonNameCertConfigured = commonNameFileStoreCert != null && commonNameFileStoreCert.Any();
            bool certConfigured = thumbprintCertConfigured || commonNameCertConfigured;

            if (this.ExistingClusterManifest != null)
            {
                var fileStoreSection =
                    this.ExistingClusterManifest.FabricSettings.FirstOrDefault(
                        section => section.Name.Equals(StringConstants.SectionName.FileStoreService, StringComparison.OrdinalIgnoreCase));
                if (fileStoreSection != null)
                {
                    var primarySecretParameter = fileStoreSection.Parameter.FirstOrDefault(
                                                     parameter => parameter.Name.Equals(StringConstants.ParameterName.PrimaryAccountNtlmPasswordSecret, StringComparison.OrdinalIgnoreCase));
                    if (primarySecretParameter != null)
                    {
                        primaryAccountNtlmPassword = primarySecretParameter.Value;
                    }

                    var secondarySecretParameter = fileStoreSection.Parameter.FirstOrDefault(
                                                       parameter => parameter.Name.Equals(StringConstants.ParameterName.SecondaryAccountNtlmPasswordSecret, StringComparison.OrdinalIgnoreCase));
                    if (secondarySecretParameter != null)
                    {
                        secondaryAccountNtlmPassword = secondarySecretParameter.Value;
                    }

                    var commonNameSecretParameter = fileStoreSection.Parameter.FirstOrDefault(
                                                     parameter => parameter.Name.Equals(StringConstants.ParameterName.CommonNameNtlmPasswordSecret, StringComparison.OrdinalIgnoreCase));
                    if (commonNameSecretParameter != null)
                    {
                        commonNameNtlmPassword = commonNameSecretParameter.Value;
                    }
                }

                var matchingPrimaryCertThumbprint = fileStoreSection.Parameter.FirstOrDefault(
                                                        parameter => parameter.Name.Equals(StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint, StringComparison.OrdinalIgnoreCase));

                var matchingSecondaryCertThumbprint = fileStoreSection.Parameter.FirstOrDefault(
                        parameter => parameter.Name.Equals(StringConstants.ParameterName.SecondaryAccountNtlmx509Thumbprint, StringComparison.OrdinalIgnoreCase));

                var matchingCommonName1CertCommonName = fileStoreSection.Parameter.FirstOrDefault(
                        parameter => parameter.Name.Equals(StringConstants.ParameterName.CommonName1Ntlmx509CommonName, StringComparison.OrdinalIgnoreCase));

                var matchingCommonName2CertCommonName = fileStoreSection.Parameter.FirstOrDefault(
                        parameter => parameter.Name.Equals(StringConstants.ParameterName.CommonName2Ntlmx509CommonName, StringComparison.OrdinalIgnoreCase));

                useCertficateForNtlmAuth = (matchingPrimaryCertThumbprint != null && matchingSecondaryCertThumbprint != null)
                    || (matchingCommonName1CertCommonName != null && matchingCommonName2CertCommonName != null);

                ReleaseAssert.AssertIf(useCertficateForNtlmAuth && !certConfigured, "CertificateInNTLMAuth is present in existing ClusterManifest but no certificate is provided in target version. ClusterId: {0}", this.ClusterId);
            }

            var fileStoreServiceParameterList = new Collection<SettingsTypeSectionParameter>
            {
                new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.AnonymousAccessEnabled,
                    Value = "false"
                }
            };

            if (!certConfigured || thumbprintCertConfigured)
            {
                fileStoreServiceParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.PrimaryAccountType,
                        Value = "LocalUser"
                    });

                fileStoreServiceParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.PrimaryAccountNtlmPasswordSecret,
                        Value = primaryAccountNtlmPassword
                    });

                fileStoreServiceParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.SecondaryAccountType,
                        Value = "LocalUser"
                    });

                fileStoreServiceParameterList.Add(
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.SecondaryAccountNtlmPasswordSecret,
                        Value = secondaryAccountNtlmPassword
                    });
            }

            if (useCertficateForNtlmAuth && certConfigured)
            {
                // Add cert for NTLM only for new clusters and existing clusters
                // which already have cert for NTLM
                if (thumbprintCertConfigured)
                {
                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.PrimaryAccountNtlmx509StoreLocation,
                            Value = StoreLocation.LocalMachine.ToString()
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.PrimaryAccountNtlmx509StoreName,
                            Value = thumbprintFileStoreCert.X509StoreName.ToString()
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.PrimaryAccountNtlmx509Thumbprint,
                            Value = thumbprintFileStoreCert.Thumbprint
                        });

                    // If secondary thumbprint is not provided, use primary thumbprint as secondary as well
                    string secondaryThumbprint = string.IsNullOrEmpty(thumbprintFileStoreCert.ThumbprintSecondary) ?
                        thumbprintFileStoreCert.Thumbprint : thumbprintFileStoreCert.ThumbprintSecondary;
                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.SecondaryAccountNtlmx509StoreLocation,
                            Value = StoreLocation.LocalMachine.ToString()
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.SecondaryAccountNtlmx509StoreName,
                            Value = thumbprintFileStoreCert.X509StoreName.ToString()
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.SecondaryAccountNtlmx509Thumbprint,
                            Value = secondaryThumbprint
                        });
                }

                if (commonNameCertConfigured)
                {
                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonNameNtlmPasswordSecret,
                            Value = commonNameNtlmPassword
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonName1Ntlmx509StoreLocation,
                            Value = StoreLocation.LocalMachine.ToString()
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonName1Ntlmx509StoreName,
                            Value = commonNameFileStoreCert.X509StoreName.ToString()
                        });

                    string cn1 = commonNameFileStoreCert.CommonNames[0].CertificateCommonName;
                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonName1Ntlmx509CommonName,
                            Value = cn1
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonName2Ntlmx509StoreLocation,
                            Value = StoreLocation.LocalMachine.ToString()
                        });

                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonName2Ntlmx509StoreName,
                            Value = commonNameFileStoreCert.X509StoreName.ToString()
                        });

                    // If cn2 is not provided, use cn1 as cn2 as well
                    string cn2 = commonNameFileStoreCert.CommonNames.Count > 1 ?
                        commonNameFileStoreCert.CommonNames[1].CertificateCommonName : cn1;
                    fileStoreServiceParameterList.Add(
                        new SettingsTypeSectionParameter
                        {
                            Name = StringConstants.ParameterName.CommonName2Ntlmx509CommonName,
                            Value = cn2
                        });
                }
            }

            var fileStoreServiceSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.FileStoreService,
                Parameter = fileStoreServiceParameterList.ToArray()
            };

            return fileStoreServiceSection;
        }

        private static int GetExpectedClusterSize(int totalExpectedNode)
        {
            if (totalExpectedNode > 10)
            {
                return (int)Math.Round(totalExpectedNode * .8, MidpointRounding.AwayFromZero);
            }

            if (totalExpectedNode > 3 && totalExpectedNode % 2 == 0)
            {
                return totalExpectedNode - 1;
            }

            return totalExpectedNode;
        }

        private void AddPlacementConstraintsForSystemServices(List<SettingsTypeSection> sections)
        {
            if (this.TargetCsmConfig.PrimaryNodeTypes.Count() == 1)
            {
                // Set the placement property for system services when there is only one primary NodeType
                var primaryNodeType = this.TargetCsmConfig.PrimaryNodeTypes.First();

                // InfrastructureService is special since the name is not well know. The placement constraint for
                // it is added in "GenerateInfrastructureServiceSection" method
                foreach (var systemServiceSectionName in this.clusterManifestGeneratorSettings.SystemServicesForScale.Keys)
                {
                    // Do not configure add-on services here, since they are configured upfront if needed
                    /* Todo 11006497: When SecretStoreSerivce is added, modify this logic since systemServiceSectionName will contain 3 new system services but AddonFeature will
                       have only a single entry "SecretStoreService". Create a new list containing these exceptional services instead of using AddonFeature*/
                    if (Enum.IsDefined(typeof(AddonFeature), systemServiceSectionName))
                    {
                        continue;
                    }

                    var matchingSection = sections.FirstOrDefault(section => section.Name == systemServiceSectionName);
                    if (matchingSection == null)
                    {
                        matchingSection = new SettingsTypeSection { Name = systemServiceSectionName, Parameter = new SettingsTypeSectionParameter[0] };
                        sections.Add(matchingSection);
                    }

                    var updatedParameters = matchingSection.Parameter.ToList();
                    updatedParameters.Add(new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.PlacementConstraints,
                        Value = string.Format("{0}=={1}", StringConstants.DefaultPlacementConstraintsKey, primaryNodeType.Name)
                    });

                    matchingSection.Parameter = updatedParameters.ToArray();
                }
            }
        }

        private void AddReplicaSetSizeForSystemServices(List<SettingsTypeSection> sections)
        {
            var replicaSetSizeForReliabilityLevel = this.TargetCsmConfig.ReliabilityLevel.GetReplicaSetSize();

            // InfrastructureService is special since the name is not well know. The replica set size for
            // it is added in "GenerateInfrastructureServiceSection" method
            foreach (var systemServiceSectionName in this.clusterManifestGeneratorSettings.SystemServicesForScale.Keys)
            {
                // Do not configure add-on services here, since they are configured upfront if needed
                /* Todo 11006497: When SecretStoreSerivce is added, modify this logic since systemServiceSectionName will contain 3 new system services but AddonFeature will
                / have only a single entry "SecretStoreService". Create a new list containing these exceptional services instead of using AddonFeature*/
                if (Enum.IsDefined(typeof(AddonFeature), systemServiceSectionName))
                {
                    continue;
                }

                var matchingSection = sections.FirstOrDefault(section => section.Name == systemServiceSectionName);
                if (matchingSection == null)
                {
                    matchingSection = new SettingsTypeSection { Name = systemServiceSectionName, Parameter = new SettingsTypeSectionParameter[0] };
                    sections.Add(matchingSection);
                }

                var updatedParameters = matchingSection.Parameter.ToList();
                updatedParameters.Add(new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.MinReplicaSetSize,
                    Value = replicaSetSizeForReliabilityLevel.MinReplicaSetSize.ToString()
                });

                updatedParameters.Add(new SettingsTypeSectionParameter
                {
                    Name = StringConstants.ParameterName.TargetReplicaSetSize,
                    Value = replicaSetSizeForReliabilityLevel.TargetReplicaSetSize.ToString()
                });

                matchingSection.Parameter = updatedParameters.ToArray();
            }
        }

        private SettingsOverridesTypeSection[] MergeSettings(
            SettingsType commonClusterSettings,
            SettingsType userDefinedClusterSettings,
            SettingsType wrpGeneratedClusterSettings)
        {
            Dictionary<string, List<SettingsOverridesTypeSectionParameter>> existingSettingDictionary = null;
            if (this.ExistingClusterManifest != null)
            {
                existingSettingDictionary = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();
                foreach (var section in this.ExistingClusterManifest.FabricSettings)
                {
                    existingSettingDictionary[section.Name] = section.Parameter.ToList();
                }
            }

            var mergedSettingDictionary = new Dictionary<string, List<SettingsOverridesTypeSectionParameter>>();

            // The order of this list determines the parameter which gets used in case the same parameter
            // exists in more than one ClusterSettings
            // ie. parameter in UserDefinedClusterSettings overrides all others
            var settingsToMerge = new List<SettingsType>
            {
                userDefinedClusterSettings,
                wrpGeneratedClusterSettings,
                commonClusterSettings
            };

            foreach (var settingToMerge in settingsToMerge)
            {
                foreach (var sectionToMerge in settingToMerge.Section)
                {
                    if (sectionToMerge == null || sectionToMerge.Parameter == null)
                    {
                        continue;
                    }

                    List<SettingsOverridesTypeSectionParameter> parameters;
                    if (!mergedSettingDictionary.TryGetValue(sectionToMerge.Name, out parameters))
                    {
                        parameters = new List<SettingsOverridesTypeSectionParameter>();
                        mergedSettingDictionary.Add(sectionToMerge.Name, parameters);
                    }

                    foreach (var parameterToMerge in sectionToMerge.Parameter)
                    {
                        // Check if parameter is already been processed
                        if (!parameters.Any(parameter =>
                                            string.Equals(parameter.Name, parameterToMerge.Name)))
                        {
                            if (this.TargetSettingsMetadata != null &&
                                !this.TargetSettingsMetadata.IsValidConfiguration(sectionToMerge.Name, parameterToMerge.Name))
                            {
                                this.TraceLogger.WriteWarning(
                                    TraceType,
                                    "Skip configuration Section={0} and Parameter={1} since it is not valid in the target version.",
                                    sectionToMerge.Name,
                                    parameterToMerge.Name);
                                continue;
                            }

                            if (this.CurrentSettingsMetadata != null &&
                                !this.CurrentSettingsMetadata.IsValidConfiguration(sectionToMerge.Name, parameterToMerge.Name))
                            {
                                this.TraceLogger.WriteWarning(
                                    TraceType,
                                    "Skip configuration Section={0} and Parameter={1} since it is not valid in the current version",
                                    sectionToMerge.Name,
                                    parameterToMerge.Name);
                                continue;
                            }

                            var parameter = new SettingsOverridesTypeSectionParameter
                            {
                                Name = parameterToMerge.Name,
                                Value = parameterToMerge.Value,
                                IsEncrypted = parameterToMerge.IsEncrypted
                            };

                            if (existingSettingDictionary != null)
                            {
                                // The cluster manifest is getting updated. Do not allow parameters marked as NotAllowed to change
                                SettingsOverridesTypeSectionParameter matchingExistingParameter = null;
                                List<SettingsOverridesTypeSectionParameter> matchingExistingSectionParameters;
                                if (existingSettingDictionary.TryGetValue(sectionToMerge.Name, out matchingExistingSectionParameters))
                                {
                                    matchingExistingParameter = matchingExistingSectionParameters.FirstOrDefault(existingParameter => existingParameter.Name == parameterToMerge.Name);
                                }

                                ReleaseAssert.AssertIfNull(this.CurrentSettingsMetadata, "CurrentSettingsMetadata should be not null");

                                if (this.CurrentSettingsMetadata.CanConfigurationChange(
                                    sectionToMerge.Name, 
                                    parameterToMerge.Name))
                                {
                                    if (this.CurrentSettingsMetadata.IsPolicySingleChange(sectionToMerge.Name, parameterToMerge.Name))
                                    {
                                        // The parameter can be changed only once.
                                        if (matchingExistingParameter == null)
                                        {
                                            // Parameter does not exist and is newly being added
                                            parameters.Add(parameter);

                                            this.TraceLogger.WriteInfo(
                                                TraceType,
                                                "Using new parameter configuration Section={0} and Parameter={1} since it is marked as SingleChange and is being changed for the first time.",
                                                sectionToMerge.Name,
                                                parameterToMerge.Name);
                                        }
                                        else
                                        {
                                            // The parameter cannot be changed and the parameter exists
                                            // in the existing cluster manifest. Use the parameter from the existing manfiest
                                            parameters.Add(matchingExistingParameter);

                                            this.TraceLogger.WriteInfo(
                                                TraceType,
                                                "Using existing parameter configuration Section={0} and Parameter={1} since it is marked as SingleChange and it has already been changed once.",
                                                sectionToMerge.Name,
                                                parameterToMerge.Name);
                                        }
                                    }
                                    else
                                    {
                                        // The parameter can be changed
                                        parameters.Add(parameter);
                                    }                               
                                }
                                else if (matchingExistingParameter != null)
                                {
                                    // The parameter cannot be changed and the parameter exists
                                    // in the existing cluster manifest. Use the parameter from the existing manfiest
                                    parameters.Add(matchingExistingParameter);

                                    this.TraceLogger.WriteInfo(
                                        TraceType,
                                        "Using existing parameter configuration Section={0} and Parameter={1} since it is marked as NotAllowed.",
                                        sectionToMerge.Name,
                                        parameterToMerge.Name);
                                }
                                else
                                {
                                    //// The parameter cannot be modified and it is being added newly - Ignore this parameter

                                    this.TraceLogger.WriteWarning(
                                        TraceType,
                                        "Skip adding new configuration Section={0} and Parameter={1} since it is marked as NotAllowed .",
                                        sectionToMerge.Name,
                                        parameterToMerge.Name);
                                }

                                if (matchingExistingParameter != null)
                                {
                                    matchingExistingSectionParameters.Remove(matchingExistingParameter);
                                }
                            }
                            else
                            {
                                // This is a new cluster manfiest generation
                                parameters.Add(parameter);
                            }
                        }
                    }
                }
            }

            // The remaining parameters in existingSettingDictionary are the ones which have been removed.
            // Ensure that the ones marked as 'NotAllowed' are added back
            if (existingSettingDictionary != null)
            {
                foreach (var section in existingSettingDictionary)
                {
                    foreach (var removedParameter in section.Value)
                    {
                        if (this.CurrentSettingsMetadata != null &&
                            !this.CurrentSettingsMetadata.CanConfigurationChange(section.Key, removedParameter.Name))
                        {
                            // The parameter is marked as NotAllowed. We cannot remove it
                            var parameterList = mergedSettingDictionary[section.Key];
                            if (parameterList == null)
                            {
                                parameterList = new List<SettingsOverridesTypeSectionParameter>();
                                mergedSettingDictionary[section.Key] = parameterList;
                            }

                            parameterList.Add(removedParameter);

                            this.TraceLogger.WriteWarning(
                                TraceType,
                                "Skip removing existing configuration Section={0} and Parameter={1} since it is marked as NotAllowed .",
                                section.Key,
                                removedParameter.Name);
                        }
                    }
                }
            }

            // If DiagnosticsStorageAccountConfig is unspecified in a PutCluster request, DCA would not be enabled for the cluster.
            // Therefore, we would remove all diagnostics related sections except for a single producer of type EtlFileProducer to enforce retention policy on the ETL files on the VMs.
            if (this.isDiagnosticsDisabled && mergedSettingDictionary.ContainsKey(StringConstants.SectionName.Diagnostics) && !this.TargetCsmConfig.IsLinuxVM)
            {
                var diagnosticsSectionParameters = mergedSettingDictionary[StringConstants.SectionName.Diagnostics];
                SettingsOverridesTypeSectionParameter maxDiskQuotaInMb = null;

                // Remove all producer/consumer sections.
                List<string> producerConsumerSectionNames = new List<string>();
                foreach (var diagnosticsSectionParameter in diagnosticsSectionParameters)
                {
                    if (diagnosticsSectionParameter.Name.Equals(StringConstants.ParameterName.ProducerInstances) || diagnosticsSectionParameter.Name.Equals(StringConstants.ParameterName.ConsumerInstances))
                    {
                        producerConsumerSectionNames.AddRange(diagnosticsSectionParameter.Value.Split(',').Select(producerConsumerSectionName => string.IsNullOrEmpty(producerConsumerSectionName) ? string.Empty : producerConsumerSectionName.Trim()));
                    }

                    if (diagnosticsSectionParameter.Name.Equals(StringConstants.ParameterName.MaxDiskQuotaInMb))
                    {
                        maxDiskQuotaInMb = diagnosticsSectionParameter;
                    }
                }

                foreach (var producerConsumerSectionName in producerConsumerSectionNames)
                {
                    if (mergedSettingDictionary.ContainsKey(producerConsumerSectionName))
                    {
                        mergedSettingDictionary.Remove(producerConsumerSectionName);
                    }
                }

                // Remove diagnostics section.
                mergedSettingDictionary.Remove(StringConstants.SectionName.Diagnostics);

                // Add diagnostics section with a single ServiceFabricEtlFile producer and the max disk quota.
                mergedSettingDictionary.Add(
                    StringConstants.SectionName.Diagnostics,
                    new List<SettingsOverridesTypeSectionParameter>
                {
                    new SettingsOverridesTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstances,
                        Value = this.isDepreciatedDiagnosticsSectionNamesUsed ? StringConstants.SectionName.WinFabEtlFile : StringConstants.SectionName.ServiceFabricEtlFile
                    }
                });

                if (maxDiskQuotaInMb != null)
                {
                    mergedSettingDictionary[StringConstants.SectionName.Diagnostics].Add(maxDiskQuotaInMb);
                }

                // Add ServiceFabricEtlFile producer section.
                mergedSettingDictionary.Add(
                    this.isDepreciatedDiagnosticsSectionNamesUsed ? StringConstants.SectionName.WinFabEtlFile : StringConstants.SectionName.ServiceFabricEtlFile,
                    new List<SettingsOverridesTypeSectionParameter>
                {
                    new SettingsOverridesTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = StringConstants.EtlFileProducer
                    },

                    new SettingsOverridesTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = bool.TrueString.ToLower()
                    },

                    new SettingsOverridesTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.clusterManifestGeneratorSettings.DataDeletionAgeInDaysForDiagnosticsDisabledClusters.ToString()
                    }
                });
            }

            var mergedSettings = new SettingsOverridesTypeSection[mergedSettingDictionary.Count];
            for (var i = 0; i < mergedSettingDictionary.Count; i++)
            {
                if (mergedSettingDictionary.ElementAt(i).Value == null || !mergedSettingDictionary.ElementAt(i).Value.Any())
                {
                    this.TraceLogger.WriteInfo(
                        TraceType,
                        "Skipping Section={0} since there are no parameters.",
                        mergedSettingDictionary.ElementAt(i).Key);
                    continue;
                }

                this.TraceLogger.WriteInfo(
                    TraceType,
                    "Adding Section={0} with {1} parameters.",
                    mergedSettingDictionary.ElementAt(i).Key,
                    mergedSettingDictionary.ElementAt(i).Value.Count);

                mergedSettings[i] = new SettingsOverridesTypeSection
                {
                    Name = mergedSettingDictionary.ElementAt(i).Key,
                    Parameter = mergedSettingDictionary.ElementAt(i).Value.ToArray()
                };
            }

            mergedSettings = mergedSettings.Where(setting => setting != null).ToArray();
            return mergedSettings;
        }

        private List<SettingsTypeSection> GenerateFileShareDcaSections()
        {
            List<SettingsTypeSection> sections = new List<SettingsTypeSection>();

            var diagnosticsSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.Diagnostics,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstances,
                        Value = "WinFabEtlFile, WinFabCrashDump, WinFabPerfCtrFolder"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerInstances,
                        Value = "FileShareWinFabEtw, FileShareWinFabCrashDump, FileShareWinFabPerfCtr"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ClusterId,
                        Value = this.ClusterId
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.EnableTelemetry,
                        Value = this.TargetCsmConfig.EnableTelemetry.ToString()
                    }
                }
            };
            sections.Add(diagnosticsSection);

            var winFabEtlFileSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.WinFabEtlFile,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = "EtlFileProducer"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString().ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(winFabEtlFileSection);

            var winFabCrashDumpSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.WinFabCrashDump,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = StringConstants.FolderProducer
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString().ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.FolderType,
                        Value = StringConstants.WindowsFabricCrashDumps
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(winFabCrashDumpSection);

            var winFabPerfCtrFolderSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.WinFabPerfCtrFolder,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = StringConstants.FolderProducer
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString().ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.FolderType,
                        Value = StringConstants.WindowsFabricPerformanceCounters
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(winFabPerfCtrFolderSection);

            var fileShareWinFabEtwSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.FileShareWinFabEtw,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = StringConstants.FileShareEtwCsvUploader
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = StringConstants.SectionName.WinFabEtlFile
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString().ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = Path.Combine(
                            this.TargetCsmConfig.DiagnosticsStoreInformation.Connectionstring,
                            string.Format("fabriclogs-{0}", this.ClusterId))
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(fileShareWinFabEtwSection);

            var fileShareWinFabCrashDumpSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.FileShareWinFabCrashDump,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = StringConstants.FileShareFolderUploader
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = StringConstants.SectionName.WinFabCrashDump
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString().ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = Path.Combine(
                            this.TargetCsmConfig.DiagnosticsStoreInformation.Connectionstring,
                            string.Format("fabricdumps-{0}", this.ClusterId))
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(fileShareWinFabCrashDumpSection);

            var fileShareWinFabPerfCtrSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.FileShareWinFabPerfCtr,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = StringConstants.FileShareFolderUploader
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = StringConstants.SectionName.WinFabPerfCtrFolder
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = true.ToString().ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = Path.Combine(
                            this.TargetCsmConfig.DiagnosticsStoreInformation.Connectionstring,
                            string.Format("fabricperf-{0}", this.ClusterId))
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(fileShareWinFabPerfCtrSection);

            return sections;
        }

        private List<SettingsTypeSection> GenerateBlobStoreDcaSections()
        {
            var sections = new List<SettingsTypeSection>();
            var dcaConnectionString = this.GenerateDcaConnectionString(this.diagnosticsStorageAccountConfig, this.diagnosticsStoreInformation);
            bool isDCAConnectionStringEncrypted = this.diagnosticsStoreInformation.IsEncrypted;

            var diagnosticsSection = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.Diagnostics,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstances,
                        Value = "ServiceFabricEtlFile, ServiceFabricEtlFileQueryable, ServiceFabricCrashDump, ServiceFabricPerfCounter"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerInstances,
                        Value = "AzureBlobServiceFabricEtw, AzureTableServiceFabricEtwQueryable, AzureBlobServiceFabricCrashDump, AzureBlobServiceFabricPerfCounter"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ClusterId,
                        Value = this.ClusterId
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.EnableTelemetry,
                        Value = this.TargetCsmConfig.EnableTelemetry.ToString()
                    }
                }
            };
            sections.Add(diagnosticsSection);

            var azureServiceFabricEtlFile = new SettingsTypeSection
            {
                Name = "ServiceFabricEtlFile",
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = "EtlFileProducer"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureServiceFabricEtlFile);

            var azureServiceFabricPerfCounter = new SettingsTypeSection
            {
                Name = "ServiceFabricPerfCounter",
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = "FolderProducer"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.FolderType,
                        Value = "WindowsFabricPerformanceCounters"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureServiceFabricPerfCounter);

            var azureServiceFabricCrashDump = new SettingsTypeSection
            {
                Name = "ServiceFabricCrashDump",
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = "FolderProducer"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.FolderType,
                        Value = "WindowsFabricCrashDumps"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureServiceFabricCrashDump);

            var azureServiceFabricEtlFileQueryable = new SettingsTypeSection
            {
                Name = StringConstants.SectionName.ServiceFabricEtlFileQueryable,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerType,
                        Value = "EtlFileProducer"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = "WindowsFabricEtlType",
                        Value = "QueryEtl"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureServiceFabricEtlFileQueryable);

            var azureBlobServiceFabricEtwSection = new SettingsTypeSection
            {
                Name =
                this.isDepreciatedDiagnosticsSectionNamesUsed
                ? StringConstants.SectionName.AzureBlobWinFabEtw
                : StringConstants.SectionName.AzureBlobServiceFabricEtw,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = dcaConnectionString,
                        IsEncrypted = isDCAConnectionStringEncrypted
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ContainerName,
                        Value = string.Format("fabriclogs-{0}", this.ClusterId).ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = "AzureBlobEtwCsvUploader"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = "ServiceFabricEtlFile"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureBlobServiceFabricEtwSection);

            var azureTableServiceFabricEtwQueryableSection = new SettingsTypeSection
            {
                Name =
                this.isDepreciatedDiagnosticsSectionNamesUsed
                ? StringConstants.SectionName.AzureTableWinFabEtwQueryable
                : StringConstants.SectionName.AzureTableServiceFabricEtwQueryable,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = dcaConnectionString,
                        IsEncrypted = isDCAConnectionStringEncrypted
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.TableNamePrefix,
                        Value = "fabriclog" + this.ClusterId.Replace("-", string.Empty).ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = "AzureTableQueryableEventUploader"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = "ServiceFabricEtlFileQueryable"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureTableServiceFabricEtwQueryableSection);

            var azureBlobServiceFabricCrashDumpSection = new SettingsTypeSection
            {
                Name =
                this.isDepreciatedDiagnosticsSectionNamesUsed
                ? StringConstants.SectionName.AzureBlobWinFabCrashDump
                : StringConstants.SectionName.AzureBlobServiceFabricCrashDump,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = dcaConnectionString,
                        IsEncrypted = isDCAConnectionStringEncrypted
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ContainerName,
                        Value = string.Format("fabriccrashdumps-{0}", this.ClusterId).ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = "AzureBlobFolderUploader"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = "ServiceFabricCrashDump",
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true",
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureBlobServiceFabricCrashDumpSection);

            var azureBlobServiceFabricPerfCounterSection = new SettingsTypeSection
            {
                Name =
                this.isDepreciatedDiagnosticsSectionNamesUsed
                ? StringConstants.SectionName.AzureBlobWinFabPerfCounter
                : StringConstants.SectionName.AzureBlobServiceFabricPerfCounter,
                Parameter = new[]
                {
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.StoreConnectionString,
                        Value = dcaConnectionString,
                        IsEncrypted = isDCAConnectionStringEncrypted
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ContainerName,
                        Value = string.Format("fabriccounters-{0}", this.ClusterId).ToLowerInvariant()
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ConsumerType,
                        Value = "AzureBlobFolderUploader"
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.ProducerInstance,
                        Value = "ServiceFabricPerfCounter",
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.IsEnabled,
                        Value = "true",
                    },
                    new SettingsTypeSectionParameter
                    {
                        Name = StringConstants.ParameterName.DataDeletionAgeInDays,
                        Value = this.TargetCsmConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays.ToString()
                    }
                }
            };
            sections.Add(azureBlobServiceFabricPerfCounterSection);

            return sections;
        }
    }
}