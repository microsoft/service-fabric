// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;
    using System.Text;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;
    using System.Collections.Generic;
    using System.Linq;

    [DataContract]
    internal class BackupPolicy : IEquatable<BackupPolicy>
    {
        private BackupPolicy()
        {
            this.UniqueId = Guid.NewGuid();
        }

        /// <summary>
        /// Internal constructor
        /// </summary>
        /// <param name="other">Object to copy from</param>
        internal BackupPolicy(BackupPolicy other)
        {
            this.UniqueId = other.UniqueId;
            this.Name = other.Name;
            this.backupEnabledSet = new HashSet<string>(other.backupEnabledSet);
            this.AutoRestore = other.AutoRestore;
            this.MaxIncrementalBackup = other.MaxIncrementalBackup;

            this.Storage = other.Storage.DeepCopy();

            if (other.BackupSchedule.BackupScheduleType == BackupScheduleType.FrequencyBased)
            {
                this.BackupSchedule = new FrequencyBasedBackupSchedule((FrequencyBasedBackupSchedule)other.BackupSchedule);
            }
            else if (other.BackupSchedule.BackupScheduleType == BackupScheduleType.TimeBased)
            {
                this.BackupSchedule = new TimeBasedBackupSchedule((TimeBasedBackupSchedule)other.BackupSchedule);
            }
            if(other.RetentionPolicy != null)
            {
                this.RetentionPolicy = new BasicRetentionPolicy((BasicRetentionPolicy)other.RetentionPolicy);
            }
            else
            {
                this.RetentionPolicy = null;
            }
        }

        [DataMember]
        internal string Name { get; private set; } 

        [DataMember]
        internal BackupSchedule BackupSchedule { get; private set; }

        [DataMember]
        internal BackupStorage Storage { get; private set; }

        [DataMember]
        internal RetentionPolicy RetentionPolicy { get; private set; }

        [DataMember (Name = "BackupEnabledSet")]
        private HashSet<string> backupEnabledSet = new HashSet<string>();

        internal IReadOnlyCollection<string> BackupEnabledSet
        {
            get { return backupEnabledSet.ToList(); }
        }

        [DataMember]
        internal bool AutoRestore { get; private set; }

        [DataMember]
        internal Guid UniqueId { get; private set; }

        [DataMember]
        internal int MaxIncrementalBackup { get; private set; }

        internal static BackupPolicy FromBackupPolicyView(BackupRestoreView.BackupPolicy backupPolicyView)
        {
            backupPolicyView.ThrowIfNull("BackupPolicy");
            BackupPolicy backupPolicy = new BackupPolicy
            {
                Name = backupPolicyView.Name,
                AutoRestore = backupPolicyView.AutoRestoreOnDataLoss,
                MaxIncrementalBackup = backupPolicyView.MaxIncrementalBackups,
                BackupSchedule = BackupSchedule.FromBackupScheduleView(backupPolicyView.Schedule),
                Storage = BackupStorage.FromBackupStorageView(backupPolicyView.Storage),
                RetentionPolicy = RetentionPolicy.FromRetentionPolicyView(backupPolicyView.RetentionPolicy)
            };
            
            return backupPolicy;
        }

       internal BackupRestoreView.BackupPolicy ToBackupPolicyView()
        {
           BackupRestoreView.BackupPolicy backupPolicyView = new BackupRestoreView.BackupPolicy
           {
               Name = this.Name,
               AutoRestoreOnDataLoss = this.AutoRestore,
               Schedule = this.BackupSchedule.ToBackupScheduleView(),
               Storage = this.Storage.ToBackupStorageView(),
               MaxIncrementalBackups = this.MaxIncrementalBackup,
               RetentionPolicy = RetentionPolicy.ToRetentionPolicyView(this.RetentionPolicy)
           };
           
           return backupPolicyView;
        }

        /// <summary>
        /// Whether two objects are equal
        /// The assumption here is that whenever the object is updated due to user action, the unique ID is updated
        /// </summary>
        /// <param name="other">Object to check equality against</param>
        /// <returns>Whether the two objects are equal or not</returns>
        public bool Equals(BackupPolicy other)
        {
            if (null == other) return false;
            return this.UniqueId.Equals(other.UniqueId);
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("BackupPolicy Name : {0}", this.Name).AppendLine();
            stringBuilder.AppendFormat("Auto-Restore : {0}", this.AutoRestore).AppendLine();
            stringBuilder.AppendFormat("Schedule Policy : {0}", this.BackupSchedule).AppendLine();
            stringBuilder.AppendFormat("Backup Storage : {0}", this.Storage).AppendLine();
            return stringBuilder.ToString();
        }

        internal Builder ToBuilder()
        {
            return new Builder(this);
        }

        internal class Builder
        {
            private BackupPolicy tempObject;

            internal Builder(BackupPolicy originalObject)
            {
                this.tempObject = new BackupPolicy(originalObject);
            }

            internal Builder UpdateUniqueId()
            {
                tempObject.UniqueId = Guid.NewGuid();
                return this;
            }

            internal Builder WithName(string value)
            {
                tempObject.Name = value;
                return this;
            }

            internal Builder WithBackupStorage(BackupStorage value)
            {
                tempObject.Storage = value;
                return this;
            }

            internal Builder WithBackupEnabledSet(IEnumerable<string> value)
            {
                tempObject.backupEnabledSet = new HashSet<string>(value);
                return this;
            }

            internal Builder AddToBackupEnabledSet(string value)
            {
                tempObject.backupEnabledSet.Add(value);
                return this;
            }

            internal Builder RemoveFromBackupEnabledSet(string value)
            {
                tempObject.backupEnabledSet.Remove(value);
                return this;
            }

            internal BackupPolicy Build()
            {
                var updatedObject = tempObject;
                tempObject = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return updatedObject;
            }
        }
    }
}