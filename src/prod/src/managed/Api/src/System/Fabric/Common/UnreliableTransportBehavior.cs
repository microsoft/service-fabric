// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Specialized;
    [Serializable]
    internal sealed class UnreliableTransportBehavior
    {
        #region Constructors & Initialization

        internal UnreliableTransportBehavior(
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

        public static string PartitionId = "PartitionId";
    }
}