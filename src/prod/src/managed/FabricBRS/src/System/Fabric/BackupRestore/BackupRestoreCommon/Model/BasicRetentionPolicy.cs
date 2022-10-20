// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Text;

    [DataContract]
    internal class BasicRetentionPolicy :RetentionPolicy
    {
        [DataMember]
        internal TimeSpan RetentionDuration { set; get; }

        [DataMember]
        internal int MinimumNumberOfBackups { set; get; }

        internal BasicRetentionPolicy() : base(RetentionPolicyType.Basic)
        {
        }


        internal static BasicRetentionPolicy FromBasicRetentionPolicyView(
            BackupRestoreView.BasicRetentionPolicy basicRetentionPolicyView)
        {
            BasicRetentionPolicy basicRetentionPolicy = new BasicRetentionPolicy();

            TimeSpan timeSpan = basicRetentionPolicyView.RetentionDuration;
            
            if ((int)timeSpan.TotalHours == 0)
            {
                throw new ArgumentException(StringResources.InvalidInterval);
            }

            timeSpan = TimeSpan.FromHours((double)(int)timeSpan.TotalHours);
            basicRetentionPolicy.RetentionDuration = timeSpan;
            basicRetentionPolicy.MinimumNumberOfBackups = basicRetentionPolicyView.MinimumNumberOfBackups;
            return basicRetentionPolicy;
        }

        internal BasicRetentionPolicy(BasicRetentionPolicy other) : this()
        {
            this.MinimumNumberOfBackups = other.MinimumNumberOfBackups;
            this.RetentionDuration = other.RetentionDuration;

        }

        internal static BackupRestoreView.BasicRetentionPolicy ToBasicRetentionPolicyView(
            BasicRetentionPolicy basicRetentionPolicy)
        {
            BackupRestoreView.BasicRetentionPolicy basicRetentionPolicyView = new BackupRestoreView.BasicRetentionPolicy();
            basicRetentionPolicyView.RetentionDuration = basicRetentionPolicy.RetentionDuration;
            basicRetentionPolicyView.MinimumNumberOfBackups = basicRetentionPolicy.MinimumNumberOfBackups;
            return basicRetentionPolicyView;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Retention Duration {0}", this.RetentionDuration).AppendLine();
            stringBuilder.AppendFormat("Minimum number of backups {0}", this.MinimumNumberOfBackups);
            return stringBuilder.ToString();
        }
    }
    
    
}