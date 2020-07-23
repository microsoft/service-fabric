// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Collections.Generic;
    using System.Text;
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.Interop;
    using System.Linq;

    [DataContract]
    internal class RestoreStatus
    {
        [DataMember]
        internal string FabricUri { get; private set; }

        [DataMember]
        internal RestoreState RestoreStatusState { get; private set; }

        [DataMember]
        internal string RestoreDetailsMessage { get; private set; }

        [DataMember (Name = "BackupLocations")]
        private List<string> backupLocations;

        internal IReadOnlyList<string> BackupLocations
        {
            get { return backupLocations; }
        }

        [DataMember]
        internal BackupStorage BackupStorage { get; private set; }

        [DataMember]
        internal  DateTime TimeStampUtc { get; private set; }

        [DataMember]
        internal BackupRestoreView.RestorePoint RestorePartitionDetails { get; private set; }       // BUG: We are using view object here

        [DataMember]
        internal Guid DataLossGuid { get; private set; }

        [DataMember]
        internal int ErrorCode { get; private set; }

        [DataMember]
        internal Guid RestoreRequestGuid { get; private set; }

        /// <summary>
        /// Internal Constructor
        /// </summary>
        /// <param name="other">Other object to copy from</param>
        internal RestoreStatus(RestoreStatus other)
        {
            if (null == other)
            {
                throw new ArgumentNullException("other");
            }

            if (null != other.BackupLocations)
            {
                this.backupLocations = new List<string>(other.backupLocations);
            }

            if (null != other.BackupStorage)
            {
                this.BackupStorage = other.BackupStorage.DeepCopy();
            }

            this.DataLossGuid = other.DataLossGuid;
            this.ErrorCode = other.ErrorCode;
            this.FabricUri = other.FabricUri;
            this.RestoreDetailsMessage = other.RestoreDetailsMessage;
            this.RestorePartitionDetails = other.RestorePartitionDetails;       // Note: It is assumed that this object is not going to be mutated
            this.RestoreRequestGuid = other.RestoreRequestGuid;
            this.RestoreStatusState = other.RestoreStatusState;
            this.TimeStampUtc = other.TimeStampUtc;
        }

        internal RestoreStatus(string fabricUri, BackupStorage backupStorage, Guid restoreRequestGuid, Guid dataLossGuid, List<string> backupLocations, BackupRestoreView.RestorePoint restoreDetails)
        {
            this.FabricUri = fabricUri;
            this.RestoreRequestGuid = restoreRequestGuid;
            this.BackupStorage = backupStorage;
            this.backupLocations = backupLocations;
            this.DataLossGuid = dataLossGuid;
            this.RestorePartitionDetails = restoreDetails;
            this.RestoreStatusState = RestoreState.Accepted;
            this.RestoreDetailsMessage = string.Empty;
        }

        internal BackupRestoreView.RestoreProgress ToRestoreResponse()
        {
            BackupRestoreView.RestoreProgress restoreProgress = new BackupRestoreView.RestoreProgress();
            restoreProgress.RestoreState = this.RestoreStatusState;
            switch (this.RestoreStatusState)
            {
                    case RestoreState.Success:
                    case RestoreState.Accepted:
                    case RestoreState.RestoreInProgress:
                        restoreProgress.RestoredEpoch = this.RestorePartitionDetails.EpochOfLastBackupRecord;
                        restoreProgress.RestoredLsn = this.RestorePartitionDetails.LsnOfLastBackupRecord;
                        restoreProgress.TimeStampUtc = this.TimeStampUtc;
                    break;
                    case RestoreState.Invalid:
                    case RestoreState.Timeout:
                    case RestoreState.Failure:
                    restoreProgress.FailureError = new FabricError()
                    {
                        Message = this.RestoreDetailsMessage,
                        Code = (NativeTypes.FABRIC_ERROR_CODE)this.ErrorCode 
                    };
                        
                    break;

            }
            return restoreProgress;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Fabric Uri : {0}",this.FabricUri).AppendLine();
            stringBuilder.AppendFormat("BackupStorage : {0}", this.BackupStorage).AppendLine();
            stringBuilder.AppendFormat("BackupLocations : {0}", string.Join(",", this.backupLocations)).AppendLine();
            stringBuilder.AppendFormat("DataLossGuid : {0}", this.DataLossGuid).AppendLine();
            stringBuilder.AppendFormat("ErrorCode : {0}", this.ErrorCode).AppendLine();
            stringBuilder.AppendFormat("RestorePartitionDetails : {0}", this.RestorePartitionDetails).AppendLine();
            stringBuilder.AppendFormat("RestoreRequestGuid : {0}", this.RestoreRequestGuid).AppendLine();
            stringBuilder.AppendFormat("TimeStampUtc : {0}", this.TimeStampUtc).AppendLine();
            stringBuilder.AppendFormat("RestoreDetailsMessage : {0}", this.RestoreStatusState).AppendLine();
            stringBuilder.AppendFormat("RestoreDetailsMessage : {0}", this.RestoreDetailsMessage).AppendLine();
            return stringBuilder.ToString();
        }

        internal Builder ToBuilder()
        {
            return new Builder(this);
        }

        /// <summary>
        /// Builder class for RestoreStatus
        /// </summary>
        internal sealed class Builder
        {
            private RestoreStatus restoreStatus;

            internal Builder(RestoreStatus restoreStatus)
            {
                this.restoreStatus = new RestoreStatus(restoreStatus);
            }

            internal Builder WithState(RestoreState value)
            {
                restoreStatus.RestoreStatusState = value;
                return this;
            }

            internal Builder WithErrorCode(int value)
            {
                restoreStatus.ErrorCode = value;
                return this;
            }

            internal Builder WithMessage(string value)
            {
                restoreStatus.RestoreDetailsMessage = value;
                return this;
            }

            internal Builder WithTimeStampUtc(DateTime value)
            {
                restoreStatus.TimeStampUtc = value;
                return this;
            }

            internal RestoreStatus Build()
            {
                var tempPartitionStatus = restoreStatus;
                restoreStatus = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return tempPartitionStatus;
            }
        }
    }
}