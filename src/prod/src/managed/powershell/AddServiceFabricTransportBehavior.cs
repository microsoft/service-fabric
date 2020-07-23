// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Common;
using System.Management.Automation;
using Microsoft.ServiceFabric.Powershell;

namespace Microsoft.ServiceFabric.PowerTools
{
    [Cmdlet(VerbsCommon.Add, "ServiceFabricTransportBehavior", SupportsShouldProcess = true)]
    public sealed class AddServiceFabricTransportBehavior : Microsoft.ServiceFabric.Powershell.CommonCmdletBase
    {
        private string destination = "*";
        private string probabilityToApply = "1.0";
        private string priority = "0";
        private string applyCount = "-1";

        [Parameter(Mandatory = true)]
        public string BehaviorName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string Destination
        {
            get
            {
                return destination;
            }


            set
            {
                destination = value;
            }
        }

        [Parameter(Mandatory = false)]
        public string ProbabilityToApply
        {
            get
            {
                return probabilityToApply;
            }


            set
            {
                probabilityToApply = value;
            }
        }

        [Parameter(Mandatory = false)]
        public string DelayInSeconds
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string DelaySpanInSeconds
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string Priority
        {
            get
            {
                return priority;
            }


            set
            {
                priority = value;
            }
        }

        [Parameter(Mandatory = false)]
        public string ApplyCount
        {
            get
            {
                return applyCount;
            }


            set
            {
                applyCount = value;
            }
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        void ValidateParams()
        {
            if (string.IsNullOrEmpty(NodeName) ||
                string.IsNullOrEmpty(Destination) ||
                int.Parse(Priority) < 0 ||
                double.Parse(ProbabilityToApply) < 0)
            {
                throw new ArgumentException("Invalid Argument Exception. \n" +
                                            "Expected Source != null/empty, Destination != null/empty, Priority > 0 and ProbabilityToApply > 0");
            }
        }

        protected override void ProcessRecord()
        {
            this.WriteObject(string.Format(
                                 "Note:Adding transport behavior causes reliability issues on the cluster.\n" +
                                 "Note: The behavior {0} is applied on the node until it is removed with Remove-ServiceFabricTransportBehavior cmdlet.", BehaviorName));
            if (this.ShouldProcess(this.NodeName))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    try
                    {
                        if (this.GetClusterConnection() == null)
                        {
                            throw new Exception("Connection to Cluster is null, Connect to Cluster before invoking the cmdlet");
                        }

                        Microsoft.ServiceFabric.Powershell.InternalClusterConnection clusterConnection =
                            new Microsoft.ServiceFabric.Powershell.InternalClusterConnection(this.GetClusterConnection());
                        ValidateParams();

                        TimeSpan delayInSeconds = string.IsNullOrEmpty(DelayInSeconds) ? TimeSpan.MaxValue : TimeSpan.FromSeconds(int.Parse(DelayInSeconds));
                        TimeSpan delaySpanInSeconds = string.IsNullOrEmpty(DelaySpanInSeconds) ? TimeSpan.Zero : TimeSpan.FromSeconds(int.Parse(DelaySpanInSeconds));

                        UnreliableTransportBehavior behavior = new UnreliableTransportBehavior(
                            Destination, BehaviorName, double.Parse(ProbabilityToApply),
                            delayInSeconds, delaySpanInSeconds,
                            int.Parse(Priority),
                            int.Parse(ApplyCount));

                        clusterConnection.AddUnreliableTransportAsync(
                            NodeName,
                            BehaviorName,
                            behavior,
                            this.GetTimeout(),
                            this.GetCancellationToken()).Wait();

                        // localization of string not needed, since this cmdlet is an internal use.
                        this.WriteObject(string.Format(
                                             "Added Unreliable transport for behavior: {0}.\n" +
                                             "Note: The behavior is applied on the node until it is removed with Remove-UnreliableTransport cmdlet.", BehaviorName));
                    }
                    catch (AggregateException aggregateException)
                    {
                        aggregateException.Handle((ae) =>
                        {
                            this.ThrowTerminatingError(
                                ae,
                                ConstantsInternal.AddUnreliableTransportId,
                                this.GetClusterConnection());
                            return true;
                        });
                    }
                }
            }
        }
    }
}