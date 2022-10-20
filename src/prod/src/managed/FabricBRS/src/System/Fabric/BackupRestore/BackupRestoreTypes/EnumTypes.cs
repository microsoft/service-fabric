// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Enums
{
    /// <summary>
    /// List types of Backup Schedule Available
    /// </summary>
    public enum BackupScheduleType
    {
        /// <summary>
        /// Invalid value
        /// </summary>
        Invalid,
        /// <summary>
        /// Type to set a Scheduled backup happening at particular time 
        /// </summary>
        TimeBased,
        /// <summary>
        /// Type to set a frequency based schedule happening at fixed regular interval
        /// </summary>
        FrequencyBased
    }

    /// <summary>
    /// List types of Retention Available
    /// </summary>
    public enum RetentionPolicyType
    {
        /// <summary>
        /// Describes the retention of backup where we can specify a duration of time to retain backups.
        /// </summary>
        Basic
    }


    /// <summary>
    /// Describes the schedule of running a Backup
    /// </summary>
    public enum BackupScheduleFrequency
    {
        /// <summary>
        /// Invalid value
        /// </summary>
        Invalid,
        /// <summary>
        /// Setting  the frequency to Daily of running a scheduled type backup
        /// </summary>
        Daily,
        /// <summary>
        /// Setting  the frequency to Weekly of running a scheduled type backup
        /// </summary>
        Weekly
    }

    /// <summary>
    /// Describes the frequency of running a frequency type Backup
    /// </summary>
    public enum BackupScheduleInterval
    {
        /// <summary>
        /// Invalid value
        /// </summary>
        Invalid,
        /// <summary>
        /// Setting  the frequency to Hours interval of running a frequency type backup
        /// </summary>
        Hours,
        /// <summary>
        /// Setting  the frequency to Minute interval of running a frequency type backup
        /// </summary>
        Minutes
    }

    /// <summary>
    /// Describes the frequency of Retaining a Backup
    /// </summary>
    public enum RetentionDurationType
    {
        /// <summary>
        /// Setting the frequency Retaining to Days
        /// </summary>
        Days,
        /// <summary>
        /// Setting the frequency Retaining to Weeks
        /// </summary>
        Weeks,
        /// <summary>
        /// Setting the frequency Retaining to Months
        /// </summary>
        Months
    }

    /// <summary>
    /// Describes the Storage type for saving the backups
    /// </summary>
    public enum BackupStorageType
    {
        /// <summary>
        /// Invalid value
        /// </summary>
        Invalid,
        /// <summary>
        /// Setting the storage type to file share type
        /// </summary>
        FileShare,
        /// <summary>
        /// Setting the storage type to azure blob storage type
        /// </summary>
        AzureBlobStore,
        /// <summary>
        /// Setting the storage type to dSMS manaed Azure storage type
        /// </summary>
        DsmsAzureBlobStore
    }

    /// <summary>
    /// Describes the Details of Backup Taken 
    /// </summary>
    public enum BackupOptionType
    {
        /// <summary>
        /// Invalid value
        /// </summary>
        Invalid,
        /// <summary>
        /// Defines a full Backup
        /// </summary>
        Full,
        /// <summary>
        /// Defines a Incremental Backup
        /// </summary>
        Incremental,
    }

    /// <summary>
    /// Specifies the Backup Protection Level
    /// </summary>
    public enum BackupPolicyScopeType
    {
        /// <summary>
        /// Invalid value
        /// </summary>
        Invalid,
        /// <summary>
        /// Indicates that backup has been applied at current level
        /// </summary>
        Partition,
        /// <summary>
        /// Indicates the backup has been applied at Service Level
        /// </summary>
        Service,
        /// <summary>
        /// Indicates the backup has been applied at Application Level
        /// </summary>
        Application

    }

    /// <summary>
    /// Specifies the Backup Entity Kind
    /// </summary>
    public enum BackupEntityKind
    {
        /// <summary>
        /// Indicates that enitity is Invalid
        /// </summary>
        Invalid,
        /// <summary>
        /// Indicates that Entity is Partition
        /// </summary>
        Partition,
        /// <summary>
        /// Indicates that Entity is Service
        /// </summary>
        Service,
        /// <summary>
        /// Indicates that Entity is Application
        /// </summary>
        Application

    }


    /// <summary>
    /// Represents the current state of a Restore Request
    /// </summary>
    public enum RestoreState
    {
        /// <summary>
        /// Restore requested is Invalid
        /// </summary>
        Invalid,
        /// <summary>
        /// Request is Accepted 
        /// </summary>
        Accepted,
        /// <summary>
        /// Restore is in Process
        /// </summary>
        RestoreInProgress,
        /// <summary>
        /// Restore Completed with Success
        /// </summary>
        Success,
        /// <summary>
        /// Restore Completed with Error
        /// </summary>
        Failure,
        /// <summary>
        /// Restore Requested has timeout
        /// </summary>
        Timeout,
    }

    /// <summary>
    /// Represents the current state of a Backup Request
    /// </summary>
    public enum BackupState
    {
        /// <summary>
        /// Backup requested is Invalid
        /// </summary>
        Invalid,
        /// <summary>
        /// Backup Request of Partition is Accepted
        /// </summary>
        Accepted,
        /// <summary>
        /// Backup Request of Partition is in Process
        /// </summary>
        BackupInProgress,
        /// <summary>
        /// Backup Request of Partition completed with Success
        /// </summary>
        Success,
        /// <summary>
        /// Backup Request of Partition completed with Error
        /// </summary>
        Failure,
        /// <summary>
        /// Backup Request of Partition has timeout
        /// </summary>
        Timeout
    }
}