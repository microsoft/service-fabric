// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;

    /// <summary>
    /// Represent an Details of a Backup
    /// </summary>
    [DataContract]
    public class RestorePointEnumerationResult
    {

        /// <summary>
        /// Details of Backup
        /// </summary>
        [DataMember]
        public List<RestorePoint> RestorePoints { set; get; }

        /// <summary>
        /// Failures while fetching restore points
        /// </summary>
        [DataMember]
        public List<RestorePointEnumerationFailureInfo> Failures { set; get; }

        internal RestorePointEnumerationResult()
        {
            this.RestorePoints = new List<RestorePoint>();
            this.Failures = new List<RestorePointEnumerationFailureInfo>();
        }
    }
}