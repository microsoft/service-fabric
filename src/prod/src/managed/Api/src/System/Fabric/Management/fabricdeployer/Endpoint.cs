// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.FabricDeployer
{
    using System;
    using System.Fabric.Management.ServiceModel;

    public class Endpoint
    {
        public Endpoint(string port, string protocol)
        {
            this.Port = port;
            this.Protocol = protocol;
        }

        public string Port { get; private set; }

        public string Protocol { get; private set; }

        public InternalEndpointType ToInternalEndpointType()
        {
            InternalEndpointType endpoint = new InternalEndpointType();
            endpoint.Port = this.Port;
            endpoint.Protocol = (InternalEndpointTypeProtocol)Enum.Parse(typeof(InternalEndpointTypeProtocol), this.Protocol);  
            return endpoint;
        }

        public InputEndpointType ToInputEndpointType()
        {
            InputEndpointType endpoint = new InputEndpointType();
            endpoint.Port = this.Port;
            endpoint.Protocol = (InputEndpointTypeProtocol)Enum.Parse(typeof(InputEndpointTypeProtocol), this.Protocol);
            return endpoint;
        }
    }
}