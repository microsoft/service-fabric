// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Server
{
    using LogProvider;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Security.Cryptography.X509Certificates;

    internal sealed class SecuritySetting : IConfigStoreUpdateHandler
    {
        private const string SecurityConfigSectionName = "Security";
        private const string SecurityServerX509NamesConfigSectionName = "Security/ServerX509Names";
        private const string FabricNodeConfigSectionName = "FabricNode";
        private const string ServerAuthCredentialTypeName = "ServerAuthCredentialType";
        private const string ServerCertThumbprints = "ServerCertThumbprints";
        private const string ClusterIdentities = "ClusterIdentities";
        private const string ServerAuthX509StoreName = "ServerAuthX509StoreName";
        private const string ServerAuthX509FindType = "ServerAuthX509FindType";
        private const string ServerAuthX509FindValue = "ServerAuthX509FindValue";
        private const string ServerAuthX509FindValueSecondary = "ServerAuthX509FindValueSecondary";

        private static readonly char[] ConfigParamValueSeparator = new char[] { ',' };

        internal CredentialType ServerAuthCredentialType { get; }

        internal X509FindType AuthX509FindType { get; private set; }

        internal HashSet<string> AllowedClientCertThumbprints { get; private set; }

        internal HashSet<string> AllowedClientIdentities { get; private set; }

        internal Dictionary<string, HashSet<string>> AllowedClientCertCommonNames { get; private set; }

        internal EndpointProtocol EndpointProtocol { get; }

        private readonly NativeConfigStore configStore;

        private static SecuritySetting instance = null;

        private static object instanceCreationLockObject = new object();

        private SecuritySetting()
        {
            this.configStore = NativeConfigStore.FabricGetConfigStore(this);
            string serverAuthCredentialTypeName = configStore.ReadUnencryptedString(SecurityConfigSectionName, ServerAuthCredentialTypeName);
            CredentialType serverAuthCredentialType = CredentialType.None;

            if ((!string.IsNullOrEmpty(serverAuthCredentialTypeName)) &&
                (!Enum.TryParse<CredentialType>(serverAuthCredentialTypeName, out serverAuthCredentialType)))
            {
                EventStoreLogger.Logger.LogWarning(
                    "Unable to convert configuration value serverAuthCredentialTypeName {0} for {1} type.",
                    serverAuthCredentialTypeName,
                    serverAuthCredentialType);
            }

            this.ServerAuthCredentialType = serverAuthCredentialType;
            if (ServerAuthCredentialType == CredentialType.X509)
            {
                this.EndpointProtocol = EndpointProtocol.Https;
            }
            else
            {
                this.EndpointProtocol = EndpointProtocol.Http;
            }

            this.AllowedClientCertThumbprints = null;
            this.AllowedClientIdentities = null;
            this.AllowedClientCertCommonNames = null;

            this.InitializeFromConfig();
        }

        private SecuritySetting(EndpointProtocol endpointProtocol)
        {
            this.ServerAuthCredentialType = endpointProtocol == EndpointProtocol.Https ? CredentialType.X509 : CredentialType.None;
            this.EndpointProtocol = endpointProtocol;
        }

        internal static SecuritySetting GetClusterSecurityDetails()
        {
            if (instance == null)
            {
                lock (instanceCreationLockObject)
                {
                    if (instance == null)
                    {
                        instance = new SecuritySetting();
                    }
                }
            }

            return instance;
        }

        internal static SecuritySetting GetClusterSecurityForTesting(EndpointProtocol endpointProtocol)
        {
            if (instance == null)
            {
                lock (instanceCreationLockObject)
                {
                    if (instance == null)
                    {
                        instance = new SecuritySetting(endpointProtocol);
                    }
                }
            }

            return instance;
        }

        bool IConfigStoreUpdateHandler.OnUpdate(string sectionName, string keyName)
        {
            switch (sectionName)
            {
                case SecurityConfigSectionName:
                    switch (keyName)
                    {
                        case ServerCertThumbprints:
                            this.UpdateAllowedClientCertThumbprints();
                            break;
                        case ClusterIdentities:
                            this.UpdateAllowedClientIdentities();
                            break;
                    }
                    break;

                case SecurityServerX509NamesConfigSectionName:
                    this.UpdateAllowedClientCertCommonNames();
                    break;

                default:
                    break;

                    // Note: No need to react to updates for ServerAuthX509FindValue, ServerAuthX509FindValueSecondary, ServerAuthX509FindType & ServerAuthX509FindValue
                    // As these updates recycle process.
            }

            return true;
        }

        bool IConfigStoreUpdateHandler.CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException("Invalid Operation");
        }

        internal void UpdateAllowedClientIdentities()
        {
            if (this.ServerAuthCredentialType == CredentialType.Windows)
            {
                string clusterIdentitiesString = this.configStore.ReadUnencryptedString(SecurityConfigSectionName, ClusterIdentities);
                this.AllowedClientIdentities = GetParamValueHashSet(clusterIdentitiesString);
            }
        }

        internal void UpdateAllowedClientCertThumbprints()
        {
            if (this.ServerAuthCredentialType == CredentialType.X509)
            {
                // Read Security/ServerCertThumbprints configuration to get allowed Thumbprints for server certificates.
                string serverCertThumbprints = this.configStore.ReadUnencryptedString(SecurityConfigSectionName, ServerCertThumbprints);
                this.AllowedClientCertThumbprints = GetParamValueHashSet(serverCertThumbprints, true);
            }
        }

        internal void UpdateX509AllowedConfiguration()
        {
            if (this.ServerAuthCredentialType == CredentialType.X509)
            {
                // Read ServerAuthX509FindType
                X509FindType serverAuthCertFindType;
                string serverAuthCertFindTypeName = this.configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509FindType);

                if (string.IsNullOrEmpty(serverAuthCertFindTypeName) || !Enum.TryParse<X509FindType>(serverAuthCertFindTypeName, out serverAuthCertFindType))
                {
                    EventStoreLogger.Logger.LogWarning("Unable to convert configuration value of {0}: {1} to X509FindType.", ServerAuthX509FindType, serverAuthCertFindTypeName);
                    return;
                }

                this.AuthX509FindType = serverAuthCertFindType;

                switch (serverAuthCertFindType)
                {
                    case X509FindType.FindByThumbprint:
                        this.UpdateAllowedClientCertThumbprints();
                        break;

                    case X509FindType.FindBySubjectName:
                        this.UpdateAllowedClientCertCommonNames();
                        break;
                }
            }
        }

        internal void UpdateAllowedClientCertCommonNames()
        {
            if (this.ServerAuthCredentialType == CredentialType.X509)
            {
                Dictionary<string, HashSet<string>> allowedServerCommonNames = new Dictionary<string, HashSet<string>>();
                
                // Read common name based Server certificates
                string serverAuthX509FindValue = this.configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509FindValue);
                string serverAuthX509FindValueSecondary = this.configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509FindValueSecondary);

                if (!string.IsNullOrEmpty(serverAuthX509FindValue))
                {
                    allowedServerCommonNames[serverAuthX509FindValue] = this.GetServerCommonNameCAThumbprints(serverAuthX509FindValue);
                }

                if (!string.IsNullOrEmpty(serverAuthX509FindValueSecondary))
                {
                    allowedServerCommonNames[serverAuthX509FindValueSecondary] = this.GetServerCommonNameCAThumbprints(serverAuthX509FindValueSecondary);
                }

                this.AllowedClientCertCommonNames = allowedServerCommonNames;
            }
        }

        private HashSet<string> GetServerCommonNameCAThumbprints(string serverCommonName)
        {
            string caCertThumbprints = this.configStore.ReadUnencryptedString(SecurityServerX509NamesConfigSectionName, serverCommonName);
            return GetParamValueHashSet(caCertThumbprints, true);
        }

        private void InitializeFromConfig()
        {
            switch (this.ServerAuthCredentialType)
            {
                case CredentialType.Windows:
                    this.UpdateAllowedClientIdentities();
                    break;

                case CredentialType.X509:
                    this.UpdateX509AllowedConfiguration();
                    break;
            }
        }

        private static HashSet<string> GetParamValueHashSet(string valueString, bool convertStringstoUpperCase = false)
        {
            HashSet<string> values = new HashSet<string>();

            foreach (string value in valueString.Split(ConfigParamValueSeparator, StringSplitOptions.RemoveEmptyEntries))
            {
                values.Add(convertStringstoUpperCase ? value.ToUpper() : value);
            }

            return values;
        }
    }
}