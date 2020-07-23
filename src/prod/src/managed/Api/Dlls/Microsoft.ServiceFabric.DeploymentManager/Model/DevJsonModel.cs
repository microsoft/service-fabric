// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;    
    using System.Linq;
    using System.Runtime.Serialization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Newtonsoft.Json;

    internal sealed class DevJsonModel : StandAloneInstallerJSONModelOctober2017
    {
        private static List<string> sectionsToDropFor5Node = new List<string>
        {
            StringConstants.SectionName.UpgradeOrchestrationService,
            StringConstants.SectionName.FileStoreService,
            StringConstants.SectionName.Common,
            StringConstants.SectionName.WinFabCrashDump,
            StringConstants.SectionName.ServiceFabricCrashDump
        };

        private static List<string> sectionsToDropFor1Node = new List<string>
        {
            StringConstants.SectionName.UpgradeOrchestrationService,
            StringConstants.SectionName.FileStoreService,
            StringConstants.SectionName.Common,
            StringConstants.SectionName.FaultAnalysisService,
            StringConstants.SectionName.WinFabCrashDump,
            StringConstants.SectionName.ServiceFabricCrashDump
        };

        private static List<string> securityParamsToDrop = new List<string>
        {
            StringConstants.ParameterName.ClientRoleEnabled,
            StringConstants.ParameterName.DisableFirewallRuleForDomainProfile,
            StringConstants.ParameterName.DisableFirewallRuleForPrivateProfile,
            StringConstants.ParameterName.DisableFirewallRuleForPublicProfile,
            StringConstants.ParameterName.AdminClientCertThumbprints,
            StringConstants.ParameterName.AllowDefaultClient,
            StringConstants.ParameterName.ClientCertThumbprints,
            StringConstants.ParameterName.ClusterCertThumbprints,            
            StringConstants.ParameterName.ServerCertThumbprints
        };

        private static Dictionary<string, string> fabricclient1NodeOverride = new Dictionary<string, string>
        {
            { "HealthReportSendInterval", "0" }
        };

        private static Dictionary<string, string> failover1NodeNonSecureOverride = new Dictionary<string, string>
        {
            { "SendToFMTimeout", "1" },
            { "NodeUpRetryInterval", "1" }
        };

        private static Dictionary<string, string> management5NodeOverride = new Dictionary<string, string>
        {
            { "ImageStoreConnectionString", "ImageStoreConnectionStringPlaceHolder" },
            { "ImageCachingEnabled", "false" },
            { "EnableDeploymentAtDataRoot", "true" }
        };

        private static Dictionary<string, string> management1NodeOverride = new Dictionary<string, string>
        {
            { "ImageStoreConnectionString", "ImageStoreConnectionStringPlaceHolder" },
            { "ImageCachingEnabled", "false" },
            { "EnableDeploymentAtDataRoot", "true" },
            { "DisableChecksumValidation", "true" }
        };    

        private static Dictionary<string, string> clustermanagerOverride = new Dictionary<string, string>
        {
            { "UpgradeStatusPollInterval", "5" },
            { "UpgradeHealthCheckInterval", "5" },
            { "FabricUpgradeHealthCheckInterval", "5" }
        };

        private static Dictionary<string, string> diagnosticsOverride = new Dictionary<string, string>
        {
            { "MaxDiskQuotaInMB", "10240" },
            { "EnableCircularTraceSession", "true" }
        };

        private static Dictionary<string, string> failovermanager5NodeOverride = new Dictionary<string, string>
        {
            { "ExpectedClusterSize", "4" },
            { "ReconfigurationTimeLimit", "20" },
            { "BuildReplicaTimeLimit", "20" },
            { "CreateInstanceTimeLimit", "20" },
            { "PlacementTimeLimit", "20" }
        };

        private static Dictionary<string, string> failovermanager1NodeOverride = new Dictionary<string, string>
        {
            { "ExpectedClusterSize", "1" },
            { "ClusterStableWaitDuration", "0" },
            { "PeriodicStateScanInterval", "1" },
            { "ReconfigurationTimeLimit", "20" },
            { "BuildReplicaTimeLimit", "20" },
            { "CreateInstanceTimeLimit", "20" },
            { "PlacementTimeLimit", "20" }
        };

        private static Dictionary<string, string> failovermanager1NodeNonsecureOverride = new Dictionary<string, string>
        {
            { "ExpectedClusterSize", "1" },
            { "ClusterStableWaitDuration", "0" },
            { "PeriodicStateScanInterval", "1" },
            { "ReconfigurationTimeLimit", "20" },
            { "BuildReplicaTimeLimit", "20" },
            { "CreateInstanceTimeLimit", "20" },
            { "PlacementTimeLimit", "20" },
            { "ServiceLocationBroadcastInterval", "1" },
            { "ServiceLookupTableEmptyBroadcastInterval", "1" },
            { "MinRebuildRetryInterval", "1" },
            { "MaxRebuildRetryInterval", "1" }
        };

        private static Dictionary<string, string> federationOverride = new Dictionary<string, string>
        {
            { "UnresponsiveDuration", "0" },
            { "ProcessAssertExitTimeout", "86400" }
        };

        private static Dictionary<string, string> hosting5NodeOverride = new Dictionary<string, string>
        {
            { "EndpointProviderEnabled", "true" },
            { "RunAsPolicyEnabled", "true" },
            { "EnableProcessDebugging", "true" },
            { "DeactivationScanInterval", "60" },
            { "DeactivationGraceInterval", "10" },
            { "ServiceTypeRegistrationTimeout", "20" },
            { "CacheCleanupScanInterval", "300" }
        };

        private static Dictionary<string, string> hosting1NodeOverride = new Dictionary<string, string>
        {
            { "EndpointProviderEnabled", "true" },
            { "RunAsPolicyEnabled", "true" },
            { "EnableProcessDebugging", "true" },
            { "DeactivationScanInterval", "600" },
            { "DeactivationGraceInterval", "2" },
            { "ServiceTypeRegistrationTimeout", "20" },
            { "CacheCleanupScanInterval", "300" },
            { "DeploymentRetryBackoffInterval", "1" }
        };

        private static Dictionary<string, string> reconfigurationAgent5NodeNonSecureOverride = new Dictionary<string, string>
        {
            { "ServiceApiHealthDuration", "20" },
            { "ServiceReconfigurationApiHealthDuration", "20" },
            { "LocalHealthReportingTimerInterval", "5" },
            { "RAUpgradeProgressCheckInterval", "3" }
        };

        private static Dictionary<string, string> reconfigurationAgent5NodeSecureOverride = new Dictionary<string, string>
        {
            { "ServiceApiHealthDuration", "20" },
            { "ServiceReconfigurationApiHealthDuration", "20" },
            { "LocalHealthReportingTimerInterval", "5" },
            { "RAUpgradeProgressCheckInterval", "3" },
            { "IsDeactivationInfoEnabled", "true" }
        };

        private static Dictionary<string, string> reconfigurationAgent1NodeNonSecureOverride = new Dictionary<string, string>
        {
            { "ServiceApiHealthDuration", "20" },
            { "ServiceReconfigurationApiHealthDuration", "20" },
            { "LocalHealthReportingTimerInterval", "5" },
            { "RAUpgradeProgressCheckInterval", "3" },
            { "RAPMessageRetryInterval", "0.5" },
            { "MinimumIntervalBetweenRAPMessageRetry", "0.5" }
        };

        private static Dictionary<string, string> reconfigurationAgent1NodeSecureOverride = new Dictionary<string, string>
        {
            { "ServiceApiHealthDuration", "20" },
            { "ServiceReconfigurationApiHealthDuration", "20" },
            { "LocalHealthReportingTimerInterval", "5" },
            { "RAUpgradeProgressCheckInterval", "3" },
            { "RAPMessageRetryInterval", "0.5" },
            { "MinimumIntervalBetweenRAPMessageRetry", "0.5" }
        };

        private static Dictionary<string, string> securityNonSecureOverride = new Dictionary<string, string>
        {
            { "ClusterCredentialType", "None" },
            { "ServerAuthCredentialType", "None" }
        };

        private static Dictionary<string, string> securitySecureOverride = new Dictionary<string, string>
        {
            { "ClusterCredentialType", "X509" },
            { "ServerAuthCredentialType", "X509" },
            { "ClientAuthAllowedCommonNames", "ServiceFabricDevClusterCert" },
            { "ServerAuthAllowedCommonNames", "ServiceFabricDevClusterCert" },
            { "ClusterAllowedCommonNames", "ServiceFabricDevClusterCert" }
        };

        private static Dictionary<string, string> transactionalReplicatorOverride = new Dictionary<string, string>
        {
            { "CheckpointThresholdInMB", "64" }
        };

        private static Dictionary<string, string> placementandloadbalancingOverride = new Dictionary<string, string>
        {
            { "MinLoadBalancingInterval", "300" },
            { "TraceCRMReasons", "false" }
        };

        private static Dictionary<string, string> winfabetlfileOverride = new Dictionary<string, string>
        {
            { "EtlReadIntervalInMinutes", "5" }
        };

        private static Dictionary<string, string> imagestoreserviceOverride = new Dictionary<string, string>
        {
            { "TargetReplicaSetSize", "1" },
            { "MinReplicaSetSize", "1" }
        };

        private static Dictionary<string, string> namingservice1NodeOverride = new Dictionary<string, string>
        {
            { "PartitionCount", "1" }
        };

        private static Dictionary<string, string> applicationgatewaySecureOverride = new Dictionary<string, string>
        {
            { "GatewayX509CertificateFindValue", "ServiceFabricDevClusterCert" },
            { "GatewayX509CertificateFindType", "FindBySubjectName" },
            { "ApplicationCertificateValidationPolicy", "None" }
        };

        private static Dictionary<string, string> dnsServiceOverride = new Dictionary<string, string>
        {
            { "AllowMultipleListeners", "true" },
            { "EnablePartitionedQuery", "true" }
        };

        private static Dictionary<string, List<Dictionary<string, string>>> settingsList = new Dictionary<string, List<Dictionary<string, string>>>
        {
            // { section, {five_nonsecure, five_nonesecur_win7, five_secure, five_secure_win7, one_nonsecure, one_nonsecure_win7, one_secure, one_secure_win7}
            { StringConstants.SectionName.FabricClient, new List<Dictionary<string, string>> { null, null, null, null, fabricclient1NodeOverride, fabricclient1NodeOverride, fabricclient1NodeOverride, fabricclient1NodeOverride } },
            { StringConstants.SectionName.Failover, new List<Dictionary<string, string>> { null, null, null, null, failover1NodeNonSecureOverride, null, null, null } },
            { StringConstants.SectionName.Management, new List<Dictionary<string, string>> { management5NodeOverride, management5NodeOverride, management5NodeOverride, management5NodeOverride, management1NodeOverride, management1NodeOverride, management1NodeOverride, management1NodeOverride } },
            { StringConstants.SectionName.ClusterManager, new List<Dictionary<string, string>> { clustermanagerOverride, clustermanagerOverride, clustermanagerOverride, clustermanagerOverride, clustermanagerOverride, clustermanagerOverride, clustermanagerOverride, clustermanagerOverride } },
            { StringConstants.SectionName.Diagnostics, new List<Dictionary<string, string>> { diagnosticsOverride, diagnosticsOverride, diagnosticsOverride, diagnosticsOverride, diagnosticsOverride, diagnosticsOverride, diagnosticsOverride, diagnosticsOverride } },
            { StringConstants.SectionName.FailoverManager, new List<Dictionary<string, string>> { failovermanager5NodeOverride, failovermanager5NodeOverride, failovermanager5NodeOverride, failovermanager5NodeOverride, failovermanager1NodeNonsecureOverride, failovermanager1NodeOverride, failovermanager1NodeOverride, failovermanager1NodeOverride } },
            { StringConstants.SectionName.Federation, new List<Dictionary<string, string>> { federationOverride,  federationOverride,  federationOverride,  federationOverride,  federationOverride,  federationOverride,  federationOverride,  federationOverride } },
            { StringConstants.SectionName.Hosting, new List<Dictionary<string, string>> { hosting5NodeOverride, hosting5NodeOverride, hosting5NodeOverride, hosting5NodeOverride, hosting1NodeOverride, hosting1NodeOverride, hosting1NodeOverride, hosting1NodeOverride } },
            { StringConstants.SectionName.ReconfigurationAgent, new List<Dictionary<string, string>> { reconfigurationAgent5NodeNonSecureOverride, reconfigurationAgent5NodeNonSecureOverride, reconfigurationAgent5NodeSecureOverride,  reconfigurationAgent5NodeSecureOverride, reconfigurationAgent1NodeNonSecureOverride, reconfigurationAgent1NodeNonSecureOverride, reconfigurationAgent1NodeSecureOverride, reconfigurationAgent1NodeSecureOverride } },
            { StringConstants.SectionName.Security, new List<Dictionary<string, string>> { securityNonSecureOverride, securityNonSecureOverride, securitySecureOverride, securitySecureOverride, securityNonSecureOverride, securityNonSecureOverride, securitySecureOverride, securitySecureOverride } },
            { StringConstants.SectionName.TransactionalReplicator, new List<Dictionary<string, string>> { transactionalReplicatorOverride,  transactionalReplicatorOverride,  transactionalReplicatorOverride,  transactionalReplicatorOverride,  transactionalReplicatorOverride,  transactionalReplicatorOverride,  transactionalReplicatorOverride,  transactionalReplicatorOverride } },
            { StringConstants.SectionName.PlacementAndLoadBalancing, new List<Dictionary<string, string>> { placementandloadbalancingOverride,  placementandloadbalancingOverride,  placementandloadbalancingOverride,  placementandloadbalancingOverride,  placementandloadbalancingOverride,  placementandloadbalancingOverride,  placementandloadbalancingOverride,  placementandloadbalancingOverride } },
            { StringConstants.SectionName.WinFabEtlFile, new List<Dictionary<string, string>> { winfabetlfileOverride,  winfabetlfileOverride,  winfabetlfileOverride,  winfabetlfileOverride,  winfabetlfileOverride,  winfabetlfileOverride,  winfabetlfileOverride,  winfabetlfileOverride } },
            { StringConstants.SectionName.ImageStoreService, new List<Dictionary<string, string>> { imagestoreserviceOverride,  imagestoreserviceOverride,  imagestoreserviceOverride,  imagestoreserviceOverride,  imagestoreserviceOverride,  imagestoreserviceOverride,  imagestoreserviceOverride,  imagestoreserviceOverride } },
            { StringConstants.SectionName.NamingService, new List<Dictionary<string, string>> { null,  null, null, null, namingservice1NodeOverride, namingservice1NodeOverride, namingservice1NodeOverride, namingservice1NodeOverride } },
            { StringConstants.SectionName.HttpApplicationGateway, new List<Dictionary<string, string>> { null, null, applicationgatewaySecureOverride, null, null, null, applicationgatewaySecureOverride, null } },
            { StringConstants.SectionName.DnsService, new List<Dictionary<string, string>> { dnsServiceOverride, dnsServiceOverride, dnsServiceOverride, dnsServiceOverride, dnsServiceOverride, dnsServiceOverride, dnsServiceOverride, dnsServiceOverride } },
        };

        // Major.Minor version for: Win7 is 6.1, Win8 is 6.2, Win8.1 is 6.3, Win10 is 10.0
        internal static bool IsWin7()
        {
            bool isWin7 = false;

            if (System.Environment.OSVersion.Version.Major < 6)
            {
                isWin7 = true;
            }

            if (System.Environment.OSVersion.Version.Major == 6)
            {
                if (System.Environment.OSVersion.Version.Minor == 1)
                {
                    isWin7 = true;
                }
            }

            return isWin7;
        }

        internal static bool IsSecureCluster(SettingsOverridesTypeSection[] fabricsettings)
        {
            bool isSecure = false;
            SettingsOverridesTypeSection setting = fabricsettings.FirstOrDefault(x => x.Name == StringConstants.SectionName.Security);
            SettingsOverridesTypeSectionParameter clustercredentialtypeparam = setting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.ClusterCredentialType);
            if (clustercredentialtypeparam.Value != "None")
            {
                isSecure = true;
            }

            return isSecure;
        }

        internal static void ApplyParamOverride(List<SettingsOverridesTypeSection> targetFabricSettings, int nodeCount, bool isSecure, bool isWin7)
        {
            int offset = 0;

            if (isSecure)
            {
                offset += 2;
            }

            if (isWin7)
            {
                offset += 1;
            }

            foreach (var item in settingsList)
            {
                Dictionary<string, string> paramOverride = null;
                switch (nodeCount) 
                {
                    case 5:
                        {
                            paramOverride = item.Value[offset];
                            break;
                        }

                    case 1:
                        {
                            paramOverride = item.Value[4 + offset];
                            break;
                        }

                    default:
                        {
                            throw new ValidationException(ClusterManagementErrorCode.DevClusterSizeNotSupported, nodeCount);
                        }
                }

                if (paramOverride == null)
                {
                    continue;
                }

                string settingName = item.Key;
                SettingsOverridesTypeSection setting = targetFabricSettings.FirstOrDefault(x => x.Name == settingName);
                if (setting == null)
                {
                    setting = new SettingsOverridesTypeSection
                    {
                        Name = settingName
                    };
                    targetFabricSettings.Add(setting);
                }

                List<SettingsOverridesTypeSectionParameter> parameters = null;

                if (setting.Parameter == null)
                {
                    parameters = new List<SettingsOverridesTypeSectionParameter>();
                }
                else
                {
                    parameters = setting.Parameter.ToList();
                }

                foreach (var paramItem in paramOverride)
                {
                    SettingsOverridesTypeSectionParameter param = parameters.FirstOrDefault(x => x.Name == paramItem.Key);
                    if (param != null)
                    {
                        param.Value = paramItem.Value;
                    }
                    else
                    {
                        param = new SettingsOverridesTypeSectionParameter
                        {
                            Name = paramItem.Key,
                            Value = paramItem.Value
                        };
                        parameters.Add(param);
                    }
                }

                setting.Parameter = parameters.ToArray();
            }
        }

        internal static void ApplyUserSettings(List<SettingsOverridesTypeSection> targetFabricSettings, List<SettingsSectionDescription> userFabricSettings)
        {
            foreach (SettingsSectionDescription section in userFabricSettings)
            {
                var targetSection = targetFabricSettings.FirstOrDefault(x => x.Name == section.Name);
                if (targetSection == null)
                {
                    targetSection = new SettingsOverridesTypeSection
                    {
                        Name = section.Name,
                        Parameter = new SettingsOverridesTypeSectionParameter[0]
                    };

                    targetFabricSettings.Add(targetSection);
                }

                List<SettingsOverridesTypeSectionParameter> targetParamList = targetSection.Parameter.ToList();
                foreach (SettingsParameterDescription param in section.Parameters)
                {                        
                    var targetParam = targetParamList.FirstOrDefault(x => x.Name == param.Name);

                    if (targetParam != null)
                    {
                        targetParam.Value = param.Value;
                    }
                    else
                    {
                        targetParamList.Add(new SettingsOverridesTypeSectionParameter
                        {
                            Name = param.Name,
                            Value = param.Value
                        });
                    }                        
                }

                targetSection.Parameter = targetParamList.ToArray();
            }
        }

        internal static bool IsSectionToDrop(int nodeCount, string settingName)
        {
            switch (nodeCount)
            {
                case 5:
                {
                    return sectionsToDropFor5Node.Contains(settingName);
                }

                case 1:
                {
                    return sectionsToDropFor1Node.Contains(settingName);
                }

                default:
                {
                    throw new ValidationException(ClusterManagementErrorCode.DevClusterSizeNotSupported, nodeCount);
                }
            }
        }

        // The FabricSettings require some hard coding to make the generated clustermanifest exactly the same as sdk clustermanifest
        internal static void TuneGeneratedManifest(ClusterManifestType clustermanifest, List<SettingsSectionDescription> userFabricSettings)
        {
            ClusterManifestTypeInfrastructureWindowsServer winserverinfra = clustermanifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
            if (winserverinfra != null)
            {
                if (clustermanifest.NodeTypes.Count() == 1)
                {
                    winserverinfra.IsScaleMin = true;
                }
                else if (clustermanifest.NodeTypes.Count() == 5)
                {
                    // Manually set the last two nodes to be non-seed nodes.
                    winserverinfra.NodeList[3].IsSeedNode = false;
                    winserverinfra.NodeList[4].IsSeedNode = false;
                }
            }

            List<SettingsOverridesTypeSection> targetFabricSettings = new List<SettingsOverridesTypeSection>();
            List<string> consumerInstances = new List<string>();
            List<string> producerInstances = new List<string>();
            foreach (SettingsOverridesTypeSection sourceSetting in clustermanifest.FabricSettings)
            {
                if (IsSectionToDrop(clustermanifest.NodeTypes.Count(), sourceSetting.Name))
                {
                    continue;
                }
                else if (sourceSetting.Name == StringConstants.SectionName.Diagnostics)
                {
                    List<SettingsOverridesTypeSectionParameter> parameters = new List<SettingsOverridesTypeSectionParameter>();
                    SettingsOverridesTypeSectionParameter param = sourceSetting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.ConsumerInstances);
                    if (param != null)
                    {
                        foreach (string item in param.Value.Split(','))
                        {
                            consumerInstances.Add(item.Trim());
                        }
                    }

                    param = sourceSetting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.ProducerInstances);
                    if (param != null)
                    {
                        foreach (string item in param.Value.Split(','))
                        {
                            if (!item.Contains("CrashDump"))
                            {
                                producerInstances.Add(item.Trim().Replace("WinFab", "ServiceFabric"));
                            }
                            else
                            {
                                consumerInstances.Add(item.Trim().Replace("ServiceFabric", "WinFab"));
                            }
                        }

                        param.Value = string.Join(", ", producerInstances.ToArray());
                        parameters.Add(param);
                    }

                    param = sourceSetting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.MaxDiskQuotaInMb);
                    if (param != null)
                    {
                        parameters.Add(param);
                    }

                    param = sourceSetting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.EnableCircularTraceSession);
                    if (param != null)
                    {
                        parameters.Add(param);
                    }

                    sourceSetting.Parameter = parameters.ToArray();
                } 
                else if (sourceSetting.Name == StringConstants.SectionName.Hosting)
                {
                    List<SettingsOverridesTypeSectionParameter> parameters = sourceSetting.Parameter.ToList();
                    SettingsOverridesTypeSectionParameter param = parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.FirewallPolicyEnabled);
                    parameters.Remove(param);
                    sourceSetting.Parameter = parameters.ToArray();
                }
                else if (sourceSetting.Name == StringConstants.SectionName.Security)
                {
                    List<SettingsOverridesTypeSectionParameter> parameters = sourceSetting.Parameter.ToList();
                    foreach (string securityparam in securityParamsToDrop)
                    {
                        SettingsOverridesTypeSectionParameter param = parameters.FirstOrDefault(x => x.Name == securityparam);
                        parameters.Remove(param);
                    }

                    sourceSetting.Parameter = parameters.ToArray();
                }
                else if (sourceSetting.Name == StringConstants.SectionName.HttpApplicationGateway)
                {
                    List<SettingsOverridesTypeSectionParameter> parameters = sourceSetting.Parameter.ToList();
                    SettingsOverridesTypeSectionParameter param = parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.GatewayX509CertificateStoreName);
                    parameters.Remove(param);
                    sourceSetting.Parameter = parameters.ToArray();
                }
                else if (sourceSetting.Name == StringConstants.SectionName.PlacementAndLoadBalancing)
                {
                    List<SettingsOverridesTypeSectionParameter> parameters = sourceSetting.Parameter.ToList();
                    SettingsOverridesTypeSectionParameter param = parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.QuorumBasedReplicaDistributionPerUpgradeDomains);
                    parameters.Remove(param);
                    sourceSetting.Parameter = parameters.ToArray();
                }
                else if (sourceSetting.Name == StringConstants.SectionName.ClusterManager)
                {
                    List<SettingsOverridesTypeSectionParameter> parameters = sourceSetting.Parameter.ToList();
                    SettingsOverridesTypeSectionParameter param = parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.EnableAutomaticBaseline);
                    parameters.Remove(param);
                    sourceSetting.Parameter = parameters.ToArray();
                }

                List<SettingsOverridesTypeSectionParameter> paramList = sourceSetting.Parameter.ToList();
                SettingsOverridesTypeSectionParameter placementConstraintsparam = paramList.FirstOrDefault(x => x.Name == StringConstants.ParameterName.PlacementConstraints);
                paramList.Remove(placementConstraintsparam);                

                SettingsOverridesTypeSectionParameter targetReplicaSetSize = paramList.FirstOrDefault(x => x.Name == StringConstants.ParameterName.TargetReplicaSetSize);
                if (targetReplicaSetSize != null)
                {
                    targetReplicaSetSize.Value = Math.Min(Convert.ToInt32(targetReplicaSetSize.Value), 3).ToString();
                }

                sourceSetting.Parameter = paramList.ToArray();

                targetFabricSettings.Add(sourceSetting);
            }

            ApplyParamOverride(targetFabricSettings, clustermanifest.NodeTypes.Count(), IsSecureCluster(clustermanifest.FabricSettings), IsWin7());            

            foreach (string settingName in consumerInstances)
            {
                SettingsOverridesTypeSection item = targetFabricSettings.FirstOrDefault(x => x.Name == settingName);
                targetFabricSettings.Remove(item);
            }

            foreach (string settingName in producerInstances)
            {
                SettingsOverridesTypeSection settingLegacy = targetFabricSettings.FirstOrDefault(x => x.Name == settingName.Replace("ServiceFabric", "WinFab"));
                if (settingLegacy == null)
                {
                    continue;
                }

                SettingsOverridesTypeSection settingNew = targetFabricSettings.FirstOrDefault(x => x.Name == settingName);

                if (settingNew == null)
                {
                    settingLegacy.Name = settingName;

                    SettingsOverridesTypeSectionParameter param = settingLegacy.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.FolderType);
                    if (param != null)
                    {
                        param.Value = param.Value.Replace("WindowsFabric", "ServiceFabric");
                    }
                }
                else
                {
                    List<SettingsOverridesTypeSectionParameter> paramNewList = settingNew.Parameter.ToList();
                    foreach (SettingsOverridesTypeSectionParameter paramLegacy in settingLegacy.Parameter)
                    {
                        SettingsOverridesTypeSectionParameter param = paramNewList.FirstOrDefault(x => x.Name == paramLegacy.Name);
                        if (param == null)
                        {
                            paramNewList.Add(paramLegacy);
                        }
                    }
                    
                    targetFabricSettings.Remove(settingLegacy);
                }             
            }
            
            ApplyUserSettings(targetFabricSettings, userFabricSettings);
            clustermanifest.FabricSettings = targetFabricSettings.ToArray();
        }

        internal static bool IsDevCluster(StandAloneInstallerJsonModelBase jsonConfig)
        {
            if (typeof(DevJsonModel).BaseType != jsonConfig.GetType())
            {
                return false;
            }

            Dictionary<string, string> fabricsettings = jsonConfig.GetFabricSystemSettings();

            if (fabricsettings.ContainsKey(StringConstants.ParameterName.IsDevCluster) &&
                string.Equals("true", fabricsettings[StringConstants.ParameterName.IsDevCluster], StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }

            return false;
        }

        [OnDeserialized]
        internal void OnDeserializedMethod(StreamingContext context)
        {
            SettingsSectionDescription setupSetting = this.Properties.FabricSettings.FirstOrDefault(x => x.Name == StringConstants.SectionName.Setup);
            if (setupSetting == null)
            {
                return;
            }

            SettingsParameterDescription dataRootParam = setupSetting.Parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.FabricDataRoot);
            if (dataRootParam != null)
            {
                dataRootParam.Value = Environment.ExpandEnvironmentVariables(dataRootParam.Value);
            }

            SettingsParameterDescription logRootParam = setupSetting.Parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.FabricLogRoot);
            if (logRootParam != null)
            {
                logRootParam.Value = Environment.ExpandEnvironmentVariables(logRootParam.Value);
            }

            SettingsParameterDescription devClusterParam = setupSetting.Parameters.FirstOrDefault(x => x.Name == StringConstants.ParameterName.IsDevCluster);
            setupSetting.Parameters.Remove(devClusterParam);
            
            this.Properties.DiagnosticsStore.Connectionstring = Environment.ExpandEnvironmentVariables(this.Properties.DiagnosticsStore.Connectionstring);
        }
    }
}