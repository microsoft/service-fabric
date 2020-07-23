// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LocalClusterManager
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using System.Linq;
    using System.ServiceProcess;
    using System.Threading.Tasks;
    using LocalClusterManager.Utilities;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Interop;
    using System.Fabric.Common;
    using System.Net.Http;
    using Microsoft.Win32;

    internal class ClusterManager
    {
        #region Public Properties

        public static bool IsServiceFabricInstalled
        {
            get
            {
                var fabricHostservice = ServiceController.GetServices().FirstOrDefault(s => s.ServiceName == ProjectUtility.ServiceFabricHostServiceName);
                return (fabricHostservice != null);
            }
        }

        public static bool IsClusterSetup
        {
            get
            {
                return (IsServiceFabricInstalled && ProjectUtility.IsNodeConfigurationDeployed());
            }
        }

        public static bool IsMeshSDKInstalled
        {
            get
            {
                try
                {
                    return (IsServiceFabricInstalled && null != Registry.GetValue(ProjectUtility.ServiceFabricSdkRegKey, ProjectUtility.FabricMeshSDKVersionRegValue, null));
                }
                catch (Exception)
                {
                }

                return false;
            }
        }

        public static bool IsClusterRunning
        {
            get
            {
                if (IsClusterSetup)
                {
                    using (ServiceController fabricHostService = new ServiceController(ProjectUtility.ServiceFabricHostServiceName))
                    {
                        return (fabricHostService.Status == ServiceControllerStatus.Running);
                    }
                }

                return false;
            }
        }

        public static bool IsClusterRunningOrStartPending
        {
            get
            {
                if (IsClusterSetup)
                {
                    using (ServiceController fabricHostService = new ServiceController(ProjectUtility.ServiceFabricHostServiceName))
                    {
                        if (fabricHostService.Status == ServiceControllerStatus.Running ||
                            fabricHostService.Status == ServiceControllerStatus.StartPending)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        }

        public static bool IsClusterStopppedOrStopPending
        {
            get
            {
                if (IsClusterSetup)
                {
                    using (ServiceController fabricHostService = new ServiceController(ProjectUtility.ServiceFabricHostServiceName))
                    {
                        if (fabricHostService.Status == ServiceControllerStatus.Stopped ||
                            fabricHostService.Status == ServiceControllerStatus.StopPending)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        }

        #endregion

        #region Public Methods

        public static Task StartClusterAsync()
        {
            return Task.Run(
                () =>
                {
                    CleanTicketFiles();

                    using (ServiceController fabricHostService = new ServiceController(ProjectUtility.ServiceFabricHostServiceName))
                    {
                        fabricHostService.Start();
                        fabricHostService.WaitForStatus((ServiceControllerStatus.Running));
                    }
                });
        }

        public static Task<bool> TryConnectClusterWithRetryAsync()
        {
            return Task.Run(
                () =>
                {
                    FabricClient localClient = new FabricClient();

                    for (int i = 0; i < ProjectUtility.ClusterConnectionRetryCount; i++)
                    {
                        Task.Delay(ProjectUtility.ClusterConnectionRetryDelay).Wait();

                        if (!IsClusterRunningOrStartPending)
                        {
                            return false;
                        }

                        try
                        {
                            localClient.QueryManager.GetNodeListAsync().Wait();
                            return true;
                        }
                        catch (Exception)
                        {
                            // Retry
                        }
                    }

                    return false;
                });
        }

        public static Task StopClusterAsync()
        {
            return Task.Run(
                () =>
                {
                    using (ServiceController fabricHostService = new ServiceController(ProjectUtility.ServiceFabricHostServiceName))
                    {
                        fabricHostService.Stop();
                        fabricHostService.WaitForStatus((ServiceControllerStatus.Stopped));
                    }

                    CleanTicketFiles();
                });
        }

        public static Task<CommandExecutionResult> ResetClusterAsync(ClusterMode clusterMode)
        {
            return RemoveClusterAsync().ContinueWith(
                cleanTask =>
                {
                    // FabricHost child processes sometime takes little while to go away. 
                    // Wait for 15 seconds before starting setup.
                    Task.Delay(15000).Wait();
                    return SetupClusterAsync(clusterMode).Result;
                });
        }

        public static Task<CommandExecutionResult> SetupClusterAsync(ClusterMode clusterMode)
        {
            return Task.Run(
                () =>
                {
                    String setupClusterScriptPath = ProjectUtility.GetSetupClusterScriptPath();
                    string arguments = String.Format("-Auto -PathToClusterLogRoot {0} -SetupLogFileName {1} ", ProjectUtility.DevClusterLogRoot, ProjectUtility.DefaultSetupLogFileName);
                    var timeout = ProjectUtility.ClusterSetupTimeout;
                    switch (clusterMode)
                    {
                        case ClusterMode.OneNode:
                            arguments += "-CreateOneNodeCluster";
                            break;

                        case ClusterMode.MeshOneNode:
                            arguments += "-CreateOneNodeCluster -CreateMeshCluster";
                            timeout = ProjectUtility.MeshClusterSetupTimeout;
                            break;

                        case ClusterMode.MeshFiveNode:
                            arguments += "-CreateMeshCluster";
                            timeout = ProjectUtility.MeshClusterSetupTimeout;
                            break;
                    }

                    return ProjectUtility.ExecutePowerShellScript(setupClusterScriptPath, arguments, timeout);
                });
        }

        public static Task<CommandExecutionResult> RemoveClusterAsync()
        {
            return Task.Run(
                () =>
                {
                    String removeClusterScriptPath = ProjectUtility.GetRemoveClusterScriptPath();
                    return ProjectUtility.ExecutePowerShellScript(removeClusterScriptPath, String.Format("-Auto -PathToClusterLogRoot {0} -SetupLogFileName {1}", ProjectUtility.DevClusterLogRoot, ProjectUtility.DefaultSetupLogFileName), ProjectUtility.ClusterCleanupTimeout);
                });
        }

        public static async Task<string> GetServiceFabricExplorerUrl()
        {
            if (await HasValidHttpGatewayUrl())
            {
                return httpGatewayUrl;
            }

            var tcpClient = new FabricClient();
            var clusterManifestString = await tcpClient.ClusterManager.GetClusterManifestAsync();

            var manifestType = Utility.ReadXmlString<ClusterManifestType>(
                clusterManifestString,
                SchemaLocation.GetWindowsFabricSchemaLocation());

            var httpGatewayEndpoint = manifestType.NodeTypes.First().Endpoints.HttpGatewayEndpoint;
            var httpGatewayProtocol = httpGatewayEndpoint.Protocol.ToString();
            var httpGatewayPort = httpGatewayEndpoint.Port;

            return string.Format(serviceFabricExplorerUrlFormat, httpGatewayProtocol, httpGatewayPort);
        }

        #endregion

        #region Private Methods

        private static void CleanTicketFiles()
        {
            String localClusterDataRoot = ProjectUtility.GetLocalClusterDataRoot();
            
            if (Directory.Exists(localClusterDataRoot))
            {
                try
                {
                    var ticketFileList = new List<string>();

                    foreach (var dir in Directory.GetDirectories(localClusterDataRoot))
                    {
                        var ticketFileDir = Path.Combine(dir, ProjectUtility.TicketFileDirectoryRelativePath);

                        if(Directory.Exists(ticketFileDir))
                        {
                            ticketFileList.AddRange(Directory.GetFiles(ticketFileDir, "*.tkt"));
                        }
                    }

                    Parallel.ForEach(ticketFileList, file => ProjectUtility.DeleteFileWithRetry(file));
                }
                catch (Exception)
                {
                    // Nothing to do for now
                }
            }
        }
        
        private static async Task<bool> HasValidHttpGatewayUrl()
        {
            if(!string.IsNullOrWhiteSpace(httpGatewayUrl))
            {
                try
                {
                    var httpReqMsg = new HttpRequestMessage(HttpMethod.Head, httpGatewayUrl);
                    await new HttpClient().SendAsync(httpReqMsg);

                    return true;
                }
                catch(Exception)
                {
                    return false;
                }
            }

            return false;
        }

        #endregion

        #region Private Fields

        private static string httpGatewayUrl = null;
        private const string serviceFabricExplorerUrlFormat = "{0}://localhost:{1}/Explorer/";

        #endregion
    }
}