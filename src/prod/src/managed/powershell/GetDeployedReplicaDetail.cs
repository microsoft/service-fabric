// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricDeployedReplicaDetail")]
    public sealed class GetDeployedReplicaDetail : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 2)]
        public long ReplicaOrInstanceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 3)]
        public SwitchParameter ReplicatorDetail
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetDeployedReplicaDetailAsync(
                                      this.NodeName,
                                      this.PartitionId,
                                      this.ReplicaOrInstanceId,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(queryResult));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetDeployedReplicaDetailErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            if (output is DeployedServiceReplicaDetail)
            {
                var item = output as DeployedServiceReplicaDetail;

                var itemPSObj = new PSObject(item);

                var reportedLoadObject = new PSObject(item.ReportedLoad);
                reportedLoadObject.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ReportedLoadPropertyName,
                        reportedLoadObject));

                if (item is DeployedStatefulServiceReplicaDetail)
                {
                    var inner = item as DeployedStatefulServiceReplicaDetail;

                    if (inner.ReplicatorStatus != null)
                    {
                        var replicatorStatus = inner.ReplicatorStatus;

                        if (replicatorStatus is PrimaryReplicatorStatus)
                        {
                            var innerReplicatorStatus = replicatorStatus as PrimaryReplicatorStatus;

                            if (!this.ReplicatorDetail)
                            {
                                var basicInfo = new PSObject(innerReplicatorStatus.ReplicationQueueStatus);
                                basicInfo.Members.Add(
                                    new PSCodeMethod(
                                        Constants.ToStringMethodName,
                                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                                itemPSObj.Properties.Add(
                                    new PSNoteProperty(
                                        Constants.ReplicatorStatusPropertyName,
                                        basicInfo));
                            }
                            else
                            {
                                var detailInfo = new PSObject(innerReplicatorStatus);
                                detailInfo.Members.Add(
                                    new PSCodeMethod(
                                        Constants.ToStringMethodName,
                                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                                itemPSObj.Properties.Add(
                                    new PSNoteProperty(
                                        Constants.ReplicatorStatusPropertyName,
                                        detailInfo));
                            }
                        }
                        else if (replicatorStatus is SecondaryReplicatorStatus)
                        {
                            var innerReplicatorStatus = replicatorStatus as SecondaryReplicatorStatus;

                            if (!this.ReplicatorDetail)
                            {
                                var basicInfo = new PSObject(innerReplicatorStatus.ReplicationQueueStatus);
                                basicInfo.Members.Add(
                                    new PSCodeMethod(
                                        Constants.ToStringMethodName,
                                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                                itemPSObj.Properties.Add(
                                    new PSNoteProperty(
                                        Constants.ReplicatorStatusPropertyName,
                                        basicInfo));
                            }
                            else
                            {
                                var detailInfo = new PSObject(innerReplicatorStatus);
                                detailInfo.Members.Add(
                                    new PSCodeMethod(
                                        Constants.ToStringMethodName,
                                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                                itemPSObj.Properties.Add(
                                    new PSNoteProperty(
                                        Constants.ReplicatorStatusPropertyName,
                                        detailInfo));
                            }
                        }
                        else
                        {
                            // Unknown format. Neglect result and return
                            return itemPSObj;
                        }
                    } // ReplicatorStatus

                    if (inner.ReplicaStatus != null)
                    {
                        var kvsStatus = inner.ReplicaStatus as KeyValueStoreReplicaStatus;
                        if (kvsStatus != null)
                        {
                            var kvsStatusObject = new PSObject(kvsStatus);
                            kvsStatusObject.Members.Add(
                                new PSCodeMethod(
                                    Constants.ToStringMethodName,
                                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                            itemPSObj.Properties.Add(
                                new PSNoteProperty(
                                    Constants.ReplicaStatus,
                                    kvsStatusObject));
                        }
                    } // ReplicaStatus

                    var deployedStatefulServiceReplicaDetail = item as DeployedStatefulServiceReplicaDetail;

                    if (deployedStatefulServiceReplicaDetail.DeployedServiceReplica != null)
                    {
                        FormatDeployedStatefulServiceReplica(deployedStatefulServiceReplicaDetail.DeployedServiceReplica, itemPSObj);

                        var deployedStatefulServiceReplica = deployedStatefulServiceReplicaDetail.DeployedServiceReplica as DeployedStatefulServiceReplica;
                        FormatReconfigurationInformation(deployedStatefulServiceReplica.ReconfigurationInformation, itemPSObj);
                    }
                }
                else
                {
                    var deployedStatelessServiceInstanceDetail = item as DeployedStatelessServiceInstanceDetail;
                    if (deployedStatelessServiceInstanceDetail.DeployedServiceReplicaInstance != null)
                    {
                        FormatDeployedStatelessServiceInstance(deployedStatelessServiceInstanceDetail.DeployedServiceReplicaInstance, itemPSObj);
                    }
                }

                return itemPSObj;
            }
            else
            {
                return base.FormatOutput(output);
            }
        }

        private static void FormatDeployedStatelessServiceInstance(DeployedServiceReplica deployedServiceReplicaInstance, PSObject deployedStatelessServiceIntanceDetailPSObj)
        {
            var deployedServiceReplicaInstancePSObj = new PSObject(deployedServiceReplicaInstance);

            deployedServiceReplicaInstancePSObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            deployedStatelessServiceIntanceDetailPSObj.Properties.Add(
                new PSNoteProperty(
                    Constants.DeployedServiceReplicaInstance,
                    deployedServiceReplicaInstancePSObj));
        }

        private static void FormatDeployedStatefulServiceReplica(DeployedServiceReplica deployedServiceReplica, PSObject deployedServiceReplicaDetailPSObj)
        {
            var deployedServiceReplicaPSObj = new PSObject(deployedServiceReplica);

            deployedServiceReplicaPSObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            deployedServiceReplicaDetailPSObj.Properties.Add(
                new PSNoteProperty(
                    Constants.DeployedServiceReplica,
                    deployedServiceReplicaPSObj));
        }

        private static void FormatReconfigurationInformation(System.Fabric.ReconfigurationInformation reconfigurationInformation, PSObject result)
        {
            var reconfigurationInformationObj = new PSObject(reconfigurationInformation);
            reconfigurationInformationObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
            result.Properties.Add(
                new PSNoteProperty(
                    Constants.ReconfigurationInformation,
                    reconfigurationInformationObj));
        }
    }
}
