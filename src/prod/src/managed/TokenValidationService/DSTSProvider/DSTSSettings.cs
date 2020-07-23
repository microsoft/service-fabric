// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;

    using Microsoft.WindowsAzure.Security.Authentication;
    using Microsoft.Win32;
    
    using Collections.Generic;

    internal sealed class DSTSSettings
    {
        private const string DSTSDnsNameKey = "DSTSDnsName";
        private const string DSTSRealmKey = "DSTSRealm";
        private const string TVSServiceDnsNameKey = "CloudServiceDnsName";
        private const string DSTSRelayServiceProtocolSchemeKey = "ProtocolScheme";
        private const string ServiceNameKey = "CloudServiceName";
        private const string TokenValidationServiceTypeKey = "TokenValidationServiceType";

        public DSTSSettings(string configSection)
        {
            this.DSTSSectionName = configSection;
            if (String.IsNullOrEmpty(this.TokenValidationServiceType))
            {
                this.TokenValidationServiceType = "DSTS";
            }
        }

        internal void LoadSettings(NativeConfigStore nativeConfig)
        {
            this.DSTSDnsName = nativeConfig.ReadUnencryptedString(DSTSSectionName, DSTSDnsNameKey);
            this.DSTSRealm = nativeConfig.ReadUnencryptedString(DSTSSectionName, DSTSRealmKey);
            this.ServiceDnsName = nativeConfig.ReadUnencryptedString(DSTSSectionName, TVSServiceDnsNameKey);
            this.ServiceName = nativeConfig.ReadUnencryptedString(DSTSSectionName, ServiceNameKey);
            this.TokenValidationServiceType = nativeConfig.ReadUnencryptedString(DSTSSectionName, TokenValidationServiceTypeKey);
        }

        public string DSTSSectionName
        {
            get;
            private set;
        }

        public string DSTSDnsName
        {
            get;
            private set;
        }

        public string DSTSRealm
        {
            get;
            private set;
        }

        public string ServiceDnsName
        {
            get;
            private set;
        }

        public string ServiceName
        {
            get;
            private set;
        }

        public string TokenValidationServiceType
        {
            get;
            private set;
        }

    }
}