// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    internal class HttpGatewaySettingsValidator
    {
        private Dictionary<string, WindowsFabricSettings.SettingsValue> httpGatewaySettings;
        private readonly ClusterManifestTypeNodeType[] nodeTypes;
        private List<InfrastructureNodeType> infrastructureInformation;

        public bool IsHttpGatewayEnabled { get; private set; }

        public HttpGatewaySettingsValidator(ClusterManifestTypeNodeType[] nodeTypes,
            Dictionary<string, WindowsFabricSettings.SettingsValue> httpGatewaySettings,
            List<InfrastructureNodeType> infrastructureInformation)
        {
            this.httpGatewaySettings = httpGatewaySettings;
            // Disabled by default
            this.IsHttpGatewayEnabled = false;
            this.nodeTypes = nodeTypes;
            this.infrastructureInformation = infrastructureInformation;
        }

        public void ValidateHttpGatewaySettings()
        {
            if (this.httpGatewaySettings != null &&
            this.httpGatewaySettings.ContainsKey(FabricValidatorConstants.ParameterNames.IsEnabled))
            {
                this.IsHttpGatewayEnabled = Boolean.Parse(this.httpGatewaySettings[FabricValidatorConstants.ParameterNames.IsEnabled].Value);
            }
            if (this.IsHttpGatewayEnabled)
            {
                if (infrastructureInformation == null)
                {
                    foreach (ClusterManifestTypeNodeType nodeType in this.nodeTypes)
                    {
                        if (nodeType.Endpoints == null || nodeType.Endpoints.HttpGatewayEndpoint == null)
                        {
                            throw new ArgumentException(
                                 string.Format( 
                                "ValidateHttpGatewaySettings failed as NodeType {0} does not have a HttpGatewayEndpoint defined",
                                nodeType.Name));
                        }

                        ValidateEndpoint(nodeType.Endpoints.HttpGatewayEndpoint, nodeType.Certificates, true);
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
                                "ValidateHttpGatewaySettings failed as Node {0} does not have a HttpGatewayEndpoint defined",
                                node.NodeName));
                        }

                        //
                        // Infrastructureinformation for azure doesnt populate the certificate information.
                        //
                        ValidateEndpoint(node.Endpoints.HttpGatewayEndpoint, node.Certificates, false);
                    }
                }
            }
        }

        private void ValidateEndpoint(InputEndpointType httpEndpoint, CertificatesType certificates, bool verifyCertificates)
        {
            if (httpEndpoint.Protocol != InputEndpointTypeProtocol.http
                && httpEndpoint.Protocol != InputEndpointTypeProtocol.https)
            {
                throw new ArgumentException(
                    string.Format(
                    "ValidateHttpGatewaySettings failed as HttpGatewayEndpoint.Protocol is invalid: {0}",
                    httpEndpoint.Protocol));
            }

            if (!verifyCertificates)
            {
                return;
            }

            //
            // If protocol is https, then server certificates have to be specified.
            //
            if (httpEndpoint.Protocol == InputEndpointTypeProtocol.https)
            {
                if (certificates == null ||
                    certificates.ServerCertificate == null ||
                    certificates.ServerCertificate.X509FindValue == null ||
                    certificates.ServerCertificate.X509FindValue.Length == 0)
                {
                    throw new ArgumentException(
                        "ValidateHttpGatewaySettings failed as ServerCertificate is not specified with HttpGatewayEndpoint Protocol https");
                }
            }

            //
            // If server certificates are specified, the protocol should be https.
            //
            if (certificates != null &&
                certificates.ServerCertificate != null &&
                certificates.ServerCertificate.X509FindValue != null &&
                certificates.ServerCertificate.X509FindValue.Length != 0)
            {
                if (httpEndpoint.Protocol != InputEndpointTypeProtocol.https)
                {
                    throw new ArgumentException(
                        "ValidateHttpGatewaySettings failed as ServerCertificate is specified and the HttpGatewayEndpoint Protocol is not https");
                }
            }
        }
    }
}