// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System.Fabric.Common;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal class EncryptionCertConfigHandler : IConfigStoreUpdateHandler
    {
        private const string TraceType = "EncryptionCertConfigHandler";
        
        private const string SecretEncryptionCertStoreNameManifestKey = "SecretEncryptionCertX509StoreName";
        private const string SecretEncryptionCertThumbprintManifestKey = "SecretEncryptionCertThumbprint";

        private const string SecretEncryptionCertX509StoreNameDefault = "My";

        private readonly Action<string, string, CancellationToken> updateCertCallback;
        private readonly StatefulService statefulService;
        private readonly NativeConfigStore nativeConfigStore;

        private readonly CancellationToken runAsyncCancellationToken;

        private static string EncryptionCertStoreName;
        private static string EncryprtionCertThumprint;
        private static readonly object Lock;

        static EncryptionCertConfigHandler()
        {
            Lock = new object();
        }

        public EncryptionCertConfigHandler(Action<string, string, CancellationToken> encryptionCertChangeCallback,
            StatefulService statefulService, CancellationToken runAsyncCancellationToken)
        {
            this.updateCertCallback = encryptionCertChangeCallback;
            this.runAsyncCancellationToken = runAsyncCancellationToken;
            this.statefulService = statefulService;
            this.nativeConfigStore = NativeConfigStore.FabricGetConfigStore();
        }

        public async Task InitializeAsync()
        {
            // Read the state and update the config in the store and trigger update callback if needed
            
            // Check if the secret certificate has been updated or not
            // Get the current certificate from store
            string thumbprintFromManifest, x509StoreNameFromManifest;
            var updateSecrets = false;
            var configStore = await ConfigStore.CreateOrGetConfigStore(this.statefulService);
            using (var transaction = this.statefulService.StateManager.CreateTransaction())
            {
                var val = await configStore.GetValueAsync(Common.Constants.SecretUpdateInProgress, transaction);
                // Check explicitly if the configured secret in manifest and the secret stored in ConfigStore are same or not
                var thumbprintFromStore = await configStore.GetValueAsync(Common.Constants.EncryptionCertThumbprintKey, transaction);

                // Initialize and fetch the thumbprint configured in manifest
                this.GetConfigFromNativeConfigStore(out thumbprintFromManifest, out x509StoreNameFromManifest);
                
                if (String.IsNullOrEmpty(val))
                {
                    if (String.IsNullOrEmpty(thumbprintFromManifest))
                    {
                        thumbprintFromManifest = null;
                    }

                    if (String.IsNullOrEmpty(thumbprintFromStore))
                    {
                        thumbprintFromStore = null;
                    }

                    // Thumbprint for encryption cert has changed, let's update secrets
                    if (!String.Equals(thumbprintFromManifest, thumbprintFromStore, StringComparison.InvariantCultureIgnoreCase))
                    {
                        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "The existing and new thumprints dont match, existing {0}, new {1}, triggering secrets udpate", thumbprintFromStore, thumbprintFromManifest);
                        updateSecrets = true;
                    }
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Detected update in progress flag, triggering secrets update");
                    // There is an update in progress, let's update secrets
                    updateSecrets = true;
                }

                if (updateSecrets)
                {
                    // Set secret update in progress flag by setting some random guid value.
                    await configStore.UpdateValueAsync(Common.Constants.SecretUpdateInProgress, Guid.NewGuid().ToString(), transaction);
                }

                await transaction.CommitAsync();
            }

            lock (Lock)
            {
                EncryprtionCertThumprint = thumbprintFromManifest;
                EncryptionCertStoreName = x509StoreNameFromManifest;
            }

            if (updateSecrets)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Invoking update cert callback");

                // Trigger the update secrets callback
                this.updateCertCallback(thumbprintFromManifest, x509StoreNameFromManifest, this.runAsyncCancellationToken);
            }
        }

        bool IConfigStoreUpdateHandler.CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        bool IConfigStoreUpdateHandler.OnUpdate(string sectionName, string keyName)
        {
            if (sectionName != BackupRestoreContants.BrsConfigSectionName) return true;
            if (keyName == BackupRestoreServiceConfig.SecretEncryptionCertStoreNameKey ||
                     keyName == BackupRestoreServiceConfig.SecretEncryptionCertThumbprintKey)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Encryption cert configuration has changed");

                try
                {
                    this.InitializeAsync().Wait();
                }
                catch (Exception ex)
                {
                    BackupRestoreTrace.TraceSource.WriteExceptionAsError(TraceType, ex, "InitializeAsync failed with error");
                    return false;
                }
                
            }

            return true;
        }

        private void GetConfigFromNativeConfigStore(out string encryptionCertThumbprint, out string encryptionCertStoreName)
        {
            var certStoreNameString = this.nativeConfigStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, SecretEncryptionCertStoreNameManifestKey);
            encryptionCertStoreName = String.IsNullOrEmpty(certStoreNameString) ? SecretEncryptionCertX509StoreNameDefault : certStoreNameString;

            encryptionCertThumbprint = this.nativeConfigStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, SecretEncryptionCertThumbprintManifestKey);
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Encryption cert thumprint {0}, store name {1}", encryptionCertThumbprint, encryptionCertStoreName);
        }

        internal static void GetEncryptionCertDetails(out string certThumbprint, out string certStoreName)
        {
            lock (Lock)
            {
                certThumbprint = EncryprtionCertThumprint;
                certStoreName = EncryptionCertStoreName;
            }
        }
    }
}