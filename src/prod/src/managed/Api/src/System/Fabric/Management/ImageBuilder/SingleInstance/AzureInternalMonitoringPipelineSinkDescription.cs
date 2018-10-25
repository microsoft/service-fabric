// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class AzureInternalMonitoringPipelineSinkDescription : DiagnosticsSinkBase
    {
        public AzureInternalMonitoringPipelineSinkDescription()
            : base()
        {
            this.accountName = null;
            this.@namespace = null;
            this.maConfigUrl = null;
            this.fluentdConfigUrl = null;
            this.autoKeyConfigUrl = null;
        }

        public string accountName;
        public string @namespace;
        public string maConfigUrl;
        public string fluentdConfigUrl;
        public string autoKeyConfigUrl;
    }
}
