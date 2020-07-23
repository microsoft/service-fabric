// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.ComponentModel;
using System.Fabric.BackupRestore.BackupRestoreEndPoints;
using System.Fabric.Common;
using System.Fabric.Common.Tracing;
using System.Fabric.Description;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;

namespace System.Fabric.BackupRestore.Setup
{
    public class Program
    {
        private const string TraceType = "FabricBRSSetup";

        private const string IsScaleMinKey = "IsScaleMin";

        // we should only load the DLLs that we want to load dynamically and not anything else
        private static readonly string[] KnownDlls =
        {
            "System.Fabric.BackupRestore",
        };

        static Program()
        {
            AppDomain.CurrentDomain.AssemblyResolve += LoadFromFabricCodePath;
        }

        static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                var assemblyToLoad = new AssemblyName(args.Name);

                if (KnownDlls.Contains(assemblyToLoad.Name))
                {
                    var folderPath = FabricEnvironment.GetCodePath();
                    var assemblyPath = Path.Combine(folderPath, assemblyToLoad.Name + ".dll");

                    if (File.Exists(assemblyPath))
                    {
                        return Assembly.LoadFrom(assemblyPath);
                    }
                }
            }
            catch (Exception)
            {
                // Supress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }

        public static int Main(string[] args)
        {
            FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.BackupRestoreService);

            int status = 0;

            var codePackageActivationContext = FabricRuntime.GetActivationContext();
            var configStore = NativeConfigStore.FabricGetConfigStore();
            var serverAuthCredentialTypeName = configStore.ReadUnencryptedString(BackupRestoreContants.SecurityConfigSectionName, BackupRestoreContants.ServerAuthCredentialTypeName);
            var serverAuthCredentialType = CredentialType.None;
            EndpointResourceDescription endpoint = null;

            try
            {
                endpoint = codePackageActivationContext.GetEndpoint(BackupRestoreContants.RestEndPointName);
            }
            catch (FabricElementNotFoundException)
            {
                traceSource.WriteWarning(TraceType, "Endpoint not found for EndpointName{0}", BackupRestoreContants.RestEndPointName);
                throw;
            }

            if (!string.IsNullOrEmpty(serverAuthCredentialTypeName) && !Enum.TryParse(serverAuthCredentialTypeName, out serverAuthCredentialType))
            {
                traceSource.WriteWarning(TraceType, "Unable to convert configuration value serverAuthCredentialTypeName {0} for {1} type.", serverAuthCredentialTypeName, serverAuthCredentialType);
            }

            if (serverAuthCredentialType == CredentialType.X509 || serverAuthCredentialType == CredentialType.Claims)
            {
                // Get server auth details

                var aclServerAuthX509StoreName = configStore.ReadUnencryptedString(BackupRestoreContants.FabricNodeConfigSectionName, BackupRestoreContants.ServerAuthX509StoreName);
                var aclCertValueFindTypeName = configStore.ReadUnencryptedString(BackupRestoreContants.FabricNodeConfigSectionName, BackupRestoreContants.ServerAuthX509FindType);
                var aclCertValue = configStore.ReadUnencryptedString(BackupRestoreContants.FabricNodeConfigSectionName, BackupRestoreContants.ServerAuthX509FindValue);
                var aclCertValueSecondary = configStore.ReadUnencryptedString(BackupRestoreContants.FabricNodeConfigSectionName, BackupRestoreContants.ServerAuthX509FindValueSecondary);

                var storeName = StoreName.My;
                var x509FindType = X509FindType.FindByThumbprint;
                X509Certificate2 certificate = null;

                if (!string.IsNullOrEmpty(aclServerAuthX509StoreName) && !Enum.TryParse(aclServerAuthX509StoreName, out storeName))
                {
                    traceSource.WriteWarning(TraceType, "Unable to Convert configuration aclServerAuthX509StoreName value {0} for {1} type.", aclServerAuthX509StoreName, storeName.GetType().ToString());
                    throw new InvalidEnumArgumentException(string.Format("Unable to Convert configuration aclCertValueFindTypeName value {0} for {1} type.", aclServerAuthX509StoreName, x509FindType.GetType()));
                }

                if (!string.IsNullOrEmpty(aclCertValueFindTypeName) && !Enum.TryParse(aclCertValueFindTypeName, out x509FindType))
                {
                    traceSource.WriteWarning(TraceType, "Unable to Convert configuration aclCertValueFindTypeName value {0} for {1} type.", aclServerAuthX509StoreName, x509FindType.GetType().ToString());
                    throw new InvalidEnumArgumentException(string.Format("Unable to Convert configuration aclCertValueFindTypeName value {0} for {1} type.", aclServerAuthX509StoreName, x509FindType.GetType()));
                }

                if (!string.IsNullOrWhiteSpace(aclCertValue))
                {
                    // Get certificate.
                    var store = new X509Store(storeName, StoreLocation.LocalMachine);

                    try
                    {
                        store.Open(OpenFlags.ReadOnly);

                        traceSource.WriteInfo(
                            TraceType,
                            "Finding applicable certificate for Acling. StoreName: {0}, StoreLocation: LocalMachine, X509FindType: {1}, X509FindValue: {2}.",
                            storeName,
                            x509FindType,
                            aclCertValue);

                        var certCollections = store.Certificates.Find(x509FindType, aclCertValue, false /*load self-signed cert*/);
                        if (certCollections.Count > 0)
                        {
                            certificate = certCollections[0];
                        }
                        else if (!string.IsNullOrWhiteSpace(aclCertValueSecondary))
                        {
                            traceSource.WriteInfo(
                                TraceType,
                                "Finding applicable certificate for Acling using Secondary cert config. StoreName: {0}, StoreLocation: LocalMachine, X509FindType: {1}, X509FindValue: {2}.",
                                storeName,
                                x509FindType,
                                aclCertValueSecondary);

                            var certCollectionsSecondary = store.Certificates.Find(x509FindType, aclCertValueSecondary, false /*load self-signed cert*/);
                            if (certCollectionsSecondary.Count > 0)
                            {
                                certificate = certCollectionsSecondary[0];
                            }
                        }
                        else
                        {
                            traceSource.WriteWarning(TraceType, "No matching certificate found. Thumbprint value: {0}, StoreName: {1}, StoreLocation: {2}", aclCertValue, aclServerAuthX509StoreName, StoreLocation.LocalMachine);
                        }
                    }
                    finally
                    {
                        store.Close();
                    }
                }
                else
                {
                    traceSource.WriteWarning(TraceType, "Invalid configuration for certificate. Thumbprint value: {0}, StoreName: {1}, StoreLocation: {2}", aclCertValue, aclServerAuthX509StoreName, StoreLocation.LocalMachine);
                }

                if (certificate != null)
                {
                    PortAclUtility.BindCertificate(endpoint.IpAddressOrFqdn, endpoint.Port, certificate.GetCertHash());
                }
            }

            // Do URL ACLing
            CodePackage codePackage = codePackageActivationContext.GetCodePackageObject("Code");
            string daclString = "D:(A;;GX;;;NS)";
            
            try
            {
                var runAsAccountName = configStore.ReadUnencryptedString(BackupRestoreContants.FabricNodeRunAsSectionName, BackupRestoreContants.RunAsAccountNameConfig);
                if (!string.IsNullOrEmpty(runAsAccountName))
                {
                    traceSource.WriteInfo(TraceType, "runAsAccountName for ACLing: {0} for CredentialType Windows", runAsAccountName);
                    daclString = GetAllowDaclFromUserName(runAsAccountName);
                }
            }
            catch (IdentityNotMappedException ex)
            {
                traceSource.WriteWarning(TraceType, "Failed to resolve NTAccount: {0}, Exception: {1}", codePackage.EntryPointRunAsPolicy.UserName, ex);
                throw;
            }
            catch (SystemException ex)
            {
                traceSource.WriteWarning(TraceType, "Failed to resolve NTAccount: {0}, Exception: {1}", codePackage.EntryPointRunAsPolicy.UserName, ex);
                throw;
            }

            var fabricBrsSecuritySetting = SecuritySetting.GetClusterSecurityDetails();
            var listeningAddress = String.Format(CultureInfo.InvariantCulture, "{0}://+:{1}/", fabricBrsSecuritySetting.EndpointProtocol, endpoint.Port);
            traceSource.WriteInfo(TraceType, "ListeningAddress: {0} ,DaclString: {1} for ACLing", listeningAddress, daclString);
            PortAclUtility.ReserveUrl(listeningAddress, daclString);

            return status;
        }

        private static string GetAllowDacl(SecurityIdentifier sid)
        {
            return $"D:(A;;GX;;;{sid.Value})";
        }

        private static string GetAllowDaclFromUserName(string userName)
        {
            NTAccount entryPointRunAsUser = new NTAccount(userName);
            SecurityIdentifier entryPointRunAsUserSid = (SecurityIdentifier)entryPointRunAsUser.Translate(typeof(SecurityIdentifier));
            return GetAllowDacl(entryPointRunAsUserSid);
        }
    }
}