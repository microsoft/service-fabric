// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.ServiceProcess;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;

    internal class InfrastructureWrapper
    {
        private const string LogTag = "InfrastructureWrapper";
        private const string FabricHostServiceName = "FabricHostSvc";
        private const string ScExe = "sc.exe";
        private const string FabricInstallerServiceName = "FabricInstallerSvc";
        private const string FabricInstallerServiceDisplayName = "Service Fabric Installer Service";
        private const string FabricInstallerServiceDescription = "Performs reliable code upgrades for Service Fabric.";

        private const string FabricInstallerServiceExe = "FabricInstallerService.exe";
        private const string FabricInstallerServiceExeDirectory = "FabricInstallerService.Code";
        private const string FabricInstallerServiceExeRelativePathToPlugin = @"WindowsFabricPackage\FabricInstallerService.Code";

        private const string InstallServiceArguments = "create {0} binPath=\"\\\"{1}\\\"\" DisplayName=\"{2}\"";
        private const string ConfigureServiceArguments = "config {0} binPath=\"\\\"{1}\\\"\"";
        private const string SetServiceStartToManualArguments = "config {0} start=demand";
        private const string SetServiceDescriptionArguments = "description {0} \"{1}\"";
        private const string SetServiceFailureActionArguments = "failure {0} reset=86400 actions=restart/5000/restart/5000/restart/5000";

        private string fabricRoot = null;

        private static readonly Process FabricHost = new Process();

        protected enum DeploymentType
        {
            Create,
            Update,
            Configure,
            None
        }

        protected bool FabricInstall(string parameters)
        {
            if (Utilities.IsWindowsFabricInstalled() ||
                Utilities.MSIExec(parameters) == 0)
            {
                return true;
            }
            else
            {
                Logger.LogError(LogTag, "Installation failed");
                return false;
            }
        }

        protected bool ConfigureNode(string dataFolder, string clusterManifestLocation, string infrastructureManifestLocation, bool runFabricHostServiceAsManual)
        {
            string argument = string.Format(Constants.ConfigureNodeParametersTemplate, DeploymentType.Configure, dataFolder, clusterManifestLocation, infrastructureManifestLocation);

            if (runFabricHostServiceAsManual)
            {
                argument += string.Format(CultureInfo.InvariantCulture, " {0}", Constants.ConfigureNodeRunFabricHostServiceAsManualSwitch);
            }

            Logger.LogInfo(LogTag, "Running fabric deployer - {0} ", argument);

            int retVal = Utilities.ExecuteCommand(Utilities.GetFabricDeployerPath(), argument);

            if (retVal != 0)
            {
                Logger.LogError(LogTag, "Unable to configure windows fabric");
                return false;
            }

            return true;
        }

        protected bool FabricDeploy(DeploymentType deployType, string clusterManifestLocation, string dataFolder, List<InfrastructureNodeType> infrastructureNodes)
        {
            return FabricDeploy(deployType, clusterManifestLocation, dataFolder, infrastructureNodes, false);
        }

        protected bool FabricDeploy(DeploymentType deployType, string clusterManifestLocation, string dataFolder, List<InfrastructureNodeType> infrastructureNodes, bool runFabricHostServiceAsManual)
        {
            if (deployType == DeploymentType.None)
            {
                return true;
            }

            string infrastructureManifestFileName = null;
            if (infrastructureNodes != null && infrastructureNodes.Count > 0)
            {
                if (!this.GenerateInfrastructureFile(infrastructureNodes, out infrastructureManifestFileName))
                {
                    return false;
                }
            }

            string deployerPath = Utilities.GetFabricDeployerPath();
            if (deployerPath == null)
            {
                Logger.LogError(LogTag, "Unable to get fabric deployer path");
                return false;
            }

            string argument;
            if (deployType == DeploymentType.Create)
            {
                if (string.IsNullOrEmpty(infrastructureManifestFileName))
                {
                    argument = string.Format(Constants.ConfigureNodeParametersTemplateClusterManifestOnly, DeploymentType.Configure, dataFolder, clusterManifestLocation);
                }
                else
                {
                    argument = string.Format(Constants.ConfigureNodeParametersTemplate, DeploymentType.Configure, dataFolder, clusterManifestLocation, infrastructureManifestFileName);
                }

                if (runFabricHostServiceAsManual)
                {
                    argument += string.Format(CultureInfo.InvariantCulture, " {0}", Constants.ConfigureNodeRunFabricHostServiceAsManualSwitch);
                }
            }
            else
            {
                string environmentDataFolder = Environment.GetEnvironmentVariable(Constants.EnvironmentVariables.FabricDataRoot, EnvironmentVariableTarget.Machine);
                if (dataFolder != null)
                {
                    if (string.Compare(environmentDataFolder, dataFolder, StringComparison.OrdinalIgnoreCase) != 0)
                    {
                        Environment.SetEnvironmentVariable(Constants.EnvironmentVariables.FabricDataRoot, dataFolder, EnvironmentVariableTarget.Machine);
                    }
                }
                else
                {
                    dataFolder = environmentDataFolder;
                }

                if (string.IsNullOrEmpty(infrastructureManifestFileName))
                {
                    argument = string.Format(
                        CultureInfo.InvariantCulture,
                        Constants.FabricDeployerParametersTemplateClusterManifestOnly,
                        deployType,
                        Utilities.GetFabricBinRoot().TrimEnd('\\'),
                        dataFolder.TrimEnd('\\'),
                        clusterManifestLocation.TrimEnd('\\'));
                }
                else
                {
                    argument = string.Format(
                        CultureInfo.InvariantCulture,
                        Constants.FabricDeployerParametersTemplate,
                        deployType,
                        Utilities.GetFabricBinRoot().TrimEnd('\\'),
                        dataFolder.TrimEnd('\\'),
                        clusterManifestLocation.TrimEnd('\\'),
                        infrastructureManifestFileName);
                }
            }

            return Utilities.ExecuteCommand(deployerPath, argument) == 0;
        }

        protected bool StartFabricHostProcess(EventHandler onFailed)
        {
            InfrastructureWrapper.FabricHost.EnableRaisingEvents = true;
            InfrastructureWrapper.FabricHost.Exited += onFailed;

            var startInfo = new ProcessStartInfo
            {
                Arguments = "-c",
                CreateNoWindow = true,
                FileName = Path.Combine(Utilities.GetFabricBinRoot(), "FabricHost.exe"),
                WorkingDirectory = Utilities.GetFabricCodePath(),
                UseShellExecute = false
            };

            // We need to explicitly append the fabric code path to the environment variable PATH
            startInfo.EnvironmentVariables["PATH"] = Utilities.GetFabricCodePath() + ";" + startInfo.EnvironmentVariables["PATH"];

            InfrastructureWrapper.FabricHost.StartInfo = startInfo;
            try
            {
                if (InfrastructureWrapper.FabricHost.Start())
                {
                    Logger.LogInfo(LogTag, "Successfully started FabricHost Process");
                    return true;
                }
                else
                {
                    Logger.LogError(LogTag, "Failed to start FabricHost Process");
                    return false;
                }
            }
            catch (Exception e)
            {
                Logger.LogError(LogTag, "Exception {0} caught when trying to start fabrichost", e);
            }

            return false;
        }

        protected void StopFabricHostProcess()
        {
            Logger.LogInfo(LogTag, "Stopping FabricHost Process");
            InfrastructureWrapper.FabricHost.Close();
        }

        protected bool StartServiceByName(string serviceName)
        {
            const int StartingTimeoutSeconds = 60;

            ServiceController sc = new ServiceController(serviceName);
            try
            {
                if (sc.Status == ServiceControllerStatus.Running)
                {
                    return true;
                }

                if (sc.Status != ServiceControllerStatus.StartPending)
                {
                    sc.Start();
                }

                sc.WaitForStatus(ServiceControllerStatus.Running, TimeSpan.FromSeconds(StartingTimeoutSeconds));

                if (sc.Status == ServiceControllerStatus.Running ||
                    sc.Status == ServiceControllerStatus.StartPending)
                {
                    Logger.LogInfo(LogTag, "Started service - {0} successfully", serviceName);
                    return true;
                }

                sc.Stop();
                sc.WaitForStatus(ServiceControllerStatus.Stopped, new TimeSpan(2 * TimeSpan.TicksPerSecond));
                Logger.LogError(LogTag, "Failed to start service - {0}", serviceName);
                return false;
            }
            catch (Exception e)
            {
                Logger.LogError(LogTag, "Exception caught when trying to start service {0} -{1}", serviceName, e);
                return false;
            }
        }

        protected bool StopServiceByName(string serviceName)
        {
            const int StoppingTimeoutSeconds = 30;

            try
            {
                ServiceController sc = new ServiceController(serviceName);

                if (sc.Status == ServiceControllerStatus.Stopped)
                {
                    Logger.LogInfo(LogTag, "Stopped service - {0} successfully", serviceName);

                    return true;
                }

                if (sc.Status != ServiceControllerStatus.StopPending)
                {
                    sc.Stop();
                }

                sc.WaitForStatus(ServiceControllerStatus.Stopped, TimeSpan.FromSeconds(StoppingTimeoutSeconds));

                if (sc.Status == ServiceControllerStatus.Stopped)
                {
                    Logger.LogInfo(LogTag, "Stopped service - {0} successfully", serviceName);

                    return true;
                }

                Logger.LogError(LogTag, "Failed to stop service - {0}", serviceName);

                return false;
            }
            catch (Exception e)
            {
                Logger.LogError(LogTag, "Exception caught when trying to stop service {0} -{1}", serviceName, e);

                return false;
            }
        }

        protected bool StartFabricHostService()
        {
            return this.StartServiceByName(FabricHostServiceName);
        }

        protected bool StopFabricHostService()
        {
            return this.StopServiceByName(FabricHostServiceName);
        }

        protected bool InstallAndConfigureFabricInstallerService(string pluginDirectory)
        {
            ServiceController fabricInstallerService = Utilities.GetInstalledService(Constants.FabricInstallerServiceName);
            string fabricInstallerServiceBinPath = Path.Combine(this.GetFabricRoot(), FabricInstallerServiceExeDirectory, FabricInstallerServiceExe);
            if (fabricInstallerService == null)
            {
                Logger.LogInfo(LogTag, "Fabric Installer Service could not be found. Installing the service ..");
                // Service is not installed. Copy Binaries and Install the service
                /* This is only needed when the installer service was not shipped as a part of package (MSI). New XCopy package has FabricInstaller Service
                 * if (!this.CopyFabricInstallerServiceBinaries(pluginDirectory))
                 * {
                 *      Logger.LogError(LogTag, "An error occured while Copying Fabric installer service binaries");
                 *      return false;
                 * }
                */
                // Default start-up type is manual
                if (!this.InstallFabricInstallerService(fabricInstallerServiceBinPath))
                {
                    Logger.LogError(LogTag, "Fabric Installer service installation failed");
                    return false;
                }
            }
            else
            {
                if (!this.FixFabricInstallerServiceInstallation(fabricInstallerServiceBinPath))
                {
                    Logger.LogError(LogTag, "FixFabricInstallerServiceInstallation failed");
                    return false;
                }
            }

            Logger.LogInfo(LogTag, "Fabric Installer Service is installed. Configuring the service ..");
            // Service is installed. Configure the service just to be safe.
            // We do not copy the binaries here, because the only way they can be overriddren would be through a fabric upgrade, which is desired.
            if (!this.ConfigureFabricInstallerService())
            {
                Logger.LogError(LogTag, "Configuring Fabric Installer service failed");
                return false;
            }

            return true;
        }

        protected bool StartFabricInstallerService()
        {
            return this.StartServiceByName(FabricInstallerServiceName);
        }

        protected bool StartAndValidateInstallerServiceCompletion()
        {
            ServiceController fabricInstallerService = Utilities.GetInstalledService(Constants.FabricInstallerServiceName);
            if (fabricInstallerService == null)
            {
                Logger.LogError(LogTag, "Fabric Installer service was not present on the machine.");
                return false;
            }

            var installerSvcStartedTimeout = TimeSpan.FromMinutes(Constants.FabricInstallerServiceStartTimeoutInMinutes);
            var hostExistsTimeout = TimeSpan.FromMinutes(Constants.FabricInstallerServiceHostCreationTimeoutInMinutes);

            Task<bool> startInstallerTask = Task.Run(() =>
            {
                try
                {
                    fabricInstallerService.WaitForStatus(ServiceControllerStatus.Running, installerSvcStartedTimeout);
                    return true;
                }
                catch (System.ServiceProcess.TimeoutException)
                {
                    return false;
                }
            });

            Logger.LogInfo(LogTag, "Starting FabricInstallerSvc.");

            fabricInstallerService.Start();

            if (!startInstallerTask.Wait(installerSvcStartedTimeout)) // Locks until service is started or reaches timeout
            {
                Logger.LogError(LogTag, "Timed out waiting for Installer Service to start.");
                return false;
            }

            Logger.LogInfo(LogTag, "Successfully started FabricInstallerSvc.");

            try
            {
                fabricInstallerService.WaitForStatus(ServiceControllerStatus.Stopped, hostExistsTimeout); // Waits for FabricInstallerSvc to complete
                Logger.LogInfo(LogTag, "FabricInstallerSvc completed.");
            }
            catch (System.ServiceProcess.TimeoutException)
            {
                Logger.LogError(LogTag, "Timed out waiting for Installer Service to complete.");
                return false;
            }

            ServiceController hostSvc = Utilities.GetInstalledService(Constants.FabricHostServiceName);
            if (hostSvc == null)
            {
                Logger.LogError(LogTag, "FabricHostSvc was not installed by FabricInstallerSvc");
                return false;
            }

            if (hostSvc.Status == ServiceControllerStatus.Stopped)
            {
                Logger.LogError(LogTag, "FabricHostSvc is not running after executing FabricInstallerSvc.");
                return false;
            }

            Logger.LogInfo(LogTag, "Waiting for the FabricHostSvc to be in Running state.");

            try
            {
                hostSvc.WaitForStatus(ServiceControllerStatus.Running, TimeSpan.FromMinutes(Constants.FabricHostAwaitRunningTimeoutInMinutes));
            }
            catch (System.ServiceProcess.TimeoutException)
            {
                Logger.LogError(LogTag, "Timed out waiting for FabricHostSvc to start after {0} minutes.", Constants.FabricHostAwaitRunningTimeoutInMinutes);
                return false;
            }

            Logger.LogInfo(LogTag, "Successfully started FabricHostSvc.");
            return true;
        }

        protected bool StopFabricInstallerService()
        {
            return this.StopServiceByName(FabricInstallerServiceName);
        }

        protected bool CurrentClusterManifestFileExists(string nodeName)
        {
            string currentClusterManifestFile = GetCurrentClusterManifestFile(nodeName);

            return File.Exists(currentClusterManifestFile);
        }

        protected string GetCurrentClusterManifestFile(string nodeName)
        {
            string fabricDataRoot = Utilities.GetWindowsFabricDataRootRegKey();

            if (string.IsNullOrEmpty(fabricDataRoot))
            {
                return string.Empty;
            }

            string nodeFolder = Path.Combine(fabricDataRoot, nodeName);

            string fabricFolder = Path.Combine(nodeFolder, Constants.FabricFolderName);

            return Path.Combine(fabricFolder, Constants.CurrentClusterManifestFileName);
        }

        private bool GenerateInfrastructureFile(List<InfrastructureNodeType> infrastructureNodes, out string infrastructureFileName)
        {
            infrastructureFileName = Path.GetTempFileName();
            try
            {
                Logger.LogInfo(LogTag, "Generating infrastructure manifest in {0}", infrastructureFileName);
                XmlWriterSettings settings = new XmlWriterSettings();
                settings.Indent = true;
                using (XmlWriter writer = XmlWriter.Create(infrastructureFileName, settings))
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(InfrastructureInformationType));
                    InfrastructureInformationType infra = new InfrastructureInformationType();
                    infra.NodeList = infrastructureNodes.ToArray();
                    serializer.Serialize(writer, infra);
                }
            }
            catch (Exception e)
            {
                if (e is ArgumentException ||
                    e is System.IO.PathTooLongException ||
                    e is System.IO.DirectoryNotFoundException ||
                    e is System.IO.IOException ||
                    e is UnauthorizedAccessException ||
                    e is NotSupportedException ||
                    e is System.Security.SecurityException)
                {
                    Logger.LogFatalException(LogTag, e);
                    return false;
                }

                throw;
            }

            return true;
        }

        private bool InstallFabricInstallerService(string fabricInstallerServiceBinPath)
        {
            string serviceInstallParameters = string.Format(InstallServiceArguments, FabricInstallerServiceName, fabricInstallerServiceBinPath, FabricInstallerServiceDisplayName);
            int exitCode = Utilities.ExecuteCommand(ScExe, serviceInstallParameters);
            if (exitCode != 0)
            {
                Logger.LogError(LogTag, "Error {0} while installing fabric installer service", exitCode);
                return false;
            }

            return true;
        }

        private bool FixFabricInstallerServiceInstallation(string fabricInstallerServiceBinPath)
        {
            string serviceInstallParameters = string.Format(ConfigureServiceArguments, FabricInstallerServiceName, fabricInstallerServiceBinPath);
            int exitCode = Utilities.ExecuteCommand(ScExe, serviceInstallParameters);
            if (exitCode != 0)
            {
                Logger.LogError(LogTag, "Error {0} while fixing fabric installer service installation", exitCode);
                return false;
            }

            return true;
        }


        private bool ConfigureFabricInstallerService()
        {
            string serviceDescriptionParameters = string.Format(SetServiceDescriptionArguments, FabricInstallerServiceName, FabricInstallerServiceDescription);
            int exitCode = Utilities.ExecuteCommand(ScExe, serviceDescriptionParameters);
            if (exitCode != 0)
            {
                Logger.LogError(LogTag, "Error {0} while setting description for fabric installer service", exitCode);
                return false;
            }

            string serviceConfigParameters = string.Format(SetServiceFailureActionArguments, FabricInstallerServiceName);
            exitCode = Utilities.ExecuteCommand(ScExe, serviceConfigParameters);
            if (exitCode != 0)
            {
                Logger.LogError(LogTag, "Error {0} while setting failure actions for fabric installer service", exitCode);
                return false;
            }

            string setServiceStartToManualArgumentsParameter = string.Format(SetServiceStartToManualArguments, FabricInstallerServiceName);
            exitCode = Utilities.ExecuteCommand(ScExe, setServiceStartToManualArgumentsParameter);
            if (exitCode != 0)
            {
                Logger.LogError(LogTag, "Error {0} while setting service start-up to manual", exitCode);
                return false;
            }

            return true;
        }
        private bool CopyFabricInstallerServiceBinaries(string pluginDirectory)
        {
            string fabricInstallerServiceSrcDir = Path.Combine(pluginDirectory, FabricInstallerServiceExeRelativePathToPlugin);
            if (!Directory.Exists(fabricInstallerServiceSrcDir))
            {
                Logger.LogInfo(LogTag, "Fabric Installer Service Directory '{0}' not found", fabricInstallerServiceSrcDir);
                return false;
            }

            string fabricInstallerServiceBinPath = Path.Combine(fabricInstallerServiceSrcDir, FabricInstallerServiceExe);
            if (!File.Exists(fabricInstallerServiceBinPath))
            {
                Logger.LogInfo(LogTag, "Fabric Installer Service Executable not found at '{0}'", fabricInstallerServiceBinPath);
                return false;
            }

            string fabricInstallerServiceTargetDir = Path.Combine(this.GetFabricRoot(), FabricInstallerServiceExeDirectory);
            try
            {
                if (!Directory.Exists(fabricInstallerServiceTargetDir))
                {
                    Directory.CreateDirectory(fabricInstallerServiceTargetDir);
                }

                foreach (string srcFile in Directory.EnumerateFiles(fabricInstallerServiceSrcDir))
                {
                    string destFile = Path.Combine(fabricInstallerServiceTargetDir, Path.GetFileName(srcFile));
                    File.Copy(srcFile, destFile, true);
                }
            }
            catch (Exception ex)
            {
                Logger.LogInfo(LogTag, "Exception {0} occured while copying the contents from {1} to {2}", ex, fabricInstallerServiceSrcDir, fabricInstallerServiceTargetDir);
                return false;
            }

            return true;
        }

        private string GetFabricRoot()
        {
            if (this.fabricRoot == null)
            {
                this.fabricRoot = Utilities.GetFabricRoot();
            }

            return this.fabricRoot;
        }
    }
}