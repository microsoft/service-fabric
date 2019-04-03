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
    class WorkItemInfo
    {
        [DataMember]
        protected internal WorkItemPropogationType WorkItemType { set; get; }

        [DataMember]
        protected internal Guid BackupPolicyUpdateGuid {set; get; }

        [DataMember]
        protected internal Guid ProctectionGuid { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("WorkItemPropogationType {0}", this.WorkItemType).AppendLine();
            stringBuilder.AppendFormat("BackupPolicyUpdateGuid {0}", this.BackupPolicyUpdateGuid).AppendLine();
            stringBuilder.AppendFormat("ProctectionUpdateGuid {0}", this.ProctectionGuid).AppendLine();
            return stringBuilder.ToString();
        }
    }

}