// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Specialized;

    [Serializable]
    public sealed class UnreliableTransportBehavior
    {
        #region Constructors & Initialization

        public UnreliableTransportBehavior(
            string destination = "*",
            string actionName = "*",
            double probabilityToApply = 1.0,
            TimeSpan? delay = null,
            TimeSpan? delaySpan = null,
            int priority = 0,
            int applyCount = -1)
        {
            this.Destination = destination;
            this.ActionName = actionName;
            this.ProbabilityToApply = probabilityToApply;
            this.Delay = delay ?? TimeSpan.MaxValue;
            this.DelaySpan = delaySpan ?? TimeSpan.Zero;
            this.Priority = priority;
            this.ApplyCount = applyCount;
            this.Filters = new NameValueCollection();
        }

        #endregion

        #region Properties

        public string Destination { get; internal set; }

        public string ActionName { get; internal set; }

        public double ProbabilityToApply { get; internal set; }

        public TimeSpan Delay { get; internal set; }

        public TimeSpan DelaySpan { get; internal set; }

        public int Priority { get; internal set; }

        public int ApplyCount { get; internal set; }

        public NameValueCollection Filters { get; internal set; }

        #endregion
        
        /// <summary>
        /// Adds Partition Id to Filters.
        /// </summary>
        /// <param name="replicaId">Partition ID to add filter for.</param>
        public void AddFilterForPartitionId(Guid replicaId)
        {
            Filters.Add(PartitionId, replicaId.ToString());
        }

        internal System.Fabric.Common.UnreliableTransportBehavior Convert()
        {
            var utb = new System.Fabric.Common.UnreliableTransportBehavior(
                this.Destination, 
                this.ActionName, 
                this.ProbabilityToApply, 
                this.Delay, 
                this.DelaySpan, 
                this.Priority, 
                this.ApplyCount);

            utb.Filters = this.Filters;

            return utb;
        }

        public static string PartitionId = "PartitionId";
    }
}


#pragma warning restore 1591