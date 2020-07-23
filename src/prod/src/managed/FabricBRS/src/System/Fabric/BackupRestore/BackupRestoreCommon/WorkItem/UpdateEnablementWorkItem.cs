// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;

namespace System.Fabric.BackupRestore.Common
{
    using System.Text;
    using System.Collections.Generic;
    using System.Runtime.Serialization;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;
    using System.Linq;

    [DataContract]
    internal  class UpdateEnablementWorkItem : BackupPolicyWorkItem
    {
        [DataMember (Name = "ListOfEnablement")]
        private HashSet<string> listOfEnablement;

        internal IReadOnlyCollection<string> ListOfEnablement
        {
            get { return listOfEnablement.ToList(); } 
        }

        private const string TraceTypeConst = "UpdateEnablementWorkItem";

        internal UpdateEnablementWorkItem(List<string> listofEnablement, WorkItemInfo workItemInfo) 
            : base(workItemInfo )
        {
            this.listOfEnablement = new HashSet<string>(listofEnablement);
        }

        public override async Task<bool> Process(StatefulService statefulService,TimeSpan timeout , CancellationToken cancellationToken , string processQueueTypeTrace)
        {
            WorkItemQueue workItemQueue =  await WorkItemQueue.CreateOrGetWorkItemQueue(statefulService);
            List<WorkItem> workItemList = new List<WorkItem>();
            WorkItem workItem = null;
            foreach (string key in this.listOfEnablement)
            {
                string applicationNameUri = null;
                string serviceNameUri = null;
                string partitionId = null;
                FabricBackupResourceType fabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionUri(
                    key, out applicationNameUri, out serviceNameUri, out partitionId);
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Resolving for Key : {0} as {1}", key,fabricBackupResourceType);
                switch (fabricBackupResourceType)
                {
                        case FabricBackupResourceType.PartitionUri:
                        workItem = new SendToServiceNodeWorkItem(serviceNameUri, partitionId,this.WorkItemInfo);
                        break;
                        
                        case FabricBackupResourceType.ServiceUri:
                        workItem = new ResolveToPartitionWorkItem(FabricBackupResourceType.ServiceUri, serviceNameUri,
                            this.WorkItemInfo);
                        break;

                        case FabricBackupResourceType.ApplicationUri:
                        workItem = new ResolveToPartitionWorkItem(FabricBackupResourceType.ApplicationUri, applicationNameUri,
                            this.WorkItemInfo);
                        break;
                }
                if (workItem != null)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Added WorkItem of Type {0} ",workItem.GetType());
                    workItemList.Add(workItem);
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace, "Work Item is Null for ApplicationUri = {0} , " +
                                                                           "ServiceUri = {1} , PartitionUri = {2}",applicationNameUri,
                                                                           serviceNameUri,partitionId);
                }
            }
            foreach (var workItemInList in workItemList)
            {
                await workItemQueue.AddWorkItem(workItemInList,timeout,cancellationToken);
            }
            return true;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("ListOfEnablement : {0}", string.Join(",",  this.listOfEnablement)).AppendLine();
            stringBuilder.AppendLine(base.ToString());
            return stringBuilder.ToString();
        }
    }

}