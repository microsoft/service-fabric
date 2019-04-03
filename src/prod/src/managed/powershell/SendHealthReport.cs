// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    /// <summary>
    /// This class will be a common base class of any "Send-{some-entity}HealthReport" cmdlet class.
    /// Parameters defined in this class are corresponding to HealthInformation class which is part of any HealthReport.
    /// Required parameters: HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired, SequenceId.
    ///
    /// These are the health reports would be created by using this class
    /// ApplicationHealthReport;
    /// DeployedApplicationHealthReport;
    /// DeployedServicePackageHealthReport;
    /// InstanceHealthReport; [StatelessServiceInstanceHealthReport?]
    /// NodeHealthReport;
    /// PartitionHealthReport;
    /// ReplicaHealthReport; [StatefulServiceReplicaHealthReport?]
    /// ServiceHealthReport;
    /// <summary>
    public abstract class SendHealthReport : CommonCmdletBase
    {
        /// This input could be integer or string value (case insensitive). 1/Ok/ok, 2/Warning/warning etc.
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true,
                   HelpMessage = "Please use Ok(1), Warning(2), Error(3) etc. Hit tab key after parameter name (-HealthState) for details.")]
        public HealthState HealthState
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true)]
        public string SourceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true)]
        public string HealthProperty
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public string Description
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public int? TimeToLiveSec
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public SwitchParameter RemoveWhenExpired
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public long? SequenceNumber
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Immediate
        {
            get;
            set;
        }

        /// errorId for the executing cmdlet. Any exception will be reported with this errorId
        protected abstract string SendHealthReportErrorId
        {
            get;
        }

        internal void ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions)
        {
            var clusterConnection = this.GetClusterConnection();
            this.ReportHealth(healthReport, sendOptions, clusterConnection);
        }

        internal void ReportHealth(HealthReport healthReport, HealthReportSendOptions sendOptions, IClusterConnection clusterConnection)
        {
            try
            {
                /// Warn user if health report will not be sent immediately.
                this.WarnIfHealthReportBatchingOn(clusterConnection);

                /// Sending the healthReport received from derived class to cluster-health-manager service.
                clusterConnection.ReportHealth(healthReport, sendOptions);
            }
            catch (Exception anyException)
            {
                this.ThrowTerminatingError(
                    anyException,
                    this.SendHealthReportErrorId,
                    clusterConnection);
            }
        }

        internal void WarnIfHealthReportBatchingOn(IClusterConnection clusterConnection)
        {
            if (this.Immediate)
            {
                // The report is configured to be sent immediately, regardless of the client settings.
                return;
            }

            TimeSpan interval = clusterConnection.FabricClientSettings.HealthReportSendInterval;
            if (interval > TimeSpan.Zero)
            {
                this.WriteWarning(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Warn_SendHealthReportIntervalNotZero,
                        interval));
            }
        }

        protected HealthInformation GetHealthInformation()
        {
            HealthInformation healthInformation = new HealthInformation(this.SourceId, this.HealthProperty, this.HealthState);

            /// Setting healthInformation's optional values if provided in cmdlet arguments.
            if (this.Description != null)
            {
                healthInformation.Description = this.Description;
            }

            if (this.TimeToLiveSec.HasValue)
            {
                healthInformation.TimeToLive = TimeSpan.FromSeconds(this.TimeToLiveSec.Value);
            }

            healthInformation.RemoveWhenExpired = this.RemoveWhenExpired.IsPresent;

            if (this.SequenceNumber.HasValue)
            {
                healthInformation.SequenceNumber = this.SequenceNumber.Value;
            }

            return healthInformation;
        }

        protected override void ProcessRecord()
        {
            HealthReportSendOptions sendOptions = null;
            if (this.Immediate)
            {
                sendOptions = new HealthReportSendOptions() { Immediate = true };
            }

            this.ReportHealth(this.GetHealthReport(this.GetHealthInformation()), sendOptions);
        }

        protected abstract HealthReport GetHealthReport(HealthInformation healthInformation);
    }
}