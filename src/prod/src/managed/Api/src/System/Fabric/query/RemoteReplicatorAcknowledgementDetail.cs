// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// Provides acknowledgement details pertaining to a replication or copy stream. Member of <see cref="System.Fabric.Query.RemoteReplicatorStatus" />
    /// </summary>
    public sealed class RemoteReplicatorAcknowledgementDetail
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.RemoteReplicatorAcknowledgementDetail" /> class.</para>
        /// </summary>
        public RemoteReplicatorAcknowledgementDetail()
        {
        }

        /// <summary>
        /// Gets the average duration for receiving acks for operations
        /// </summary>
        /// <value>
        /// <para>Represents the average receive duration for a remote replicator instance.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan AverageReceiveDuration { get; internal set; }

        /// <summary>
        /// Gets the average duration for applying operations
        /// </summary>
        /// <value>
        /// <para>Represents the average apply duration for a remote replicator instance.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan AverageApplyDuration { get; internal set; }

        /// <summary>
        /// Gets the number of operations not yet received
        /// </summary>
        /// <value>
        /// <para>Represents the number of operations not yet received by a remote replicator.</para>
        /// </value>
        public long NotReceivedCount { get; internal set; }

        /// <summary>
        /// Gets the number of operations received and not applied
        /// </summary>
        /// <value>
        /// <para>Represents the number of operations received and not yet applied by a remote replicator.</para>
        /// </value>
        public long ReceivedAndNotAppliedCount { get; internal set; }

        [JsonCustomization(PropertyName = "AverageReceiveDuration")]
        private long AverageReceiveDurationMs 
        {
            get { return (int)this.AverageReceiveDuration.TotalMilliseconds; }
            set { this.AverageReceiveDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerMillisecond); }
        }

        [JsonCustomization(PropertyName = "AverageApplyDuration")]
        private long AverageApplyDurationMs 
        {
            get { return (int)this.AverageApplyDuration.TotalMilliseconds; }
            set { this.AverageApplyDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerMillisecond); }
        }

        internal static unsafe RemoteReplicatorAcknowledgementDetail CreateFromNative(NativeTypes.FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL nativeResultItem)
        {
            return new RemoteReplicatorAcknowledgementDetail
            {
                AverageReceiveDuration = TimeSpan.FromMilliseconds(nativeResultItem.AverageReceiveDurationMilliseconds),
                AverageApplyDuration = TimeSpan.FromMilliseconds(nativeResultItem.AverageApplyDurationMilliseconds),
                NotReceivedCount = nativeResultItem.NotReceivedCount,
                ReceivedAndNotAppliedCount = nativeResultItem.ReceivedAndNotAppliedCount
            };
        }
    }
}