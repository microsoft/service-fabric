// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.Text;
    using System.Collections.Generic;

    /// <summary>
    ///  Paged Backup Policy List
    /// </summary>
    [DataContract]
    public class PagedBackupPolicyDescriptionList
    {
        internal PagedBackupPolicyDescriptionList()
        {
            this.Items = new List<BackupPolicy>();
        }    
        /// <summary>
        ///  Paged Backup Policy List
        /// </summary>
        [DataMember]
        public List<BackupPolicy> Items { set; get; }

        /// <summary>
        ///  ContinuationToken
        /// </summary>
        [DataMember]
        public string ContinuationToken { set ; get ;}

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Items : {0}", this.Items).AppendLine();
            stringBuilder.AppendFormat("ContinuationToken : {0}", this.ContinuationToken).AppendLine();
            return stringBuilder.ToString();
        }

    }
}