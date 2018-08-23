// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    internal class TokenValidationServiceSettingsValidator
    {
        private Dictionary<string, Dictionary<string, WindowsFabricSettings.SettingsValue>> allTVSSettings;
        private Dictionary<string, WindowsFabricSettings.SettingsValue> httpGatewaySettings;

        private readonly ClusterManifestTypeNodeType[] nodeTypes;
        private List<InfrastructureNodeType> infrastructureInformation;
        private bool runAsPolicyEnabled;

        public bool IsTVSEnabled { get; private set; }

        public TokenValidationServiceSettingsValidator(
            Dictionary<string, Dictionary<string, WindowsFabricSettings.SettingsValue> > allTVSSettings,
            Dictionary<string, WindowsFabricSettings.SettingsValue> httpGatewaySettings,
            ClusterManifestTypeNodeType[] nodeTypes,
            List<InfrastructureNodeType> infrastructureInformation,
            bool runAsPolicyEnabled)
        {
            this.allTVSSettings = allTVSSettings;
            this.httpGatewaySettings = httpGatewaySettings;
            this.nodeTypes = nodeTypes;
            this.infrastructureInformation = infrastructureInformation;
            this.runAsPolicyEnabled = runAsPolicyEnabled;
            // Disabled by default
            this.IsTVSEnabled = false;
        }

        public void ValidateSettings()
        {
            if (this.allTVSSettings.Count > 0)
            {
                this.IsTVSEnabled = true;

                if (!this.runAsPolicyEnabled)
                {
                    throw new ArgumentException(
                        string.Format(
                            "RunAsPolicyEnabled should be set to true when enabling TokenValidationService"));
                }

                foreach (Dictionary<string, WindowsFabricSettings.SettingsValue> tvsSettings in this.allTVSSettings.Values)
                {
                    bool hasDSTSProvider = true;

                    if (tvsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.Providers))
                    {
                        var providers = tvsSettings[FabricValidatorConstants.ParameterNames.Providers].Value.Split(
                            new char[] { ',' },
                            StringSplitOptions.RemoveEmptyEntries);

                        hasDSTSProvider = (providers.Length < 1);

                        if (!hasDSTSProvider)
                        {
                            foreach (var provider in providers)
                            {
                                if (string.Equals(provider, FabricValidatorConstants.Provider_DSTS, StringComparison.OrdinalIgnoreCase))
                                {
                                    hasDSTSProvider = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!hasDSTSProvider)
                    {
                        break;
                    }

                    if (!tvsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.DSTSDnsName) ||
                        String.IsNullOrEmpty(tvsSettings[FabricValidatorConstants.ParameterNames.DSTSDnsName].Value))
                    {
                        throw new ArgumentException(
                            string.Format(
                                "ValidateTokenValidationServiceSettings failed as DSTSDnsName is not specified"));
                    }

                    if (!tvsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.DSTSRealm) ||
                        String.IsNullOrEmpty(tvsSettings[FabricValidatorConstants.ParameterNames.DSTSRealm].Value))
                    {
                        throw new ArgumentException(
                            string.Format(
                                        "ValidateTokenValidationServiceSettings failed as DSTSRealm is not specified"));
                    }

                    if (!tvsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.DSTSWinFabServiceDnsName) ||
                       String.IsNullOrEmpty(tvsSettings[FabricValidatorConstants.ParameterNames.DSTSWinFabServiceDnsName].Value))
                    {
                        throw new ArgumentException(
                            string.Format(
                                        "ValidateTokenValidationServiceSettings failed as DSTSMetadataServiceDnsName is not specified"));
                    }

                    if (!tvsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.DSTSWinFabServiceName) ||
                        String.IsNullOrEmpty(tvsSettings[FabricValidatorConstants.ParameterNames.DSTSWinFabServiceName].Value))
                    {
                        throw new ArgumentException(
                            string.Format(
                                        "ValidateTokenValidationServiceSettings failed as DSTSWinFabServiceName is not specified"));
                    }

                    if (tvsSettings.ContainsKey(FabricValidatorConstants.ParameterNames.PlacementConstraints))
                    {
                        FabricValidatorUtility.ValidateExpression(tvsSettings, FabricValidatorConstants.ParameterNames.PlacementConstraints);
                    }
                }

                //
                // Metadata endpoint is exposed via the httpgateway, so validate its settings according to TVS requirements.
                //
                ValidateHttpGatewaySettingsForTVS();
            }
        }

        private void ValidateHttpGatewaySettingsForTVS()
        {
            if (this.httpGatewaySettings == null)
            {
                throw new ArgumentException(
                    string.Format(
                        "ValidateTokenValidationServiceSettings failed as HttpGateway is not enabled"));
            }

            if (!httpGatewaySettings.ContainsKey(FabricValidatorConstants.ParameterNames.IsEnabled) ||
                String.IsNullOrEmpty(httpGatewaySettings[FabricValidatorConstants.ParameterNames.IsEnabled].Value))
            {
                throw new ArgumentException(
                    string.Format(
                        "ValidateTokenValidationServiceSettings failed as HttpGateway is not enabled"));
            }

            bool isHttpGatewayEnabled = false;
            if (!Boolean.TryParse(
                    this.httpGatewaySettings[FabricValidatorConstants.ParameterNames.IsEnabled].Value, 
                    out isHttpGatewayEnabled) ||
                !isHttpGatewayEnabled)
            {
                throw new ArgumentException(
                    string.Format(
                        "ValidateTokenValidationServiceSettings failed as HttpGateway is not enabled"));
            }

            if (infrastructureInformation == null)
            {
                foreach (ClusterManifestTypeNodeType nodeType in this.nodeTypes)
                {
                    if (nodeType.Endpoints == null || nodeType.Endpoints.HttpGatewayEndpoint == null)
                    {
                        throw new ArgumentException(
                            string.Format(
                            "ValidateTokenValidationServiceSettings failed as NodeType {0} does not have a HttpGatewayEndpoint defined",
                            nodeType.Name));
                    }
                }
            }
            else
            {
                foreach (var node in this.infrastructureInformation)
                {
                    if (node.Endpoints == null || node.Endpoints.HttpGatewayEndpoint == null)
                    {
                        throw new ArgumentException(
                            string.Format(
                            "ValidateTokenValidationServiceSettings failed as Node {0} does not have a HttpGatewayEndpoint defined",
                            node.NodeName));
                    }
                }
            }
        }
    }
}