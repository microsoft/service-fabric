// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.Collections.Generic;

    /// <summary>
    /// Describes the Backup Entity List of Applied Backup Policies 
    /// </summary>
    [DataContract]
    public class PagedBackupEntityList
    {
        /// <summary>
        /// Creating default Backup Entity
        /// </summary>
        public PagedBackupEntityList()
        {
            this.Items = new List<BackupEntity>();
        }

        /// <summary>
        /// List of Backup Entity having periodic backup  
        /// </summary>
        [DataMember]
        public List<BackupEntity> Items { set; get; }

        /// <summary>
        ///  Continuation Token
        /// </summary>
        [DataMember]
        public string ContinuationToken {set ; get ;}
    }

    
}