// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Security.Principal;

    internal class InvalidDeploymentParameterException : Exception
    {
        public InvalidDeploymentParameterException(string message)
            : base(message)
        {
        }
    }

    abstract class DeploymentOperation
    {
        public static void ExecuteOperation(DeploymentParameters parameters)
        {
            DeploymentOperation.ExecuteOperation(parameters, true);
        }

        public static void ExecuteOperation(DeploymentParameters parameters, bool disableFileTrace)
        {
            try
            {
                parameters.Initialize();
                DeployerTrace.UpdateFileLocation(parameters.DeploymentSpecification.GetTracesFolder());
                DeployerTrace.WriteInfo("Running deployer with {0}", parameters.ToString());
                if (parameters.FabricDataRoot == null)
                {
                    if (parameters.Operation == DeploymentOperations.ValidateClusterManifest
                        || parameters.Operation == DeploymentOperations.DockerDnsSetup
                        || parameters.Operation == DeploymentOperations.DockerDnsCleanup
                        || parameters.Operation == DeploymentOperations.ContainerNetworkSetup
                        || parameters.Operation == DeploymentOperations.ContainerNetworkCleanup
                        || parameters.Operation == DeploymentOperations.IsolatedNetworkSetup
                        || parameters.Operation == DeploymentOperations.IsolatedNetworkCleanup) // Some operations may not require FDR
                    {
                        try
                        {
                            ExecuteOperationPrivate(parameters);
                        }
                        catch (Exception)
                        {
                            parameters.DeleteTargetFile = false;
                            throw;
                        }
                    }
                    else
                    {
                        throw new ArgumentNullException(
                            String.Format("parameters.FabricDataRoot Operation {0} requires Fabric Data Root to be defined but was passed as null.", parameters.Operation));
                    }
                }
                else
                {
                    string fileLockPath = Helpers.GetRemotePath(parameters.FabricDataRoot, parameters.MachineName);
                    fileLockPath = Path.Combine(fileLockPath, "lock");

                    // Remove uses a different lock than the other operations, because it can be launched by the deployer separately when RemoveNodeConfig is running
                    if (parameters.Operation == DeploymentOperations.Remove)
                    {
                        fileLockPath += "_Remove";
                    }

                    try
                    {
                        Helpers.CreateDirectoryIfNotExist(parameters.FabricDataRoot, parameters.MachineName);
                        using (FileWriterLock fileWriterLock = new FileWriterLock(fileLockPath))
                        {
                            TimeSpan timeout = TimeSpan.FromMinutes(2);
                            if (!fileWriterLock.Acquire(timeout))
                            {
                                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Could not acquire lock: {0} in {1}", fileLockPath, timeout));
                            }

                            ExecuteOperationPrivate(parameters);
                        }
                    }
                    catch (Exception)
                    {
                        parameters.DeleteTargetFile = false;
                        throw;
                    }
                }
            }
            finally
            {
                if (disableFileTrace)
                {
                    DeployerTrace.CloseHandle();
                }
            }
        }

        private static void ExecuteOperationPrivate(DeploymentParameters parameters)
        {
            DeployerTrace.WriteInfo("Executing {0}", parameters.ToString());
            DeploymentOperation operation = null;
            switch (parameters.Operation)
            {
                case DeploymentOperations.Configure:
                    operation = new ConfigureOperation();
                    break;
                case DeploymentOperations.ValidateClusterManifest:
                    operation = new ValidateClusterManifestOperation();
                    break;
                case DeploymentOperations.Create:
                    operation = new CreateorUpdateOperation();
                    break;
                case DeploymentOperations.Update:
                    operation = new CreateorUpdateOperation();
                    break;
                case DeploymentOperations.UpdateInstanceId:
                    operation = new UpdateInstanceIdOperation();
                    break;
                case DeploymentOperations.UpdateNodeState:
                    operation = new UpdateNodeStateOperation();
                    break;
                case DeploymentOperations.None:
                    operation = new RestartOperation();
                    break;
                case DeploymentOperations.Remove:
                    operation = new RemoveOperation();
                    break;
#if !DotNetCoreClrLinux
                case DeploymentOperations.RemoveNodeConfig:
                    operation = new RemoveNodeConfigOperation();
                    break;
#endif
                case DeploymentOperations.Rollback:
                    operation = new RollbackOperation();
                    break;
                case DeploymentOperations.Validate:
                    operation = new ValidateOperation();
                    break;
#if !DotNetCoreClrIOT
                case DeploymentOperations.DockerDnsSetup:
                    operation = new DockerDnsSetupOperation();
                    break;

                case DeploymentOperations.DockerDnsCleanup:
                    operation = new DockerDnsCleanupOperation();
                    break;

                case DeploymentOperations.ContainerNetworkSetup:
                    operation = new ContainerNetworkSetupOperation();
                    break;

                case DeploymentOperations.ContainerNetworkCleanup:
                    operation = new ContainerNetworkCleanupOperation();
                    break;

                case DeploymentOperations.IsolatedNetworkSetup:
                    operation = new IsolatedNetworkSetupOperation();
                    break;

                case DeploymentOperations.IsolatedNetworkCleanup:
                    operation = new IsolatedNetworkCleanupOperation();
                    break;
#endif
                default:
                    throw new ArgumentException(StringResources.Warning_DeploymentOperationCantBeNull);
            }

            ClusterManifestType clusterManifest = parameters.ClusterManifestLocation == null ?
                null :
                XmlHelper.ReadXml<ClusterManifestType>(parameters.ClusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());

            if (parameters.Operation != DeploymentOperations.Validate
                && parameters.Operation != DeploymentOperations.ValidateClusterManifest
                && parameters.Operation != DeploymentOperations.UpdateInstanceId
                && parameters.Operation != DeploymentOperations.Remove
                && parameters.Operation != DeploymentOperations.RemoveNodeConfig
                && parameters.Operation != DeploymentOperations.Rollback
                && parameters.Operation != DeploymentOperations.DockerDnsSetup
                && parameters.Operation != DeploymentOperations.DockerDnsCleanup
                && parameters.Operation != DeploymentOperations.ContainerNetworkSetup
                && parameters.Operation != DeploymentOperations.ContainerNetworkCleanup
                && parameters.Operation != DeploymentOperations.None
                && clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure
                && parameters.InfrastructureManifestLocation == null)
            {
                throw new ArgumentException("InfrastructureManifestLocation");
            }

            InfrastructureInformationType infrastructureManifest = parameters.InfrastructureManifestLocation == null ?
                null :
                XmlHelper.ReadXml<InfrastructureInformationType>(parameters.InfrastructureManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());


            Infrastructure infrastructure = clusterManifest == null ? null : Infrastructure.Create(clusterManifest.Infrastructure, infrastructureManifest == null ? null : infrastructureManifest.NodeList, clusterManifest.NodeTypes);
            DeployerTrace.WriteInfo("Running operation {0}", operation.GetType());
#if !DotNetCoreClrLinux
            bool isChangeDeploymentOperationToRemove = false;
#endif

            try
            {
                operation.OnExecuteOperation(parameters, clusterManifest, infrastructure);
            }
            catch (ChangeDeploymentOperationToRemoveException)
            {
#if !DotNetCoreClrLinux
                isChangeDeploymentOperationToRemove = true;
#endif
                DeployerTrace.WriteInfo("Deployment operation modification to remove detected");                
            }

#if !DotNetCoreClrLinux
            if (isChangeDeploymentOperationToRemove)
            {
                var infraNode = infrastructure.InfrastructureNodes.SingleOrDefault(n => n.NodeName == parameters.NodeName);
                parameters.DeleteLog = false;
                parameters.MachineName = infraNode.IPAddressOrFQDN;
                parameters.DeploymentPackageType = FabricPackageType.XCopyPackage;
                operation = new RemoveNodeConfigOperation();
                DeployerTrace.WriteInfo("Deployment modified to RemoveNodeConfig. New parameters set: parameter.DeleteLog: {0}, parameters.MachineName: {1}, parameters.DeploymentPackageType: {2}",
                                       parameters.DeleteLog,
                                       parameters.MachineName,
                                       parameters.DeploymentPackageType);          
                operation.OnExecuteOperation(parameters, clusterManifest, infrastructure);
            }

            if (Utility.GetTestFailDeployer())
            {
                DeployerTrace.WriteInfo("Failing deployment as test hook is found");
                Utility.DeleteTestFailDeployer();
                throw new InvalidDeploymentException(StringResources.Error_FabricDeployer_TestHookFound_Formatted);
            }
#endif
        }

        protected static void WriteFabricHostSettingsFile(string hostSettingsFolder, SettingsType hostSettings, string machineName)
        {
            Dictionary<string, HostedServiceState> hostedServiceStates = GetHostedServicesState(hostSettingsFolder);

            List<SettingsTypeSection> sections = new List<SettingsTypeSection>();
            if (hostSettings.Section != null)
            {
                foreach (var hostSettingsSection in hostSettings.Section)
                {
                    if (hostedServiceStates.ContainsKey(hostSettingsSection.Name))
                    {
                        switch (hostedServiceStates[hostSettingsSection.Name])
                        {
                            case HostedServiceState.Up:
                                sections.Add(hostSettingsSection);
                                break;
                            case HostedServiceState.Killed:
                                List<SettingsTypeSectionParameter> parameters = new List<SettingsTypeSectionParameter>(hostSettingsSection.Parameter);
                                parameters.Add(new SettingsTypeSectionParameter() { Name = Constants.ParameterNames.Disabled, Value = true.ToString() });
                                sections.Add(new SettingsTypeSection() { Name = hostSettingsSection.Name, Parameter = parameters.ToArray() });
                                break;
                            default:
                                continue;
                        }
                    }
                    else
                    {
                        sections.Add(hostSettingsSection);
                    }
                }
            }

            SettingsType outputSettings = new SettingsType() { Section = sections.ToArray() };
            string fabricHostSettingPath = Path.Combine(hostSettingsFolder, Constants.FileNames.FabricHostSettings);

            if (!string.IsNullOrEmpty(machineName) && !machineName.Equals(Helpers.GetMachine()))
            {
                fabricHostSettingPath = Helpers.GetRemotePath(fabricHostSettingPath, machineName);
            }

            XmlHelper.WriteXmlExclusive<SettingsType>(fabricHostSettingPath, outputSettings);
            DeployerTrace.WriteInfo("Host Settings file generated at {0}", fabricHostSettingPath);
        }

        private static Dictionary<string, HostedServiceState> GetHostedServicesState(string hostSettingsFolder)
        {
            string hostedServiceStateFile = Path.Combine(hostSettingsFolder, Constants.FileNames.HostedServiceState);
            Dictionary<string, HostedServiceState> hostedServiceStates = new Dictionary<string, HostedServiceState>();
            if (File.Exists(hostedServiceStateFile))
            {
                string[] hostedServiceState = File.ReadAllLines(hostedServiceStateFile);
                foreach (var hostedService in hostedServiceState)
                {
                    if (string.IsNullOrWhiteSpace(hostedService))
                    {
                        continue;
                    }
                    string[] stateInfo = hostedService.Split(",".ToCharArray());
                    string nodeName = stateInfo[0];
                    string serviceName = stateInfo[1];
                    string hostedServiceName = string.Format(
                        CultureInfo.InvariantCulture,
                        Constants.SectionNames.HostSettingsSectionPattern,
                        nodeName,
                        serviceName);
                    hostedServiceStates.Add(hostedServiceName, (HostedServiceState)Enum.Parse(typeof(HostedServiceState), stateInfo[2]));
                }
            }
            return hostedServiceStates;
        }

        protected abstract void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusteManifest, Infrastructure infrastructure);
    }
}