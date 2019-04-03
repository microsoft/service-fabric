// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;

    /// <summary>
    /// Failure in fetching Restore Points
    /// </summary>
    [DataContract]
    public class RestorePointEnumerationFailureInfo
    {
        /// <summary>
        /// Details of the Error
        /// </summary>
        [DataMember]
        public FabricError Error { set; get; }
    }
}