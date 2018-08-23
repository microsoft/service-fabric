// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Text;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Testability;

    /// <summary>
    /// Settings that configure the replicator
    /// </summary>
    internal class TransactionalReplicatorSettings
    {
        public TransactionalReplicatorSettings()
        {
            this.PublicSettings = new ReliableStateManagerReplicatorSettings();
            this.Test_LoggingEngine = "ktl";
            this.LogWriteFaultInjectionParameters = new LogWriteFaultInjectionParameters();
        }
         
        public ReliableStateManagerReplicatorSettings PublicSettings { get; set; }

        public LogWriteFaultInjectionParameters LogWriteFaultInjectionParameters { get; set; }

        public bool EnableIncrementalBackupsAcrossReplicas { get; set; }

        public uint ProgressVectorMaxEntries { get; set; }

        public string Test_LoggingEngine { get; set; }

        public override string ToString()
        {
            var builder = new StringBuilder();
            builder.AppendFormat(Environment.NewLine);
            if (this.PublicSettings != null)
            {
                builder.Append(this.PublicSettings);
            }       

            Utility.Assert(this.Test_LoggingEngine != null, SR.Error_Test_LoggingEngine_Null);

            builder.AppendFormat("Test_LoggingEngine = {0}" + Environment.NewLine, this.Test_LoggingEngine);
            builder.AppendFormat("LogWriteFaultInjectionParameters = {0}", this.LogWriteFaultInjectionParameters);
            builder.AppendFormat("ProgressVectorMaxEntries = {0}", this.ProgressVectorMaxEntries);
            builder.AppendFormat("EnableIncrementalBackupsAcrossReplicas = {0}", this.EnableIncrementalBackupsAcrossReplicas);

            return builder.ToString();
        }
    }
}