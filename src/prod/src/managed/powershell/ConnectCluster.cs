// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Strings;
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsCommunications.Connect, "ServiceFabricCluster")]
    public sealed class ConnectCluster : ClusterCmdletBase
    {
        public const double HealthReportSendIntervalSecDefault = 0.0;

        [Parameter(Mandatory = false, Position = 0, ParameterSetName = "Default")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Windows")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "X509")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Dsts")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Aad")]
        [ValidateNotNullOrEmpty]
        public string[] ConnectionEndpoint
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter AllowNetworkConnectionOnly
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Windows")]
        public SwitchParameter WindowsCredential
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Windows")]
        public string ClusterSpn
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "X509")]
        public SwitchParameter X509Credential
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "X509")]
        [Parameter(Mandatory = false, ParameterSetName = "Dsts")]
        [Parameter(Mandatory = false, ParameterSetName = "Aad")]
        public string[] ServerCommonName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "X509")]
        [Parameter(Mandatory = false, ParameterSetName = "Dsts")]
        [Parameter(Mandatory = false, ParameterSetName = "Aad")]
        public string[] ServerCertThumbprint
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "X509")]
        public X509FindType FindType
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "X509")]
        public string FindValue
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "X509")]
        public StoreLocation StoreLocation
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "X509")]
        public string StoreName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Dsts")]
        public SwitchParameter DSTS
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Dsts")]
        public string MetaDataEndpoint
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Dsts")]
        public string CloudServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Dsts")]
        public string[] CloudServiceDNSNames
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Aad")]
        public SwitchParameter AzureActiveDirectory
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Aad")]
        public string SecurityToken
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Aad")]
        public SwitchParameter GetMetadata
        {
            get;
            set;
        }

        // FabricClientSettings.ConnectionInitializationTimeout
        [Parameter(Mandatory = false)]
        [ValidateRange(0, Double.MaxValue)]
        public double? ConnectionInitializationTimeoutInSec
        {
            get;
            set;
        }

        // FabricClientSettings.HealthOperationTimeout
        [Parameter(Mandatory = false)]
        [ValidateRange(0, Double.MaxValue)]
        public double? HealthOperationTimeoutInSec
        {
            get;
            set;
        }

        // FabricClientSettings.HealthReportSendInterval
        // Adding HealthReportSendInterval to let the user decide when or whether HealthReports are batched.
        [Parameter(Mandatory = false,
                   HelpMessage =
                       "Non-negative. Default is 0, in which case HealthReport will not be batched and rather sent immediately.")]
        [ValidateRange(0, Double.MaxValue)]
        public double? HealthReportSendIntervalInSec
        {
            get;
            set;
        }

        // FabricClientSettings.HealthReportRetrySendInterval
        [Parameter(Mandatory = false)]
        [ValidateRange(0, Double.MaxValue)]
        public double? HealthReportRetrySendIntervalInSec
        {
            get;
            set;
        }

        // FabricClientSettings.KeepAliveInterval
        [Parameter(Mandatory = false)]
        [ValidateRange(0, Double.MaxValue)]
        public double? KeepAliveIntervalInSec
        {
            get;
            set;
        }

        // FabricClientSettings.ServiceChangePollInterval
        [Parameter(Mandatory = false)]
        [ValidateRange(0, Double.MaxValue)]
        public double? ServiceChangePollIntervalInSec
        {
            get;
            set;
        }

        // FabricClientSettings.PartitionLocationCacheLimit
        [Parameter(Mandatory = false)]
        [ValidateRange(0, long.MaxValue)]
        public long? PartitionLocationCacheLimit
        {
            get;
            set;
        }

        // FabricClientSettings.AuthTokenBufferSize
        [Parameter(Mandatory = false)]
        [ValidateRange(0, long.MaxValue)]
        public long? AuthTokenBufferSize
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Dsts")]
        public bool? Interactive
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public bool? SkipChecks
        {
            get;
            set;
        }       

        protected override void BeginProcessing()
        {
            if (this.ServerCommonName == null && (this.X509Credential || this.DSTS || this.AzureActiveDirectory))
            {
                this.ServerCommonName = new string[] { };

                System.Collections.Generic.List<string> serverCommonNameList = new System.Collections.Generic.List<string>(this.ServerCommonName);
                foreach (string endPoint in this.ConnectionEndpoint)
                {
                    System.Uri uri;
                    if (Uri.TryCreate(string.Format("http://{0}", endPoint), UriKind.Absolute, out uri) && uri.HostNameType == UriHostNameType.Dns)
                    {
                        if (Array.Find(this.ServerCommonName, n => n.Equals(uri.Host, StringComparison.CurrentCultureIgnoreCase)) == null)
                        {
                            serverCommonNameList.Add(uri.Host);
                        }
                    }
                }

                this.ServerCommonName = serverCommonNameList.ToArray();

                if (this.ServerCommonName.Length == 0 && this.ServerCertThumbprint == null)
                {
                    this.ThrowTerminatingError(new ErrorRecord(
                                              new PSArgumentException(
                                                  StringResources.PowerShell_ConnectClusterWithoutServerCommonNameAndServerCertThumbprintError),
                                              Constants.CreateClusterConnectionErrorId,
                                              ErrorCategory.InvalidArgument,
                                              this.ServerCommonName));
                }
            }
        }

        protected override void ProcessRecord()
        {
            try
            {
                ClusterConnection.SecurityCredentialsBuilder securityCredentialsBuilder = null;

                if (this.WindowsCredential)
                {
                    securityCredentialsBuilder = new ClusterConnection.WindowsSecurityCredentialsBuilder(
                        this.ClusterSpn);
                }
                else if (this.X509Credential)
                {
                    securityCredentialsBuilder =
                        new ClusterConnection.X509SecurityCredentialsBuilder(
                        this.ServerCertThumbprint, this.ServerCommonName, this.FindType, this.FindValue, this.StoreLocation, this.StoreName);
                }
                else if (this.DSTS)
                {
                    bool interactive = true;
                    if (this.Interactive.HasValue)
                    {
                        interactive = this.Interactive.GetValueOrDefault();
                    }

                    securityCredentialsBuilder =
                        new ClusterConnection.DSTSCredentialsBuilder(
                        this.ServerCommonName,
                        this.ServerCertThumbprint,
                        this.MetaDataEndpoint,
                        interactive,
                        this.CloudServiceName,
                        this.CloudServiceDNSNames);
                }
                else if (this.AzureActiveDirectory)
                {
                    securityCredentialsBuilder =
                        new ClusterConnection.AadCredentialsBuilder(
                        this.ServerCommonName,
                        this.ServerCertThumbprint,
                        this.SecurityToken);
                }

                // We need to provide a custom FabricClientSettingsBuilder,
                // because we want to override HealthReportSendInterval with zero or user's input value.
                ClusterConnection.FabricClientSettingsBuilder fabricClientSettingsBuilder =
                    this.CreateFabricClientSettingsBuilder();

                IClusterConnection clusterConnection = new ClusterConnection(
                    new ClusterConnection.FabricClientBuilder(
                        securityCredentialsBuilder,
                        fabricClientSettingsBuilder,
                        this.ConnectionEndpoint),
                    this.GetMetadata);

                try
                {
                    if (this.GetMetadata)
                    {
                        clusterConnection.InitializeClaimsMetadata(this.GetTimeout());
                    }
                    else if (this.SkipChecks.GetValueOrDefault() == false)
                    {
                        this.TestClusterConnection(
                            clusterConnection,
                            this.AllowNetworkConnectionOnly);
                    }
                }
                catch (Exception e)
                {
                    this.ThrowTerminatingError(
                        e,
                        Constants.CreateClusterConnectionErrorId,
                        null);
                }

                this.SetClusterConnection(clusterConnection);
                this.WriteObject(this.FormatOutput(clusterConnection));
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.CreateClusterConnectionErrorId,
                    null);
            }
        }

        // Returns a new FabricClientSettingsBuilder object using the properties of this class.
        private ClusterConnection.FabricClientSettingsBuilder CreateFabricClientSettingsBuilder()
        {
            ClusterConnection.FabricClientSettingsBuilder fabricClientSettingsBuilder =

                // Passing parameter values as it is. When any parameter is null it will ignored downstream.
                new ClusterConnection.FabricClientSettingsBuilder(
                this.ConnectionInitializationTimeoutInSec,
                this.HealthOperationTimeoutInSec,
                //// This cmdlet will set interval to default value(0) if not provided by user.
                this.HealthReportSendIntervalInSec ?? HealthReportSendIntervalSecDefault,
                this.KeepAliveIntervalInSec,
                this.ServiceChangePollIntervalInSec,
                this.PartitionLocationCacheLimit,
                this.HealthReportRetrySendIntervalInSec,
                this.AuthTokenBufferSize);

            return fabricClientSettingsBuilder;
        }
    }
}
