// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Security.Cryptography.X509Certificates;

    internal sealed class SecuritySetting : IConfigStoreUpdateHandler
    {
        private const string TraceType = "SecuritySetting";

        private static readonly char[] ConfigParamValueSeparator = new char[] { ',' };

        internal CredentialType ServerAuthCredentialType { get; }

        internal X509FindType ServerAuthX509FindType { get; private set; }

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
            string serverAuthCredentialTypeName = configStore.ReadUnencryptedString(BackupRestoreContants.SecurityConfigSectionName, BackupRestoreContants.ServerAuthCredentialTypeName);
            CredentialType serverAuthCredentialType = CredentialType.None;

            if ((!string.IsNullOrEmpty(serverAuthCredentialTypeName)) &&
                (!Enum.TryParse<CredentialType>(serverAuthCredentialTypeName, out serverAuthCredentialType)))
            {
                BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Unable to convert configuration value serverAuthCredentialTypeName {0} for {1} type.", serverAuthCredentialTypeName, serverAuthCredentialType);
            }

            this.ServerAuthCredentialType = serverAuthCredentialType;
            if (ServerAuthCredentialType == CredentialType.X509)
            {
                this.EndpointProtocol = Description.EndpointProtocol.Https;
            }
            else
            {
                this.EndpointProtocol = Description.EndpointProtocol.Http;
            }

            this.AllowedClientCertThumbprints = null;
            this.AllowedClientIdentities = null;
            this.AllowedClientCertCommonNames = null;

            this.InitializeFromConfig();
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

        bool IConfigStoreUpdateHandler.OnUpdate(string sectionName, string keyName)
        {
            switch(sectionName)
            {
                case BackupRestoreContants.SecurityConfigSectionName:
                    switch(keyName)
                    {
                        case BackupRestoreContants.ServerCertThumbprints:
                            this.UpdateAllowedClientCertThumbprints();
                            break;
                        case BackupRestoreContants.ClusterIdentities:
                            this.UpdateAllowedClientIdentities();
                            break;
                    }
                    break;

                case BackupRestoreContants.SecurityServerX509NamesConfigSectionName:
                    this.UpdateAllowedClientCertCommonNames();
                    break;

                    // Note: No need to react to updates for ServerAuthX509FindValue, ServerAuthX509FindValueSecondary, ServerAuthX509FindType & ServerAuthX509FindValue
                    // As these updates recycle process.
            }

            return true;
        }

        bool IConfigStoreUpdateHandler.CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        internal void UpdateAllowedClientIdentities()
        {
            if (this.ServerAuthCredentialType == CredentialType.Windows)
            {
                string clusterIdentitiesString = this.configStore.ReadUnencryptedString(
                    BackupRestoreContants.SecurityConfigSectionName,
                    BackupRestoreContants.ClusterIdentities);

                this.AllowedClientIdentities = this.GetParamValueHashSet(clusterIdentitiesString);
            }
        }

        internal void UpdateAllowedClientCertThumbprints()
        {
            if (this.ServerAuthCredentialType == CredentialType.X509)
            {
                // Read Security/ServerCertThumbprints configuration to get allowed Thumbprints for server certificates.
                string serverCertThumbprints = this.configStore.ReadUnencryptedString(
                    BackupRestoreContants.SecurityConfigSectionName,
                    BackupRestoreContants.ServerCertThumbprints);

                this.AllowedClientCertThumbprints = this.GetParamValueHashSet(serverCertThumbprints, true);
            }
        }

        internal void UpdateX509AllowedConfiguration()
        {
            if (this.ServerAuthCredentialType == CredentialType.X509)
            {
                // Read ServerAuthX509FindType
                X509FindType serverAuthCertFindType;
                string serverAuthCertFindTypeName = this.configStore.ReadUnencryptedString(BackupRestoreContants.FabricNodeConfigSectionName, BackupRestoreContants.ServerAuthX509FindType);

                if (string.IsNullOrEmpty(serverAuthCertFindTypeName) || !Enum.TryParse<X509FindType>(serverAuthCertFindTypeName, out serverAuthCertFindType))
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Unable to convert configuration value of {0}: {1} to X509FindType.", BackupRestoreContants.ServerAuthX509FindType, serverAuthCertFindTypeName);
                    return;
                }

                this.ServerAuthX509FindType = serverAuthCertFindType;

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
                string serverAuthX509FindValue = this.configStore.ReadUnencryptedString(
                    BackupRestoreContants.FabricNodeConfigSectionName,
                    BackupRestoreContants.ServerAuthX509FindValue);

                string serverAuthX509FindValueSecondary = this.configStore.ReadUnencryptedString(
                    BackupRestoreContants.FabricNodeConfigSectionName,
                    BackupRestoreContants.ServerAuthX509FindValueSecondary);


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
            string caCertThumbprints = this.configStore.ReadUnencryptedString(
                    BackupRestoreContants.SecurityServerX509NamesConfigSectionName,
                    serverCommonName);

            return this.GetParamValueHashSet(caCertThumbprints, true);
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

        private HashSet<string> GetParamValueHashSet(string valueString, bool convertStringstoUpperCase = false)
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