// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Frequency based backup scheduling policy
    /// </summary>
    [Serializable]
    internal class FrequencyBasedBackupPolicy : BackupPolicy
    {
        /// <summary>
        /// Run frequency type
        /// </summary>
        public BackupPolicyRunFrequency RunFrequencyType { get; set; }

        /// <summary>
        /// value of run frequency
        /// </summary>
        public ushort RunFrequency { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public FrequencyBasedBackupPolicy()
            : base(BackupPolicyType.FrequencyBased)
        {
        }

        /// <summary>
        /// Gets a string representation of the frequency based backup policy object
        /// </summary>
        /// <returns>A string representation of the frequency based backup policy object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "RunFrequencyType={0}", this.RunFrequencyType));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "RunFrequency={0}", this.RunFrequency));

            return sb.ToString();
        }

        #region Interop Helpers

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeBackupPolicy = GetNativeBackupPolicy(pinCollection);

            var nativeFrquencyPolicyDescription = new NativeBackupRestoreTypes.FABRIC_FREQUENCY_BASED_BACKUP_POLICY
            {
                RunFrequencyType = (NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE) RunFrequencyType,
                Value = RunFrequency,
            };

            nativeBackupPolicy.PolicyDescription = pinCollection.AddBlittable(nativeFrquencyPolicyDescription);
            

            return pinCollection.AddBlittable(nativeBackupPolicy);
        }

        internal static unsafe FrequencyBasedBackupPolicy FromNative(NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY backupPolicy)
        {
            var frequencyBasedPolicy = new FrequencyBasedBackupPolicy();
            frequencyBasedPolicy.PopulateBackupPolicyFromNative(backupPolicy);

            var frequencyBasedPolicyDescription =
                *(NativeBackupRestoreTypes.FABRIC_FREQUENCY_BASED_BACKUP_POLICY*) backupPolicy.PolicyDescription;

            frequencyBasedPolicy.RunFrequencyType =
                (BackupPolicyRunFrequency) frequencyBasedPolicyDescription.RunFrequencyType;
            frequencyBasedPolicy.RunFrequency = frequencyBasedPolicyDescription.Value;

            return frequencyBasedPolicy;
        }

        # endregion
    }
}