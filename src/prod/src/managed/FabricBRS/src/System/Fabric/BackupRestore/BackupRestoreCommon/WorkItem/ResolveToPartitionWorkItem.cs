// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;
    using Threading.Tasks;

    [DataContract]
    internal class ResolveToPartitionWorkItem : BackupPolicyWorkItem
    {
        [DataMember]
        internal FabricBackupResourceType FabricBackupResourceType { get; private set; }

        [DataMember]
        internal string ApplicationOrServiceUri { get; private set; }


        internal ResolveToPartitionWorkItem(FabricBackupResourceType fabricBackupResourceType, string applicationOrServiceUri, WorkItemInfo workItemInfo) 
            :base(workItemInfo)
        {
            this.FabricBackupResourceType = fabricBackupResourceType;
            this.ApplicationOrServiceUri = applicationOrServiceUri;
            
        }

        public override async Task<bool> Process(Microsoft.ServiceFabric.Services.Runtime.StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken , string processQueueTypeTrace)
        {
            WorkItem workItem = null;
            WorkItemQueue workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(statefulService);
            BackupMappingStore backupMappingStore =
                await BackupMappingStore.CreateOrGetBackupMappingStore(statefulService);
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Resolving for {0} of type {1} ",this.ApplicationOrServiceUri,this.FabricBackupResourceType);
            try
            {
                if (this.FabricBackupResourceType == FabricBackupResourceType.ApplicationUri)
                {
                    ServiceList serviceList =
                        await FabricClientHelper.GetServiceList(this.ApplicationOrServiceUri);
                    foreach (Query.Service service in serviceList)
                    {
                        if (service.ServiceKind == ServiceKind.Stateful && (service as StatefulService).HasPersistedState)
                        {
                            BackupMapping specificServiceBackupMapping = await backupMappingStore.GetValueAsync(
                                UtilityHelper.GetBackupMappingKey(service.ServiceName.OriginalString, null));

                            if (specificServiceBackupMapping == null || this.WorkItemInfo.WorkItemType == WorkItemPropogationType.SuspendPartition
                                        || this.WorkItemInfo.WorkItemType == WorkItemPropogationType.ResumePartition)
                            {
                                ServicePartitionList servicePartitionList =
                                    await
                                        FabricClientHelper.GetPartitionList(service.ServiceName.OriginalString);
                                foreach (Partition servicePartition in servicePartitionList)
                                {
                                    BackupMapping specificPartitionBackupMapping =
                                        await backupMappingStore.GetValueAsync(UtilityHelper.GetBackupMappingKey(
                                            service.ServiceName.OriginalString,
                                            servicePartition.PartitionInformation.Id.ToString()));
                                    if (specificPartitionBackupMapping == null || this.WorkItemInfo.WorkItemType == WorkItemPropogationType.SuspendPartition
                                        || this.WorkItemInfo.WorkItemType == WorkItemPropogationType.ResumePartition)
                                    {
                                        //Send to Service Partition
                                        workItem = new SendToServiceNodeWorkItem
                                            ( await UtilityHelper.GetCustomServiceUri(service.ServiceName.OriginalString,timeout,cancellationToken),
                                                servicePartition.PartitionInformation.Id.ToString(),
                                                this.WorkItemInfo);
                                        await workItemQueue.AddWorkItem(workItem,timeout,cancellationToken);
                                    }
                                }
                            }
                        }
                    }
                }
                else if (this.FabricBackupResourceType == FabricBackupResourceType.ServiceUri)
                {
                    ServicePartitionList servicePartitionList =
                                 await
                                     FabricClientHelper.GetPartitionList(this.ApplicationOrServiceUri);
                    foreach (Partition servicePartition in servicePartitionList)
                    {
                        BackupMapping specificPartitionBackupMapping =
                            await backupMappingStore.GetValueAsync(UtilityHelper.GetBackupMappingKey(
                                this.ApplicationOrServiceUri, servicePartition.PartitionInformation.Id.ToString()));
                        if (specificPartitionBackupMapping == null || this.WorkItemInfo.WorkItemType == WorkItemPropogationType.SuspendPartition
                                        || this.WorkItemInfo.WorkItemType == WorkItemPropogationType.ResumePartition)
                        {
                            //Send to Service Partition
                            workItem = new SendToServiceNodeWorkItem
                                (await UtilityHelper.GetCustomServiceUri(this.ApplicationOrServiceUri, timeout, cancellationToken), servicePartition.PartitionInformation.Id.ToString(),this.WorkItemInfo);
                            await workItemQueue.AddWorkItem(workItem, timeout, cancellationToken);
                        }
                    }
                }
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Resolving successful");
            }
            catch (Exception exception)
            {
                AggregateException aggregateException = exception as AggregateException;
                if (aggregateException != null)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace, "Aggregate Exception Stack Trace : {0}",
                        exception.StackTrace);
                    foreach (Exception innerException in aggregateException.InnerExceptions)
                    {
                        BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace,
                            "Inner Exception : {0} , Message : {1} , Stack Trace : {2} ",
                            innerException.InnerException, innerException.Message, innerException.StackTrace);

                    }
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(processQueueTypeTrace,
                            "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                            exception.InnerException, exception.Message, exception.StackTrace);
                }
                return false;
            }
            return true;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("ApplicationOrServiceUri : {0}",this.ApplicationOrServiceUri).AppendLine();
            stringBuilder.AppendFormat("FabricBackupResourceType {0}", this.FabricBackupResourceType).AppendLine();
            stringBuilder.AppendLine(base.ToString());
            return stringBuilder.ToString();
        }
    }

}