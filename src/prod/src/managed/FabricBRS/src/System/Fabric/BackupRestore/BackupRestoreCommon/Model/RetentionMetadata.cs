// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using Newtonsoft.Json;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;
    using System.Runtime.Serialization;
    using BackupRestoreView = System.Fabric.BackupRestore.BackupRestoreTypes;

    [Serializable]
    internal class RetentionMetadata
    {
        internal DateTime RetentionPolicyUpdateTime { get; private set; }
        internal DateTime LastRetentionStartTime { get; private set; }
        internal DateTime LastRetentionCompletionTime { get; private set; }

        [JsonConverter(typeof(RetentionPolicyJsonConverter))]
        internal RetentionPolicy CurrentRetentionPolicy { get; private set; }
        internal Dictionary<string, string> OnGoingRetentionFile { get; private set; }

        internal RetentionMetadata(RetentionPolicy retentionPolicy)
        {
            this.CurrentRetentionPolicy = retentionPolicy;
            this.LastRetentionCompletionTime = DateTime.MinValue;
            this.LastRetentionStartTime = DateTime.MinValue;
            this.RetentionPolicyUpdateTime = DateTime.UtcNow;
            this.OnGoingRetentionFile = new Dictionary<string, string>();
        }

        internal void UpdateOnGoingRetentionFile(string partitionId, string fileName = null)
        {
            if(fileName == null)
            {
                if(this.OnGoingRetentionFile.Remove(partitionId))
                {
                    return;
                }
                else{
                    // This warning could come when there are no files to delete for a policy and in that case it is working correctly.
                    BackupRestoreTrace.TraceSource.WriteInfo("RetentionManagerTrace", " Not able to remove key from OnGoingRetentionFile dict for partitionId: {0}", partitionId);
                }
            }
            else
            {
                this.OnGoingRetentionFile[partitionId] = fileName;
            }
            return;
        }

        internal bool UpdateRetentionPolicyMetadata(RetentionPolicy retentionPolicy)
        {
            if (retentionPolicy.RetentionPolicyType != this.CurrentRetentionPolicy.RetentionPolicyType)
            {
                this.RetentionPolicyUpdateTime = DateTime.Now;
                this.CurrentRetentionPolicy = retentionPolicy;
                return true;
            }
            else
            {
                if (retentionPolicy.RetentionPolicyType == RetentionPolicyType.Basic)
                {
                    var derivedRetentionPolicy = (BasicRetentionPolicy)retentionPolicy;
                    var derivedCurrentRetentionPolicy = (BasicRetentionPolicy)retentionPolicy;
                    if(derivedRetentionPolicy.RetentionDuration == derivedCurrentRetentionPolicy.RetentionDuration &&
                        derivedRetentionPolicy.MinimumNumberOfBackups == derivedCurrentRetentionPolicy.MinimumNumberOfBackups)
                    {
                        return false;
                    }

                    this.CurrentRetentionPolicy = retentionPolicy;
                    return true;
                }

                return true;
            }
        }

        internal void UpdateLastRetentionStartTime()
        {
            this.LastRetentionStartTime = DateTime.Now;
        }

        internal void UpdateLastRetentionCompletionTime()
        {
            this.LastRetentionCompletionTime = DateTime.Now;
        }
    }

}