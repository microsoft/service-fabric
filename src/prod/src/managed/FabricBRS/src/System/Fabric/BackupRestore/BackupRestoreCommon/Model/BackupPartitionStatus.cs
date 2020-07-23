// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{

    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.Interop;
    using System.Text;
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;

    [DataContract]
    internal class BackupPartitionStatus
    {
        [DataMember]
        internal BackupState BackupPartitionStatusState { get; private set; }

        [DataMember]
        internal string Uri { get; private set; }
        
        [DataMember]
        internal int ErrorCode { get; private set; }

        [DataMember]
        internal string Message { get; private set; }
        
        [DataMember]
        internal DateTime TimeStampUtc { get; private set; }

        [DataMember]
        internal Guid OperationId { get; private set; }

        [DataMember]
        internal Guid BackupId { get; private set; }

        [DataMember]
        internal string BackupLocation { get; private set; }

        [DataMember]
        internal BackupRestoreView.BackupEpoch EpochOfLastBackupRecord { get; private set; }            // BUG: We are using view type here

        [DataMember]
        internal long LsnOfLastBackupRecord { get; private set; }
        
        internal BackupPartitionStatus(string fabricUri , Guid operationId)
        {
            this.Uri = fabricUri;
            this.OperationId = operationId;
            this.BackupPartitionStatusState = BackupState.Accepted;
        }

        /// <summary>
        /// Internal copy constructor
        /// </summary>
        /// <param name="other">Object to copy from</param>
        internal BackupPartitionStatus(BackupPartitionStatus other)
        {
            if (null == other)
            {
                throw new ArgumentNullException("other");
            }

            this.BackupId = other.BackupId;
            this.BackupLocation = other.BackupLocation;
            this.BackupPartitionStatusState = other.BackupPartitionStatusState;
            this.EpochOfLastBackupRecord = other.EpochOfLastBackupRecord;               // Note: This is assumed here that Epoch is not mutated
            this.ErrorCode = other.ErrorCode;
            this.LsnOfLastBackupRecord = other.LsnOfLastBackupRecord;
            this.Message = other.Message;
            this.OperationId = other.OperationId;
            this.TimeStampUtc = other.TimeStampUtc;
            this.Uri = other.Uri;
        }

        internal BackupRestoreView.BackupProgress ToBackupPartitionResponse()
        {
            BackupRestoreView.BackupProgress backupProgress = new BackupRestoreView.BackupProgress();
            backupProgress.BackupState = this.BackupPartitionStatusState;
            switch (this.BackupPartitionStatusState)
            {
                    case BackupState.Accepted:
                    case BackupState.BackupInProgress:
                    case BackupState.Success:
                        backupProgress.EpochOfLastBackupRecord = this.EpochOfLastBackupRecord;
                        backupProgress.BackupLocation = this.BackupLocation;
                        backupProgress.TimeStampUtc = this.TimeStampUtc;
                        backupProgress.LsnOfLastBackupRecord = this.LsnOfLastBackupRecord;
                        backupProgress.BackupId = this.BackupId;
                    break;
                    case BackupState.Failure:
                    case BackupState.Timeout:
                    backupProgress.FailureError = new FabricError()
                    {
                        Code = (NativeTypes.FABRIC_ERROR_CODE) this.ErrorCode,
                        Message = this.Message
                    };

                    break;
            }
            return backupProgress;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("BackupPartitionStatusState :{0}", this.BackupPartitionStatusState).AppendLine();
            stringBuilder.AppendFormat("BackupId :{0}", this.BackupId).AppendLine();
            stringBuilder.AppendFormat("BackupLocation :{0}", this.BackupLocation).AppendLine();
            stringBuilder.AppendFormat("Uri :{0}", this.Uri).AppendLine();
            stringBuilder.AppendFormat("ErrorCode :{0}", this.ErrorCode).AppendLine();
            stringBuilder.AppendFormat("Message :{0}", this.Message).AppendLine();
            stringBuilder.AppendFormat("OperationId :{0}", this.OperationId).AppendLine();
            stringBuilder.AppendFormat("EpochOfLastBackupRecord :{0}", this.EpochOfLastBackupRecord).AppendLine();
            stringBuilder.AppendFormat("LsnOfLastBackupRecord :{0}", this.LsnOfLastBackupRecord).AppendLine();
            stringBuilder.AppendFormat("TimeStampUtc :{0}", this.TimeStampUtc).AppendLine();
            return stringBuilder.ToString();
        }

        internal Builder ToBuilder()
        {
            return new Builder(this);
        }

        /// <summary>
        /// Builder class for BackupPartitionStatus
        /// </summary>
        internal sealed class Builder
        {
            private BackupPartitionStatus partitionStatus;

            internal Builder(BackupPartitionStatus partitionStatus)
            {
                this.partitionStatus = new BackupPartitionStatus(partitionStatus);
            }

            internal Builder WithState(BackupState value)
            {
                partitionStatus.BackupPartitionStatusState = value;
                return this;
            }

            internal Builder WithErrorCode(int value)
            {
                partitionStatus.ErrorCode = value;
                return this;
            }

            internal Builder WithMessage(string value)
            {
                partitionStatus.Message = value;
                return this;
            }

            internal Builder WithTimeStampUtc(DateTime value)
            {
                partitionStatus.TimeStampUtc = value;
                return this;
            }

            internal Builder WithBackupId(Guid value)
            {
                partitionStatus.BackupId = value;
                return this;
            }

            internal Builder WithBackupLocation(string value)
            {
                partitionStatus.BackupLocation = value;
                return this;
            }

            internal Builder WithEpochOfLastBackupRecord(BackupRestoreView.BackupEpoch value)
            {
                partitionStatus.EpochOfLastBackupRecord = value;
                return this;
            }

            internal Builder WithLsnOfLastBackupRecord(long value)
            {
                partitionStatus.LsnOfLastBackupRecord = value;
                return this;
            }

            internal BackupPartitionStatus Build()
            {
                var tempPartitionStatus = partitionStatus;
                partitionStatus = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return tempPartitionStatus;
            }
        }
    }
}