// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Text;
    using System.Collections.Generic;
    using System.Runtime.Serialization;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;

    [DataContract]
    internal abstract class BackupPolicyWorkItem : WorkItem
    {
        [DataMember]
        internal WorkItemInfo WorkItemInfo;

        internal BackupPolicyWorkItem(WorkItemInfo workItemInfo) 
        {
            this.WorkItemInfo = workItemInfo;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendLine(base.ToString());
            stringBuilder.AppendFormat("WorkItemInfo {0}",this?.WorkItemInfo).AppendLine();
            return stringBuilder.ToString();
        }
    }
}