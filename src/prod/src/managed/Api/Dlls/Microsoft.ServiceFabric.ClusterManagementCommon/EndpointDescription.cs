// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;

    public enum EndpointProtocol
    {
        NotSpecified,
        Tcp,
        Http,
        Https,
    }

    public class EndpointDescription
    {
        public int Port { get; set; }

        public EndpointProtocol Protocol { get; set; }

        public override bool Equals(object other)
        {
            var otherEndpointDescription = other as EndpointDescription;
            if (otherEndpointDescription == null)
            {
                return false;
            }

            return (this.Port == otherEndpointDescription.Port) &&
                   (this.Protocol == otherEndpointDescription.Protocol);
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.Port.GetHashCode();
            hash = (13 * hash) + this.Protocol.GetHashCode();
            return hash;
        }

        public InputEndpointType ToInputEndpointType(InputEndpointTypeProtocol defaultProtocol)
        {
            return new InputEndpointType()
            {
                Port = this.Port.ToString(),
                Protocol = this.ToInputEndpointTypeProtocol(defaultProtocol)
            };
        }

        private InputEndpointTypeProtocol ToInputEndpointTypeProtocol(InputEndpointTypeProtocol defaultProtocol)
        {
            switch (this.Protocol)
            {
            case EndpointProtocol.Http:
                return InputEndpointTypeProtocol.http;

            case EndpointProtocol.Https:
                return InputEndpointTypeProtocol.https;

            case EndpointProtocol.Tcp:
                return InputEndpointTypeProtocol.tcp;

            case EndpointProtocol.NotSpecified:
                return defaultProtocol;

            default:
                throw new ArgumentOutOfRangeException(
                    "defaultProtocol",
                    this.Protocol,
                    "Cannot convert to InputEndpointTypeProtocol");
            }
        }
    }
}