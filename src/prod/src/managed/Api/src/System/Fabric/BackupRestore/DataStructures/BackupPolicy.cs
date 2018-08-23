// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Runtime.Serialization;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Backup schedule policy
    /// </summary>
    [Serializable]
    [KnownType(typeof(FrequencyBasedBackupPolicy))]
    [KnownType(typeof(ScheduleBasedBackupPolicy))]
    internal abstract class BackupPolicy
    {
        /// <summary>
        /// Name of the policy
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Unique Policy Identifier
        /// </summary>
        public Guid PolicyId { get; set; }

        /// <summary>
        /// Indicates the types of backup scheduling policy
        /// </summary>
        public BackupPolicyType PolicyType { get; set; }

        /// <summary>
        /// Maximum number of incremental backups that can be taken before taking a full backup again
        /// </summary>
        public byte MaxIncrementalBackups { get; set; }

        /// <summary>
        /// Backup store details
        /// </summary>
        public BackupStoreInformation StoreInformation { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="policyType"></param>
        protected BackupPolicy(BackupPolicyType policyType)
        {
            this.PolicyType = policyType;
        }

        /// <summary>
        /// Gets a string representation of the backup policy object
        /// </summary>
        /// <returns>A string represenation of the backup policy object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Name={0}", this.Name));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PolicyId={0}", this.PolicyId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PolicyType={0}", this.PolicyType));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "MaxIncrementalBackups={0}", this.MaxIncrementalBackups));
            
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "StoreInformation:"));
            sb.AppendLine(StoreInformation.ToString());
            
            return sb.ToString();
        }

        #region Interop helpers

        internal abstract IntPtr ToNative(PinCollection pinCollection);

        internal static unsafe BackupPolicy FromNative(IntPtr pointer)
        {
            var nativeBackupPolicy = *(NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY*) pointer;
            if (nativeBackupPolicy.PolicyType ==
                NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_TYPE.FABRIC_BACKUP_POLICY_TYPE_FREQUENCY_BASED)
            {
                return FrequencyBasedBackupPolicy.FromNative(nativeBackupPolicy);
            }
            else if (nativeBackupPolicy.PolicyType ==
                     NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_TYPE.FABRIC_BACKUP_POLICY_TYPE_SCHEDULE_BASED)
            {
                return ScheduleBasedBackupPolicy.FromNative(nativeBackupPolicy);
            }

            return null;
        }

        protected void PopulateBackupPolicyFromNative(NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY backupPolicy)
        {
            Name = NativeTypes.FromNativeString(backupPolicy.Name);
            PolicyId = backupPolicy.PolicyId;
            MaxIncrementalBackups = backupPolicy.MaxIncrementalBackups;
            StoreInformation = BackupStoreInformation.FromNative(backupPolicy.StoreInformation);
        }

        protected NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY GetNativeBackupPolicy(PinCollection pinCollection)
        {
            var nativePolicy = new NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY
            {
                Name = pinCollection.AddObject(Name),
                PolicyId = PolicyId,
                PolicyType = (NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_TYPE) PolicyType,
                MaxIncrementalBackups = MaxIncrementalBackups,
                StoreInformation = StoreInformation.ToNative(pinCollection),
            };

            return nativePolicy;
        }

        #endregion
    }
}