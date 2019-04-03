// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Testability.Common;

    [Serializable]
    public class FabricClientHelperInfo
    {
        public FabricClientHelperInfo(string name,
            string[] gatewayAddresses,
            string[] httpGatewayAddresses,
            SecurityCredentials securityCredentials,
            FabricClientSettings clientSettings,
            FabricClientTypes type)
        {
            ThrowIf.NullOrEmpty(name, "name");

            this.Name = name;

            if (gatewayAddresses != null && gatewayAddresses.Length > 0)
            {
                this.GatewayAddresses = gatewayAddresses;
            }

            if (httpGatewayAddresses != null && httpGatewayAddresses.Length > 0)
            {
                this.HttpGatewayAddresses = httpGatewayAddresses;
            }

            this.SecurityCredentials = securityCredentials;
            this.ClientSettings = clientSettings;
            this.ClientTypes = type;
        }

        public string Name
        {
            get;
            private set;
        }

        public IEnumerable<string> GatewayAddresses
        {
            get;
            private set;
        }

        public IEnumerable<string> HttpGatewayAddresses
        {
            get;
            private set;
        }

        public SecurityCredentials SecurityCredentials
        {
            get;
            private set;
        }

        public FabricClientSettings ClientSettings
        {
            get;
            private set;
        }

        public FabricClientTypes ClientTypes
        {
            get;
            private set;
        }
    }
}

#pragma warning restore 1591