// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Linq;
    using System.IO;
    using System.Xml;
    using System.Text;
    using System.Security;

    internal class InvalidDeploymentException : Exception
    {
        public InvalidDeploymentException(string message)
            : base(message)
        {
        }
    }

    internal class ChangeDeploymentOperationToRemoveException : Exception
    {
        public ChangeDeploymentOperationToRemoveException(string message)
            : base(message)
        {
        }
    }

    internal class DeploymentParameters
    {
        public DeploymentOperations Operation { get; set; }

        public string FabricBinRoot
        {
            get { return values[FabricBinRootString]; }
            set { values[FabricBinRootString] = value; }
        }

        public string FabricDataRoot
        {
            get { return values[FabricDataRootString]; }
            private set { values[FabricDataRootString] = value; }
        }

        public string FabricLogRoot
        {
            get { return values[FabricLogRootString]; }
            private set { values[FabricLogRootString] = value; }
        }

        public bool EnableCircularTraceSession
        {
            get { return values[EnableCircularTraceSessionString]; }
            private set { values[EnableCircularTraceSessionString] = value; }
        }

        public string ClusterManifestLocation
        {
            get { return values[ClusterManifestString]; }
            set { values[ClusterManifestString] = value; }
        }

        public string OldClusterManifestLocation
        {
            get { return values[OldClusterManifestString]; }
            set { values[OldClusterManifestString] = value; }
        }

        public string InfrastructureManifestLocation
        {
            get { return values[InfrastructureManifestString]; }
            set { values[InfrastructureManifestString] = value; }
        }

        public string InstanceId
        {
            get { return values[InstanceIdString] ?? System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultInstanceId; }
            set { values[InstanceIdString] = value; }
        }

        public string TargetVersion
        {
            get { return values[TargetVersionString] ?? System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultTargetVersion; }
            set { values[TargetVersionString] = value; }
        }

        public string NodeName
        {
            get { return values[NodeNameString]; }
            set { values[NodeNameString] = value; }
        }

        public string NodeTypeName
        {
            get { return values[NodeTypeNameString]; }
            set { values[NodeTypeNameString] = value; }
        }

        public string MachineName
        {
            get { return values[MachineNameString]; }
            set { values[MachineNameString] = value; }
        }

        public string FabricPackageRoot
        {
            get { return values[FabricPackageRootString]; }
            set { values[FabricPackageRootString] = value; }
        }

        public string BootstrapMSIPath
        {
            get { return values[BootstrapMSIPathString]; }
            set { values[BootstrapMSIPathString] = value; }
        }

        public string JsonClusterConfigLocation
        {
            get { return values[JsonClusterConfigLocationString]; }
            set { values[JsonClusterConfigLocationString] = value; }
        }

        public string SkipDeleteFabricDataRoot
        {
            get { return values[SkipDeleteFabricDataRootString]; }
            set { values[SkipDeleteFabricDataRootString] = value; }
        }

        public string RunAsUserName { get { return values[RunAsUserNameString]; } }

        public SecureString RunAsPassword { get { return values[RunAsPaswordString]; } }

        public string RunAsUserType { get { return values[RunAsUserTypeString]; } }

        public string ServiceStartupType { get { return values[ServiceStartupTypeString]; } }

        public string OutputFile { get { return values[OutputFileString]; } }

        public string ErrorOutputFile { get { return values[ErrorOutputFileString]; } }

        public string CurrentVersion { get { return values[CurrentVersionString]; } }

        public FabricDeploymentSpecification DeploymentSpecification { get; set; }

        public bool DeleteTargetFile { get; set; }

        public bool DeleteLog { get; set; }

        public FabricPackageType DeploymentPackageType { get; set; }

        public bool SkipFirewallConfiguration { get; set; }

        public ContainerDnsSetup ContainerDnsSetup { get; set; }

        public bool ContainerNetworkSetup { get; set; }

        public string ContainerNetworkName { get; set; }

#if !DotNetCoreClrLinux
        public bool SkipContainerNetworkResetOnReboot { get; set; }

        public bool SkipIsolatedNetworkResetOnReboot { get; set; }
#endif
        // Properties to track state of preview features that need to lightup at runtime
        public bool EnableUnsupportedPreviewFeatures { get; set; }

        public bool IsSFVolumeDiskServiceEnabled { get; set; }

        public bool IsolatedNetworkSetup { get; set; }

        public string IsolatedNetworkName { get; set; }

        public string IsolatedNetworkInterfaceName { get; set; }

        /// <summary>
        /// Flag that continues the DNS setup even though the Containers feature is not present.
        /// </summary>        
        public bool ContinueIfContainersFeatureNotInstalled
        {
            get
            {
                return GetBoolValue(ContinueIfContainersFeatureNotInstalledString);
            }
            set
            {
                values[ContinueIfContainersFeatureNotInstalledString] = value.ToString();
            }
        }

        public bool UseContainerServiceArguments { get; set; }

        public string ContainerServiceArguments { get; set; }

        public bool EnableContainerServiceDebugMode { get; set; }

        public DeploymentParameters() : this(true) {}

        public DeploymentParameters(bool setBinRoot)
        {
            DeploymentSpecification = FabricDeploymentSpecification.Create();
            DeleteTargetFile = false;
            if (setBinRoot)
            {
                try
                {
                    this.FabricBinRoot = FabricEnvironment.GetBinRoot();
                }
                catch (Exception) { }
            }
        }

        public void SetParameters(Dictionary<string, dynamic> parameters, DeploymentOperations operation)
        {
            foreach (var key in parameters.Keys)
            {
                values[key] = parameters[key];
            }
            Operation = operation;
        }

        public void Initialize()
        {
            DeploymentOperations operation = Operation;

            if (operation == DeploymentOperations.Create || operation == DeploymentOperations.Update || operation == DeploymentOperations.Configure)
            {
                if (this.ClusterManifestLocation == null || !File.Exists(this.ClusterManifestLocation))
                {
                    throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_ClusterManifestNotFound_Formatted);
                }
            }

            switch (operation)
            {
                case DeploymentOperations.Configure:
                    {
                        ClusterManifestType manifest = XmlHelper.ReadXml<ClusterManifestType>(this.ClusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
                        SetupSettings settings = new SetupSettings(manifest);
                        this.FabricDataRoot = settings.FabricDataRoot == null ? this.FabricDataRoot : settings.FabricDataRoot;
                        this.FabricLogRoot = settings.FabricLogRoot == null ? this.FabricLogRoot : settings.FabricLogRoot;
                        bool isDataRootProvided = this.FabricDataRoot != null;
                        ResolveDataRoot(isDataRootProvided);
                        DeploymentSpecification.SetDataRoot(this.FabricDataRoot);
                        ResolveLogRoot(isDataRootProvided);
                        DeploymentSpecification.SetLogRoot(this.FabricLogRoot);
                    }
                    break;
                case DeploymentOperations.RemoveNodeConfig:
                    // for RemoveNodeConfig the user does not pass the data root in, so it has to be determined
                    this.FabricDataRoot = FabricEnvironment.GetDataRoot(this.MachineName);
                    this.FabricLogRoot = FabricEnvironment.GetLogRoot(this.MachineName);
                    DeploymentSpecification.SetDataRoot(this.FabricDataRoot);
                    DeploymentSpecification.SetLogRoot(this.FabricLogRoot);
                    break;
                case DeploymentOperations.Create:
                case DeploymentOperations.None:
                case DeploymentOperations.Remove:
                case DeploymentOperations.Rollback:
                case DeploymentOperations.Update:
                case DeploymentOperations.UpdateNodeState:
                case DeploymentOperations.UpdateInstanceId:
                    SetCommonRoots();
                    break;
                case DeploymentOperations.ValidateClusterManifest:
                    SetCommonRoots(Directory.GetCurrentDirectory());
                    break;
                case DeploymentOperations.Validate:
                    if (this.OldClusterManifestLocation != null && File.Exists(this.OldClusterManifestLocation) && this.FabricDataRoot == null)
                    {
                        SetCommonRoots(Path.GetDirectoryName(this.OldClusterManifestLocation));
                    }
                    else
                    {
                        SetCommonRoots();
                    }
                    break;
                case DeploymentOperations.DockerDnsSetup:
                case DeploymentOperations.DockerDnsCleanup:
                case DeploymentOperations.ContainerNetworkSetup:
                case DeploymentOperations.ContainerNetworkCleanup:
                case DeploymentOperations.IsolatedNetworkSetup:
                case DeploymentOperations.IsolatedNetworkCleanup:
                    break;
                default:
                    throw new InvalidOperationException(String.Format("The operation {0} has no explicit deployment criteria.", operation.ToString()));
            }

            if (this.InfrastructureManifestLocation == null && this.NodeName != null)
            {
                this.InfrastructureManifestLocation = this.DeploymentSpecification.GetInfrastructureManfiestFile(this.NodeName);
            }

            if (operation == DeploymentOperations.UpdateNodeState || 
                operation == DeploymentOperations.Remove || 
                operation == DeploymentOperations.UpdateInstanceId ||
                (operation == DeploymentOperations.RemoveNodeConfig && !string.IsNullOrEmpty(this.FabricDataRoot) && Directory.Exists(this.FabricDataRoot)))
            {
                this.ClusterManifestLocation = Helpers.GetCurrentClusterManifestPath(this.FabricDataRoot);
                this.InfrastructureManifestLocation = Helpers.GetInfrastructureManifestPath(this.FabricDataRoot);
            }

            if (!string.IsNullOrEmpty(this.ClusterManifestLocation))
            {
                ClusterManifestType manifest = XmlHelper.ReadXml<ClusterManifestType>(this.ClusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
                SetupSettings settings = new SetupSettings(manifest);
                this.SkipFirewallConfiguration = settings.SkipFirewallConfiguration;
                this.ContainerDnsSetup = settings.ContainerDnsSetup;
                this.ContainerNetworkSetup = settings.ContainerNetworkSetup;
                this.ContainerNetworkName = settings.ContainerNetworkName;
#if !DotNetCoreClrLinux
                this.SkipContainerNetworkResetOnReboot = settings.SkipContainerNetworkResetOnReboot;
                this.SkipIsolatedNetworkResetOnReboot = settings.SkipIsolatedNetworkResetOnReboot;
#endif
                this.IsolatedNetworkSetup = settings.IsolatedNetworkSetup;
                this.IsolatedNetworkName = settings.IsolatedNetworkName;
                this.IsolatedNetworkInterfaceName = settings.IsolatedNetworkInterfaceName;
                this.UseContainerServiceArguments = settings.UseContainerServiceArguments;
                this.ContainerServiceArguments = settings.ContainerServiceArguments;
                this.EnableContainerServiceDebugMode = settings.EnableContainerServiceDebugMode;

                SetOptionalFeatureParameters(settings);
            }
            else
            {
                this.SkipFirewallConfiguration = false;                
            }

            Validate();
        }

        public void CreateFromFile()
        {
            DeleteTargetFile = true;
            this.FabricDataRoot = FabricEnvironment.GetDataRoot();
            this.DeploymentSpecification.SetDataRoot(FabricDataRoot);
            SetFabricLogRoot();
            this.DeploymentSpecification.SetLogRoot(this.FabricLogRoot);
            var targetInformationFilePath = Path.Combine(this.FabricDataRoot, Constants.FileNames.TargetInformation);
            if (!File.Exists(targetInformationFilePath))
            {
                this.Operation = DeploymentOperations.None;
                this.ClusterManifestLocation = Helpers.GetCurrentClusterManifestPath(this.FabricDataRoot);
                this.InfrastructureManifestLocation = Helpers.GetInfrastructureManifestPath(this.FabricDataRoot);
            }
            else
            {
                this.Operation = DeploymentOperations.Create;
                var targetInformation = XmlHelper.ReadXml<TargetInformationType>(targetInformationFilePath, SchemaLocation.GetWindowsFabricSchemaLocation());
                var targetInstallation = targetInformation.TargetInstallation;
                if (targetInstallation == null)
                {
                    throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_InvalidTargetInstallation_Formatted);
                }

                this.InfrastructureManifestLocation = targetInstallation.InfrastructureManifestLocation;
                this.InstanceId = targetInstallation.InstanceId;
                this.NodeName = targetInstallation.NodeName;
                if (string.IsNullOrEmpty(this.InfrastructureManifestLocation) && !string.IsNullOrEmpty(this.NodeName))
                {
                    this.InfrastructureManifestLocation = this.DeploymentSpecification.GetInfrastructureManfiestFile(this.NodeName);
                }

                this.TargetVersion = targetInstallation.TargetVersion;
                this.ClusterManifestLocation = targetInstallation.ClusterManifestLocation;
            }

            if (!string.IsNullOrEmpty(this.ClusterManifestLocation))
            {
                var manifest = XmlHelper.ReadXml<ClusterManifestType>(this.ClusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
                var settings = new SetupSettings(manifest);
                this.SkipFirewallConfiguration = settings.SkipFirewallConfiguration;

                SetOptionalFeatureParameters(settings);
            }
        }

        private void Validate()
        {
            switch (Operation)
            {
                case DeploymentOperations.Create:
                    break;
                case DeploymentOperations.Update:
                    break;
                case DeploymentOperations.Configure:
                    break;
                case DeploymentOperations.ValidateClusterManifest:
                    if (this.ClusterManifestLocation == null || !File.Exists(this.ClusterManifestLocation))
                    {
                        throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_ClusterManifestNotFound_Formatted);
                    }
                    Version version;
                    if (!string.IsNullOrEmpty(this.TargetVersion)
                        && !this.TargetVersion.Equals(System.Fabric.FabricValidatorConstants.FabricUpgradeDefaultTargetVersion)
                        && (!Version.TryParse(this.TargetVersion, out version) || version.Build == -1))
                    {
                        throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_InvalidVersion_Formatted);
                    }
                    break;
                case DeploymentOperations.Remove:
                    break;
                case DeploymentOperations.Rollback:
                    break;
                case DeploymentOperations.UpdateNodeState:
                    if (this.FabricDataRoot == null)
                    {
                        throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_FabricDataRootNotFound_Formatted);
                    }
                    break;
                case DeploymentOperations.Validate:
                    if (this.ClusterManifestLocation == null || !File.Exists(this.ClusterManifestLocation))
                    {
                        throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_ClusterManifestNotFound_Formatted);
                    }

                    // Either OldClusterManifestLocation file must exist, or one of FabricDataRoot/NodeName must be set
                    if ((this.OldClusterManifestLocation == null || !File.Exists(this.OldClusterManifestLocation))
                        && this.FabricDataRoot == null && this.NodeName == null)
                    {
                        throw new InvalidDeploymentParameterException(StringResources.Error_FabricDeployer_NodeNameAndOldCMNotFound_Formatted);
                    }
                    break;
                case DeploymentOperations.None:
                    break;
            }
        }

        public bool IsSupportedParameter(string parameter)
        {
            return values.ContainsKey(parameter);
        }

        private void SetCommonRoots(string backupFabricDataRoot = null)
        {
            if (this.FabricDataRoot == null) // FabricDataRoot not provided
            {
                try
                {
                    this.FabricDataRoot = FabricEnvironment.GetDataRoot(this.MachineName);
                }
                catch
                {
                    if(!string.IsNullOrEmpty(backupFabricDataRoot) && Directory.Exists(backupFabricDataRoot))
                    {
                        // Set FDR based on backup, if no value is retrievable
                        this.FabricDataRoot = backupFabricDataRoot;
                    }
                    else
                    {
                        throw;
                    }
                }

                this.DeploymentSpecification.SetDataRoot(this.FabricDataRoot); // Also sets deployment specification Log Root to [FabricDataRoot]\log

                if (string.IsNullOrEmpty(this.FabricLogRoot))
                {
                    SetFabricLogRoot(); // Tries to get data root from environment, else pulls from deployment specification
                }
            }
            else
            {
                this.DeploymentSpecification.SetDataRoot(this.FabricDataRoot); // Also sets deployment specification Log Root
                this.FabricLogRoot = this.FabricLogRoot ?? this.DeploymentSpecification.GetLogRoot();
            }
            this.DeploymentSpecification.SetLogRoot(this.FabricLogRoot);
        }

        private void SetOptionalFeatureParameters(SetupSettings settings)
        {
            this.EnableCircularTraceSession = settings.EnableCircularTraceSession;
            DeploymentSpecification.SetEnableCircularTraceSession(this.EnableCircularTraceSession);

            // Initialize the properties used to reflect state of preview features that need to lightup at runtime
            this.EnableUnsupportedPreviewFeatures = settings.EnableUnsupportedPreviewFeatures;
            this.IsSFVolumeDiskServiceEnabled = settings.IsSFVolumeDiskServiceEnabled;
        }

        private void SetFabricLogRoot()
        {
            try
            {
                this.FabricLogRoot = FabricEnvironment.GetLogRoot(this.MachineName);
            }
            catch (FabricException)
            {
                this.FabricLogRoot = this.DeploymentSpecification.GetLogRoot(); // This is to prevent break in backward compatibility of our internal tools.
            }
        }

        private void ResolveDataRoot(bool isDataRootProvided)
        {
            if (!isDataRootProvided)
            {
                this.FabricDataRoot = FabricEnvironment.GetDataRoot();
            }

            Helpers.CreateDirectoryIfNotExist(this.FabricDataRoot, this.MachineName);
        }

        private void ResolveLogRoot(bool isDataRootProvided)
        {
            if (this.FabricLogRoot == null)
            {
                if (isDataRootProvided)
                {
                    this.FabricLogRoot = DeploymentSpecification.GetLogRoot();
                }
                else
                {
                    this.FabricLogRoot = FabricEnvironment.GetLogRoot();
                }
            }

            Helpers.CreateDirectoryIfNotExist(this.FabricLogRoot, this.MachineName);
        }

        private bool GetBoolValue(string key, bool defaultValue = false)
        {
            bool val;
            bool status = bool.TryParse(values[key], out val);
            return status ? val : defaultValue;
        }

        public override string ToString()
        {
            StringBuilder builder = new StringBuilder();
            builder.AppendFormat("{0} ", Operation);
            foreach (var parameter in values)
            {
                builder.AppendFormat("{0}:{1} ", parameter.Key, parameter.Value);
            }
            builder.Length--;
            return builder.ToString();
        }

        internal const string FabricBinRootString = "/fabricBinRoot";
        internal const string FabricDataRootString = "/fabricDataRoot";
        internal const string FabricLogRootString = "/fabricLogRoot";
        internal const string ClusterManifestString = "/cm";
        internal const string OldClusterManifestString = "/oldClusterManifestString";
        internal const string InfrastructureManifestString = "/im";
        internal const string InstanceIdString = "/instanceId";
        internal const string TargetVersionString = "/targetVersion";
        internal const string NodeNameString = "/nodeName";
        internal const string NodeTypeNameString = "/nodeTypeName";
        internal const string RunAsUserTypeString = "/runAsType";
        internal const string RunAsUserNameString = "/runAsAccountName";
        internal const string RunAsPaswordString = "/runAsPassword";
        internal const string ServiceStartupTypeString = "/serviceStartupType";
        internal const string OutputFileString = "/output";
        internal const string CurrentVersionString = "/currentVersion";
        internal const string ErrorOutputFileString = "/error";

        internal const string JsonClusterConfigLocationString = "/jsonClusterConfigLocation";
        internal const string BootstrapMSIPathString = "/bootstrapMSIPath";
        internal const string MachineNameString = "/machineName";
        internal const string FabricPackageRootString = "/fabricPackageRoot";
        internal const string EnableCircularTraceSessionString = "/enableCircularTraceSession";        
        internal const string SkipDeleteFabricDataRootString = "/skipDeleteData";
        internal const string ContinueIfContainersFeatureNotInstalledString = "/continueIfContainersFeatureNotInstalled";

        private Dictionary<string, dynamic> values = new Dictionary<string, dynamic>(StringComparer.OrdinalIgnoreCase) 
        {
            {FabricBinRootString, null},
            {FabricDataRootString, null},
            {FabricLogRootString, null},
            {ClusterManifestString, null},
            {OldClusterManifestString, null},
            {InfrastructureManifestString, null},
            {InstanceIdString, null},
            {TargetVersionString, null},
            {NodeNameString, null},
            {NodeTypeNameString, null},
            {RunAsUserTypeString, null},
            {RunAsUserNameString, null},
            {RunAsPaswordString, null},
            {ServiceStartupTypeString, null},
            {OutputFileString, null},
            {CurrentVersionString, null},
            {ErrorOutputFileString, null},
            {BootstrapMSIPathString, null},
            {MachineNameString, null},
            {FabricPackageRootString, null},
            {JsonClusterConfigLocationString, null},
            {EnableCircularTraceSessionString, null},                        
            {ContinueIfContainersFeatureNotInstalledString, null},
            {SkipDeleteFabricDataRootString, null},
        };
    }
}
