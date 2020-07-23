// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Text;
    using System.Runtime.Serialization;

    [DataContract]
    class WorkItemProcessInfo
    {
        [DataMember]
        internal Guid WorkItemId { get; private set; }

        [DataMember]
        internal DateTime DueDateTime { get; private set; }

        [DataMember]
        internal WorkItemQueueRunType WorkItemQueueRunType { get; private set; }

        internal WorkItemProcessInfo(Guid workItemId, DateTime dueDateTime)
        {
            this.WorkItemId = workItemId;
            this.DueDateTime = dueDateTime;
            this.WorkItemQueueRunType = WorkItemQueueRunType.WorkItemQueue;
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="other">Object to copy from</param>
        internal WorkItemProcessInfo(WorkItemProcessInfo other)
        {
            if (null == other)
            {
                throw new ArgumentNullException("other");
            }

            this.WorkItemId = other.WorkItemId;
            this.DueDateTime = other.DueDateTime;
            this.WorkItemQueueRunType = other.WorkItemQueueRunType;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("WorkItemGuid {0}", this.WorkItemId).AppendLine();
            stringBuilder.AppendFormat("DueDateTime {0}", this.DueDateTime).AppendLine();
            stringBuilder.AppendFormat("WorkitemQueueRuntype {0}", this.WorkItemQueueRunType);
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
            private WorkItemProcessInfo tempObject;

            internal Builder(WorkItemProcessInfo originalObject)
            {
                this.tempObject = new WorkItemProcessInfo(originalObject);
            }

            internal Builder WithDueDateTime(DateTime value)
            {
                tempObject.DueDateTime = value;
                return this;
            }

            internal Builder WithWorkItemQueueRunTimeType(WorkItemQueueRunType value)
            {
                tempObject.WorkItemQueueRunType = value;
                return this;
            }

            internal WorkItemProcessInfo Build()
            {
                var updatedObject = tempObject;
                tempObject = null;     // So that build cannot be invoked again, not an ideal solution but works for us
                return updatedObject;
            }
        }
    }
}