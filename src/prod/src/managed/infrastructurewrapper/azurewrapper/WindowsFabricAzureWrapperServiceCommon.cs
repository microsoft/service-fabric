// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.ServiceProcess;
    
    internal class WindowsFabricAzureWrapperServiceCommon
    {
        public const string WindowsFabricAzureWrapperServiceName = "WindowsFabricAzureWrapperService";
        public const string WindowsFabricAzureWrapperDisplayName = "Windows Fabric AzureWrapper Service";
        public const string WindowsFabricAzureWrapperDescription = "Windows Fabric entry point service V2 on Windows Azure platform";
        public const string WindowsFabricAzureWrapperBinaryName = "WindowsFabricAzureWrapperService.exe";        
        public const string Vc11RuntimeInstallerBinaryName = "vcredist_x64.exe";
        public const uint RebootRequired = 0xBC2;

        public const string WindowsFabricAzureWrapperServiceInstallUtilEventSource = "WindowsFabricAzureWrapperServiceInstallationUtility";

        public const string WindowsFabricAzureWrapperServiceTraceListenerName = "WindowsFabricAzureWrapperServiceTraceListener";

        public const string NodeNameTemplate = "{0}.{1}";
        public const string FaultDomainTemplate = "fd:/{0}";
        
        public const string WindowsFabricRootDirectoryLocalResourceName = "WinFabRoot";
        public const string WindowsFabricRootDirectoryName = "WFRoot";
        public const string WindowsFabricDataRootDirectoryName = "WindowsFabricData";
        public const string WindowsFabricDeploymentRootDirectoryName = "WindowsFabricDeployment";
        
        public const string NodeAddressEndpointName = "NodeAddress";
        public const string ApplicationPortsEndpointName = "ApplicationPorts";
        public const string ClientConnectionEndpointName = "ClientConnectionAddress";                
        public const string HttpGatewayEndpointName = "HttpGatewayListenAddress";
        public const string HttpApplicationGatewayEndpointName = "HttpApplicationGatewayListenAddress";
        public const string LeaseDriverEndpointName = "LeaseAgentAddress";

        public const string DefaultWindowsFabricInstallationPackageName = "WindowsFabric.msi";

        public const string ApplicationPortCountConfigurationSetting = "WindowsFabric.ApplicationPortCount";        

        public const string EphemeralPortCountConfigurationSetting = "WindowsFabric.EphemeralPortCount";        

        public const string NonWindowsFabricPortRangeConfigurationSettingNameTemplate = "{0}.PortCount";

        public const string BootstrapClusterManifestLocationConfigurationSetting = "WindowsFabric.ClusterManifestLocation";
        public const string DefaultClusterManifestLocation = "_default_";
        public const string NotReadyClusterManifestLocation = "_notReady_";
        public const string DefaultClusterManifestFileName = "ClusterManifest.xml";
        public const string DownloadedClusterManifestFileNameTemplate = "ClusterManifest-{0}.xml";
        public const string ClusterManifestLocationPattern = @"\A(DefaultEndpointsProtocol=.+;AccountName=(.+);AccountKey=.+);Container=(.+);File=(.+)\z";

        public const string ClusterManifestSchemaFileName = "ServiceFabricServiceModel.xsd";

        public const string WindowsFabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";

        public const string IsWindowsFabricRoleConfigurationSetting = "WindowsFabric.IsFabricRole";
        public const bool DefaultIsWindowsFabricRole = false;

        public const string DeploymentRetryIntervalSecondsConfigurationSetting = "WindowsFabric.DeploymentRetryIntervalSeconds";
        public const int DefaultDeploymentRetryIntervalSeconds = 180;
        public const string TextLogTraceLevelConfigurationSetting = "WindowsFabric.TextLogTraceLevel";
        public const string DefaultTextLogTraceLevel = "Info";
        public const string MultipleCloudServiceDeploymentConfigurationSetting = "WindowsFabric.MultipleCloudServiceDeployment";
        public const bool DefaultMultipleCloudServiceDeployment = false;
        public const string SubscribeBlastNotificationConfigurationSetting = "WindowsFabric.SubscribeBlastNotification";
        public const bool DefaultSubscribeBlastNotification = true;

        public const string MsiInstallationArgumentTemplate = "/q /i \"{0}\" /l*v \"{1}\" IACCEPTEULA=yes";
        public const string MsiInstallationLogFileNameTemplate = "WindowsFabricInstall_{0}.log";

        public const string TextLogFileNameTemplate = @"WindowsFabricAzureWrapperService_{0}.log";
        public const string LogEntryType = "WindowsFabricAzureWrapperService";

        public static readonly string[] FabricEndpointNames = 
        { 
            NodeAddressEndpointName,
            ApplicationPortsEndpointName,
            ClientConnectionEndpointName,            
            HttpGatewayEndpointName,
            HttpApplicationGatewayEndpointName,
            LeaseDriverEndpointName 
        };

        internal static ServiceController GetInstalledService(string serviceName)
        {
            ServiceController[] allServices = ServiceController.GetServices();

            return allServices.FirstOrDefault(service => service.ServiceName.Equals(serviceName, StringComparison.OrdinalIgnoreCase));
        }

        internal static int InstallService(string serviceBinaryPath)
        {
            string installUtilPath = Environment.ExpandEnvironmentVariables(@"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\Installutil.exe");
            string arguments = string.Format(CultureInfo.InvariantCulture, "/LogFile= /LogToConsole=false {0}", serviceBinaryPath);

            int exitCode = ExecuteCommand(installUtilPath, arguments);

            return exitCode;
        }

        internal static int UninstallService(string serviceBinaryPath)
        {
            string installUtilPath = Environment.ExpandEnvironmentVariables(@"%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\Installutil.exe");
            string arguments = string.Format(CultureInfo.InvariantCulture, "/LogFile= /LogToConsole=false /u {0}", serviceBinaryPath);

            int exitCode = ExecuteCommand(installUtilPath, arguments);

            return exitCode;
        }

        internal static int ExecuteNetshCommand(string arguments)
        {
            return ExecuteCommand("netsh", arguments);
        }

        internal static int ExecuteCommand(string command, string arguments)
        {
            return ExecuteCommand(command, arguments, TimeSpan.MaxValue);
        }

        internal static int ExecuteCommand(string command, string arguments, TimeSpan timeout)
        {
            return ExecuteCommand(command, arguments, Directory.GetCurrentDirectory(), timeout);
        }

        internal static int ExecuteCommand(string command, string arguments, string workingDirectory, TimeSpan timeout)
        {
            using (Process process = new Process())
            {
                process.StartInfo.FileName = command;
                process.StartInfo.Arguments = arguments;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.WorkingDirectory = workingDirectory;
                process.Start();

                if (timeout == TimeSpan.MaxValue)
                {
                    process.WaitForExit();
                }
                else
                {
                    if (!process.WaitForExit((int)timeout.TotalMilliseconds))
                    {
                        process.Kill();

                        string errorMessage = string.Format(CultureInfo.InvariantCulture, "Execution of command {0} {1} timed out after {2} milliseconds and is terminated.", command, arguments, (int)timeout.TotalMilliseconds);

                        throw new System.TimeoutException(errorMessage);
                    }                    
                }

                return process.ExitCode;
            }
        }

        internal static bool TryStartProcess(string command, string arguments)
        {
            try
            {
                using (Process process = new Process())
                {
                    process.EnableRaisingEvents = false;
                    process.StartInfo.FileName = command;
                    process.StartInfo.Arguments = arguments;
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.CreateNoWindow = true;
                    process.StartInfo.WorkingDirectory = Directory.GetCurrentDirectory();

                    return process.Start();
                }
            }
            catch (Exception e)
            {
                if (IsFatalException(e))
                {
                    throw;
                }

                return false;
            }
        }

        internal static bool IsFatalException(Exception e)
        {
            return
                (e is OutOfMemoryException) || // The exception that is thrown when there is not enough memory to continue the execution of a program.
                (e is StackOverflowException) || // The exception that is thrown when the execution stack overflows because it contains too many nested method calls. 
                (e is PlatformNotSupportedException) || // The exception that is thrown when a feature does not run on a particular platform.
                (e is AppDomainUnloadedException) ||  // The exception that is thrown when an attempt is made to access an unloaded application domain.  
                (e is BadImageFormatException) || // The exception that is thrown when the file image of a dynamic link library (DLL) or an executable program is invalid.                
                (e is DuplicateWaitObjectException) || // The exception that is thrown when an object appears more than once in an array of synchronization objects.                
                (e is EntryPointNotFoundException) || // The exception that is thrown when an attempt to load a class fails due to the absence of an entry method.                
                (e is InvalidProgramException) || // The exception that is thrown when a program contains invalid Microsoft intermediate language (MSIL) or metadata. Generally this indicates a bug in the compiler.               
                (e is TypeInitializationException) || // The exception that is thrown as a wrapper around the exception thrown by the class initializer.                 
                (e is TypeLoadException) || // TypeLoadException is thrown when the common language runtime cannot find the assembly, the type within the assembly, or cannot load the type.                
                (e is System.Threading.SynchronizationLockException); // The exception that is thrown when a method requires the caller to own the lock on a given Monitor, and the method is invoked by a caller that does not own that lock.
        }
    }
}