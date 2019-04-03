// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricTestCommandStatusList")]
    public sealed class GetTestCommandStatusList : CommonCmdletBase
    {
        [Parameter(Mandatory = false)]
        public TestCommandStateFilter StateFilter
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public TestCommandTypeFilter TypeFilter
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var testCommandQueryList = clusterConnection.GetTestCommandStatusListAsync(
                                               this.StateFilter == TestCommandStateFilter.Default ? TestCommandStateFilter.All : this.StateFilter,
                                               this.TypeFilter  == TestCommandTypeFilter.Default ? TestCommandTypeFilter.All : this.TypeFilter,
                                               this.GetTimeout(),
                                               this.GetCancellationToken()).GetAwaiter().GetResult();

                foreach (var item in testCommandQueryList)
                {
                    this.WriteObject(this.FormatOutput(item));
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetTestCommandStatusListCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}