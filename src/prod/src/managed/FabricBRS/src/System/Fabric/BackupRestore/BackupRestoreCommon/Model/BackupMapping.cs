// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Text;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;

    /// <summary>
    /// Describes the Short Retention Policy of the Backups
    /// </summary>
    
    [DataContract]
    internal class BackupMapping
    {
        private BackupMapping()
        {
            this.ProtectionId = Guid.NewGuid();
        }

        [DataMember]
        internal Guid ProtectionId { get; private set; }

        [DataMember]
        internal string ApplicationOrServiceUri { get; private set; }

        [DataMember]
        internal string BackupPolicyName { get; private set; }

        internal static BackupMapping FromBackupMappingView(BackupRestoreView.BackupMapping backupProtectionView, string applicationOrServiceUri)
        {
            BackupMapping backupMapping = new BackupMapping
            {
                BackupPolicyName = backupProtectionView.BackupPolicyName,
                ApplicationOrServiceUri = applicationOrServiceUri,
            };

            return backupMapping;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("ApplicationOrServiceURI : {0}", this.ApplicationOrServiceUri).AppendLine();
            stringBuilder.AppendFormat("BackupPolicyName : {0} ", this.BackupPolicyName).AppendLine();
            return stringBuilder.ToString();
        }
    }
}