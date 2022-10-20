// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Common;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{

    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    ///  Describe a Partitions Id and Service Uri
    /// </summary>
    [DataContract]
    public class BackupSuspendedPartitionInfo 
    {
        [DataMember]
        public string ServiceUri { set; get; }

        [DataMember]
        public string PartitionId { set; get; }

        internal BackupSuspendedPartitionInfo(string fabricUriKey)
        {
            string applicationNameUri = null;
            string serviceNameUri = null;
            string partitionId = null;

            UtilityHelper.GetApplicationAndServicePartitionUri(fabricUriKey, out applicationNameUri, out serviceNameUri,out partitionId);
            this.ServiceUri = serviceNameUri;
            this.PartitionId = partitionId;
        }

        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ServiceUri={0}", this.ServiceUri));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PartitionId={0}", this.PartitionId));
            return sb.ToString();
        }
    }
}