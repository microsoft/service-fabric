// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric.Health;
    using System.Management.Automation;

    /// <summary>
    /// This class will implement "Send-ClusterHealthReport" cmdlet.
    /// Required parameters: HeathStatus, SourceId, Property,
    /// Optional parameters: Description, TimeToLive, RemoveWhenExpired
    /// </summary>
    [Cmdlet(VerbsCommunications.Send, "ServiceFabricClusterHealthReport")]
    public sealed class SendClusterHealthReport : SendHealthReport
    {
        /// errorId for the executing cmdlet. Any exception will be reported with this errorId
        protected override string SendHealthReportErrorId
        {
            get
            {
                return Constants.SendClusterHealthReportErrorId;
            }
        }

        protected override HealthReport GetHealthReport(HealthInformation healthInformation)
        {
            return new ClusterHealthReport(healthInformation);
        }
    }
}