// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Fabric.BackupRestore.Enums;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;


    /// <summary>
    ///  Represents Backup Info List
    /// </summary>
    [DataContract]
    public class PagedBackupInfoList  
    {
        /// <summary>
        /// List of Backups
        /// </summary>
        [DataMember]
        public List<RestorePoint> Items { get; set; }

        /// <summary>
        /// Continuation token
        /// </summary>
        [DataMember]
        public string ContinuationToken { get; set; }
        
        /// <summary>
        /// Override ToString
        /// </summary>
        /// <returns>String representation of RecoveryPoint object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Items Count={0}", this.Items?.Count));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ContinuationToken={0}", this.ContinuationToken));
            return sb.ToString();
        }
    }
}