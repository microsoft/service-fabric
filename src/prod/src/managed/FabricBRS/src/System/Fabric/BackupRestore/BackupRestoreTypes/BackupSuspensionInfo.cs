// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{

    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using System.Text;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json;

    /// <summary>
    ///  Describe a Partitions Id and Service Uri
    /// </summary>
    [DataContract]
    public class BackupSuspensionInfo
    {
        /// <summary>
        ///  Describe a Backup Entity Suspension from Periodic Backup
        /// </summary>
        [DataMember]
        public bool IsSuspended { set; get; }

        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        public BackupEntityKind SuspensionInheritedFrom { set; get; }

        internal BackupSuspensionInfo()
        {
            this.IsSuspended = false;
            this.SuspensionInheritedFrom = BackupEntityKind.Invalid;
        }

        internal BackupSuspensionInfo(BackupEntityKind suspensionInheritedFrom, bool isSuspended)
        {
            this.IsSuspended = isSuspended;
            this.SuspensionInheritedFrom = suspensionInheritedFrom;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Is Suspended {0}",this?.IsSuspended).AppendLine();
            stringBuilder.AppendFormat("SuspensionInheritedFrom {0}", this?.SuspensionInheritedFrom).AppendLine();
            return stringBuilder.ToString();
        }
    }
}