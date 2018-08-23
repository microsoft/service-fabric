// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using YamlDotNet.RepresentationModel;

    //
    // Container endpoints are given as either containerport or HostPort:containerport
    // This can be given in one of the following formats,
    // 1. ContainerPort -(random host port will be chosen)
    // 2. HostPort:ContainerPort
    // 3. HostPort Range : ContainerPort Range
    // 4. ip:port : port
    // 5. HostPort : ContainerPort/protocol.
    //

    // Of these, for now we will support only 1, 2 and 5.

    internal class ComposeServicePorts : ComposeObjectDescription
    {
        public string HostPort;

        public string ContainerPort;

        public string Protocol;

        public ComposeServicePorts()
        {
        }

        public override void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys)
        {
            var portMapping = DockerComposeUtils.ValidateAndGetScalar(traceContext, "", node).ToString();
            var ports = portMapping.Split(DockerComposeConstants.SemiColonDelimiter[0]);

            if (ports.Length > 2)
            {
                throw new FabricComposeException(string.Format("{0} - Unable to parse Port - {1}", traceContext, portMapping));
            }

            if (ports.Length == 1)
            {
                this.ExtractProtocol(traceContext, portMapping, ports[0]);
            }
            else
            {
                this.HostPort = ports[0];
                if (string.IsNullOrEmpty(this.HostPort))
                {
                    throw new FabricComposeException(string.Format("{0} - Port invalid format - specified {1}", traceContext, portMapping));
                }

                this.ExtractProtocol(traceContext, portMapping, ports[1]);
            }
        }

        public override void Validate()
        {
            if (string.IsNullOrEmpty(this.ContainerPort))
            {
                throw new FabricComposeException("Port value cannot be empty.");
            }

            this.TryParseAsInt32(this.ContainerPort);
            if (!string.IsNullOrEmpty(this.HostPort))
            {
                this.TryParseAsInt32(this.HostPort);
            }
        }

        public new string ToString()
        {
            if (this.HostPort != null)
            {
                return $"{this.HostPort}:{this.ContainerPort}";
            }
            else
            {
                return this.ContainerPort;
            }
        }

        private void ExtractProtocol(string traceContext, string portMapping, string portProtocolValue)
        {
            var portProtocol = portProtocolValue.Split(DockerComposeConstants.ForwardSlashDelimiter[0]);
            if (portProtocol.Length > 2)
            {
                throw new FabricComposeException(string.Format("{0} - Unable to parse Port - {1}", traceContext, portMapping));
            }

            this.ContainerPort = portProtocol[0];
            if (portProtocol.Length == 2)
            {
                this.Protocol = portProtocol[1];
            }
        }

        private void TryParseAsInt32(string port)
        {
            try
            {
                Int32.Parse(port);
            }
            catch (Exception)
            {
                throw new FabricComposeException(string.Format("Unable to parse port {0} as integer", port));
            }
        }
    }
}