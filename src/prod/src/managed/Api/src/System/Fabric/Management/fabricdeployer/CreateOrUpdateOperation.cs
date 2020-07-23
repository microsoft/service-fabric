// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Common;
    using System.Linq;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using Collections.Generic;
    using System.Security.AccessControl;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;

    internal class CreateorUpdateOperation : UpdateNodeStateOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            var isRunningAsAdmin = AccountHelper.IsAdminUser();

            if (infrastructure == null)
            {
                DeployerTrace.WriteError("Cannot continue creating or updating deployment without infrastructure information");
                throw new InvalidDeploymentException(StringResources.Error_FabricDeployer_InvalidInfrastructure_Formatted);
            }

            if (parameters.Operation == DeploymentOperations.Update)
            {
                if (ShouldRemoveCurrentNode(parameters.NodeName, clusterManifest))
                {
#if !DotNetCoreClrLinux
                    DeployerTrace.WriteError("Current node is not present in the manifest list for Windows Server deployments. Changing DeploymentOperation to RemoveNodeConfig");
                    throw new ChangeDeploymentOperationToRemoveException("Modify Operation to RemoveNodeConfig");
#else
                    DeployerTrace.WriteInfo("CoreClrLinux: RemoveNodeConfigOperation is not enabled on Linux.");
                    return;
#endif
                }
            }

#if !DotNetCoreClrLinux
            AclClusterLevelCerts(clusterManifest);
#endif // !DotNetCoreClrLinux

#if !DotNetCoreClrIOT
#if DotNetCoreClrLinux
            BootstrapContainerSetup();
#endif // DotNetCoreClrLinux

            // Creates or deletes supported networks based on configuration.
            ManageNetworks(parameters, clusterManifest, infrastructure);
#endif // !DotNetCoreClrIOT

#if !DotNetCoreClrIOT && DotNetCoreClrLinux
            // On Linux, FabricDeployer configure operation is not called.
            // Adding the registry entries in create operatio.
            // Do not update the entries in operations other than Create/Update
            if (parameters.Operation == DeploymentOperations.Create || parameters.Operation == DeploymentOperations.Update)
            {
                Utility.SetFabricRegistrySettings(parameters);
            }
#endif

            base.OnExecuteOperation(parameters, clusterManifest, infrastructure);

            if (fabricValidator.IsTVSEnabled)
            {
#if !DotNetCoreClr // Disable compiling on windows. Need to correct when porting FabricDeployer.
                if (!TVSSetup.SetupInfrastructureForClaimsAuth(isRunningAsAdmin))
                {
                    DeployerTrace.WriteError("Enabling WIF failed when creating or updating deployment with claims authentication.");
                    throw new InvalidDeploymentException(StringResources.Error_FabricDeployer_TVSSetupFailed_Formatted);
                }
#else
                DeployerTrace.WriteInfo("CoreClr: Token validation service is not enabled on Linux.");
#endif // !DotNetCoreClrLinux
            }
        }

#region Bootstrap Container Setup for Linux
#if DotNetCoreClrLinux && !DotNetCoreClrIOT
        /// <summary>
        /// Makes sure appropriate container packages are installed and configured correctly on the machine.
        /// </summary>
        private void BootstrapContainerSetup()
        {
            ProcessResult result;
            string sfPackageInstalledCmd = string.Format(Constants.DpkgPackageCleanlyInstalledCommandFormat, Constants.PackageName_ServiceFabric);
            result = Utility.RunLinuxBashCommand(sfPackageInstalledCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);

            if (result.ExitCode != 0)
            {
                DeployerTrace.WriteWarning(string.Format("{0} is either not installed or the package is not in a clean state.", Constants.PackageName_ServiceFabric));
                
                Utility.RunAptLockLinuxBashCommand(Constants.AptPackageResolveCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
                Utility.RunAptLockLinuxBashCommand(Constants.DpkgConfigureCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);

                result = Utility.RunLinuxBashCommand(sfPackageInstalledCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(30), true);
                if (result.ExitCode != 0)
                {
                    throw new InvalidDeploymentException(string.Format("{0} package must be installed and not in error state to proceed", Constants.PackageName_ServiceFabric));
                }
            }

            string sfpackageRequiresDockerCmd =
                string.Format(Constants.AptPackageDepencenciesContainsFormat, Constants.PackageName_ServiceFabric, Constants.PackageName_DockerCE);
            var typicalExitCodes = new int[] { 0, 1 };
            result = Utility.RunLinuxBashCommand(sfpackageRequiresDockerCmd, Environment.CurrentDirectory, TimeSpan.FromSeconds(60), typicalExitCodes, 10, 5, true); // Usually takes 1-2 seconds

            if (!Utility.IsExitCodeExpected(result, new int[] { 0, 1 }))
            {
                throw new InvalidDeploymentException(string.Format("{0} depends check for {1} failed", Constants.PackageName_ServiceFabric, Constants.PackageName_DockerCE));
            }

            if (result.ExitCode == 0)
            {
                DeployerTrace.WriteInfo(string.Format("{0} depends on {1}. Expecting this is already installed and configured; exiting.", Constants.PackageName_ServiceFabric, Constants.PackageName_DockerCE));
                return;
            }

            // Remove docker-ce & containerd.io
            Utility.PurgeAptPackageIfPresent(Constants.PackageName_DockerCE);
            Utility.PurgeAptPackageIfPresent(Constants.PackageName_ContainerdIO);

            result = Utility.RunAptLockLinuxBashCommand(Constants.AptAutoremoveCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            if (result.ExitCode != 0)
            {
                DeployerTrace.WriteWarning(string.Format("Command '{0}' hit error {1}. Troubleshooting...", Constants.AptAutoremoveCommand, result.ExitCode));
                Utility.RunAptLockLinuxBashCommand(Constants.DpkgConfigureCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
                Utility.RunAptLockLinuxBashCommand(Constants.AptAutoremoveCommand, Environment.CurrentDirectory, TimeSpan.FromMinutes(15), true);
            }

            // Set up Moby
            /* We will be tracking moby package auto/manual state if we are responsible for installing it.
             * If and when SF runtime is purged from the environment the moby-* packages will be marked in
             * the apt db as auto to be autoremoved in the next cleanup cycle.
             * 
             * Cases:
             *  sf installed one package - track
             *  sf installed both packages - track
             *  sf installed no packages - no tracking
             *  
             * - If user had manually installed a moby package, this will be manual so is safe against autoremove.
             *      In such case we will not be tracking this and as such will not be marking moby packages as
             *      auto when servicefabric is purged.
             * - If a moby package was brought in as a dependency from another package, it is auto and is safe
             *      against autoremove until the parent package is removed. If this happens, the next run of
             *      FabricDeployer will fix this by reinstalling the appropriate package and begin tracking.
             */

            bool mobyEngineInstalledInThisRun = false, mobyCliInstalledInThisRun = false;
            Utility.ForceAptPackageInstall(Constants.PackageName_MobyEngine, ref mobyEngineInstalledInThisRun);
            Utility.ForceAptPackageInstall(Constants.PackageName_MobyCLI, ref mobyCliInstalledInThisRun);
            if (mobyEngineInstalledInThisRun || mobyCliInstalledInThisRun)
            {
                FabricEnvironment.SetSfInstalledMoby("1");
            }
        }
#endif // DotNetCoreClrLinux && !DotNetCoreClrIOT
#endregion

        /// <summary>
        /// Sets up or cleans up two types of networks -
        /// 1) Open Network
        /// 2) Isolated Network
        /// </summary>
        private void ManageNetworks(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
#region Container Network Setup

            if (parameters.ContainerNetworkSetup)
            {
                var containerNetworkSetupOperation = new ContainerNetworkSetupOperation();
                containerNetworkSetupOperation.ExecuteOperation(parameters, clusterManifest, infrastructure);
            }
            else
            {
                // Clean up docker network set up. This is needed for the SFRP scenario (there is no explicit uninstall)
                // when customers want to clean up container network set up through config upgrade.

                var containerServiceArguments = (parameters.UseContainerServiceArguments) ? (parameters.ContainerServiceArguments) : (FlatNetworkConstants.ContainerServiceArguments);

#if !DotNetCoreClrLinux
                containerServiceArguments = (parameters.EnableContainerServiceDebugMode) 
                    ? (string.Format("{0} {1}", containerServiceArguments, FlatNetworkConstants.ContainerProviderServiceDebugModeArg)) : containerServiceArguments;

                // This check is needed to allow clean up on azure. This is symmetrical to the set up condition.
                if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure ||
                    clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
                {
                    var containerNetworkCleanupOperation = new ContainerNetworkCleanupOperation();
                    containerNetworkCleanupOperation.ExecuteOperation(parameters.ContainerNetworkName, containerServiceArguments, parameters.FabricDataRoot, parameters.Operation);
                }
#else
                // This check is needed to disallow one box clean up on linux. This is symmetrical to the set up condition.
                // This was also needed because one box clean up resulted in an exception in the GetNetwork api. 
                // Exception => System.Threading.Tasks.TaskCanceledException: A task was canceled
                if (clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS)
                {
                    var containerNetworkCleanupOperation = new ContainerNetworkCleanupOperation();
                    containerNetworkCleanupOperation.ExecuteOperation(parameters.ContainerNetworkName, containerServiceArguments, parameters.FabricDataRoot, parameters.Operation);
                }
#endif // !DotNetCoreClrLinux
            }

#endregion

#region Isolated Network Setup

            if (parameters.EnableUnsupportedPreviewFeatures)
            {
                if (parameters.IsolatedNetworkSetup)
                {
                    var isolatedNetworkSetupOperation = new IsolatedNetworkSetupOperation();
                    isolatedNetworkSetupOperation.ExecuteOperation(parameters, clusterManifest, infrastructure);
                }
                else
                {
                    var isolatedNetworkCleanupOperation = new IsolatedNetworkCleanupOperation();
                    isolatedNetworkCleanupOperation.ExecuteOperation(parameters.IsolatedNetworkName, parameters.FabricBinRoot, parameters.Operation);
                }
            }
            else
            {
                DeployerTrace.WriteInfo("Isolated Network preview feature disabled for the cluster.");
            }   
#endregion
        }

#if !DotNetCoreClrLinux
        internal static void AclClusterLevelCerts(ClusterManifestType clusterManifest)
        {
            DeployerTrace.WriteInfo("AclClusterLevelCerts: Enter");

            ClusterManifestTypeNodeType[] nodeTypes = clusterManifest.NodeTypes;
            if (nodeTypes == null)
            {
                DeployerTrace.WriteWarning("AclClusterLevelCerts: No node type is defined on the cluster manifest.");
                return;
            }

            IEnumerable<FabricCertificateType> certs = nodeTypes.Where(nodeType => nodeType.Certificates != null).Select(nodeType => nodeType.Certificates)
                .SelectMany(certificates => new FabricCertificateType[]
                {
                    certificates.ClientCertificate,
                    certificates.ClusterCertificate,
                    certificates.ServerCertificate
                }).Where(cert => cert != null);
            if (!certs.Any())
            {
                DeployerTrace.WriteWarning("AclClusterLevelCerts: No load cert is defined.");
                return;
            }

            List<CertId> certIds = new List<CertId>();
            foreach (FabricCertificateType cert in certs)
            {
                certIds.Add(new CertId(cert.X509FindType, cert.X509FindValue, cert.X509StoreName));
                if (!string.IsNullOrWhiteSpace(cert.X509FindValueSecondary)
                    && cert.X509FindValueSecondary != cert.X509FindValue)
                {
                    certIds.Add(new CertId(cert.X509FindType, cert.X509FindValueSecondary, cert.X509StoreName));
                }
            }

            string accountName = "NT AUTHORITY\\NETWORK SERVICE";
            string runasAccount = TryGetRunAsAccountName(clusterManifest);
            if (runasAccount != null)
            {
                accountName = runasAccount;
            }

            List<CertId> acledCertIds = new List<CertId>();
            foreach (CertId certId in certIds)
            {
                if (!acledCertIds.Any(p => p.Equals(certId)))
                {
                    DeployerTrace.WriteInfo("AclClusterLevelCerts: processing {0}", certId);
                    AclCert(certId, accountName);
                    acledCertIds.Add(certId);
                    DeployerTrace.WriteInfo("AclClusterLevelCerts: {0} processed", certId);
                }
            }
        }

        private static void AclCert(CertId certId, string accountName)
        {
            X509Store store = null;

            try
            {
                store = new X509Store(certId.StoreName, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadWrite | OpenFlags.OpenExistingOnly);

                X509Certificate2Collection certs = store.Certificates.Find(ConvertToX509FindType(certId.FindType), certId.FindValue, validOnly: false);
                if (certs == null || certs.Count == 0)
                {
                    DeployerTrace.WriteInfo("AclCert is skipped for {0} because it's not installed", certId);
                    return;
                }

                foreach (X509Certificate2 cert in certs)
                {
                    if (cert.PrivateKey == null)
                    {
                        DeployerTrace.WriteWarning("AclCert is skipped for {0} because its private key is null", certId);
                        continue;
                    }

                    RSACryptoServiceProvider privateKey = cert.PrivateKey as RSACryptoServiceProvider;
                    if (privateKey == null)
                    {
                        DeployerTrace.WriteWarning("AclCert is skipped for {0} because its private key type {1} is not supported ", certId, cert.PrivateKey.GetType());
                        continue;
                    }

                    CspParameters csp = new CspParameters(privateKey.CspKeyContainerInfo.ProviderType, privateKey.CspKeyContainerInfo.ProviderName, privateKey.CspKeyContainerInfo.KeyContainerName);
                    csp.Flags = CspProviderFlags.UseExistingKey | CspProviderFlags.UseMachineKeyStore;
                    csp.KeyNumber = (int)privateKey.CspKeyContainerInfo.KeyNumber;
#if !DotNetCoreClr
                    csp.CryptoKeySecurity = privateKey.CspKeyContainerInfo.CryptoKeySecurity;

                    CryptoKeyAccessRule accessRule = new CryptoKeyAccessRule(accountName, CryptoKeyRights.FullControl, AccessControlType.Allow);
                    csp.CryptoKeySecurity.AddAccessRule(accessRule);
#endif

                    using (RSACryptoServiceProvider newCsp = new RSACryptoServiceProvider(csp))
                    {
                        DeployerTrace.WriteInfo("AclCert success: {0}", certId);
                    }
                }

            }
            catch (Exception ex)
            {
                DeployerTrace.WriteError("AclCert error with {0}: {1}", certId, ex);
            }
            finally
            {
                if (store != null)
                {
                    store.Close();
                }
            }
        }
#endif

        private static string TryGetRunAsAccountName(ClusterManifestType clusterManifest)
        {
            SettingsOverridesTypeSection runAsSection = clusterManifest.FabricSettings.FirstOrDefault(p => p.Name == Constants.SectionNames.RunAs);
            if (runAsSection != null && runAsSection.Parameter != null)
            {
                SettingsOverridesTypeSectionParameter parameter = runAsSection.Parameter.FirstOrDefault(p => p.Name == Constants.ParameterNames.RunAsAccountType);
                if (parameter != null && parameter.Value == Constants.ManagedServiceAccount)
                {
                    parameter = runAsSection.Parameter.FirstOrDefault(p => p.Name == Constants.ParameterNames.RunAsAccountName);
                    if (parameter != null)
                    {
                        return parameter.Value;
                    }
                }
            }

            return null;
        }

        // Checks if current node should be removed for Windows Server deployments only.
        private bool ShouldRemoveCurrentNode(string nodeName, ClusterManifestType clusterManifest)
        {
            var infrastructureType = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;

            // For deployments that are not standalone, this section will not be present. Continue update operation for this node as is.
            if (infrastructureType == null)
            {
                DeployerTrace.WriteInfo("Node section not found in cluster manifest infrastructure section.");
                return false;
            }

            if (nodeName == null)
            {
                return false;
            }
            else
            {
                var nodesToBeRemovedSection = clusterManifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Setup, StringComparison.OrdinalIgnoreCase)
                                                                      && section.Parameter != null
                                                                      && section.Parameter.Any(parameter => parameter.Name.Equals(Constants.ParameterNames.NodesToBeRemoved)));
                List<string> nodesToRemove = new List<string>();
                if (nodesToBeRemovedSection != null)
                {
                    nodesToRemove = nodesToBeRemovedSection.Parameter.First(parameter => parameter.Name.Equals(Constants.ParameterNames.NodesToBeRemoved, StringComparison.OrdinalIgnoreCase)).Value.Split(',').Select(p => p.Trim()).ToList();
                }

                bool isNodeToBeRemoved = !infrastructureType.NodeList.Any(n => n.NodeName == nodeName) && nodesToRemove.Contains(nodeName);
                DeployerTrace.WriteInfo("Checking if current node needs to be removed : {0}", isNodeToBeRemoved);
                return isNodeToBeRemoved;
            }
        }

        private class CertId
        {
            public CertId(FabricCertificateTypeX509FindType findType, string findValue, string storeName)
            {
                this.FindType = findType;
                this.FindValue = findValue;
                this.StoreName = storeName;
            }

            public override int GetHashCode()
            {
                return base.GetHashCode();
            }

            public override bool Equals(object obj)
            {
                if (obj == null || !(obj is CertId))
                {
                    return false;
                }

                CertId theOther = (CertId)obj;
                return this.FindType == theOther.FindType
                    && this.FindValue == theOther.FindValue
                    && this.StoreName == theOther.StoreName;
            }

            public override string ToString()
            {
                return string.Format("{0},{1},{2}", this.FindType, this.FindValue, this.StoreName);
            }

            public FabricCertificateTypeX509FindType FindType { get; private set; }

            public string FindValue { get; private set; }

            public string StoreName { get; private set; }
        }

        private static X509FindType ConvertToX509FindType(FabricCertificateTypeX509FindType findType)
        {
            switch (findType)
            {
                case FabricCertificateTypeX509FindType.FindByThumbprint:
                    return X509FindType.FindByThumbprint;

                case FabricCertificateTypeX509FindType.FindBySubjectName:
                    return X509FindType.FindBySubjectName;

                case FabricCertificateTypeX509FindType.FindByExtension:
                    return X509FindType.FindByExtension;

                default:
                    throw new NotSupportedException(findType + " is not supported");
            }
        }
    }
}
