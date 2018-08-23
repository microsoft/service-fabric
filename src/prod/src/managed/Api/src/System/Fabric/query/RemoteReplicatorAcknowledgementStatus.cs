// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Provides copy and replication stream acknowledgement details. Member of <see cref="System.Fabric.Query.RemoteReplicatorStatus" />
    /// </summary>
    public sealed class RemoteReplicatorAcknowledgementStatus
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.RemoteReplicatorAcknowledgementStatus" /> class.</para>
        /// </summary>
        public RemoteReplicatorAcknowledgementStatus()
        {
        }

        /// <summary>
        /// Contains details regarding acknowledgement for the replication operation stream
        /// </summary>
        /// <value>
        /// <para>Returns RemoteReplicatorAcknowledgementDetail object for replication operation stream. See <see cref="System.Fabric.Query.RemoteReplicatorAcknowledgementDetail" /> for more information. </para>
        /// </value>
        public RemoteReplicatorAcknowledgementDetail ReplicationStreamAcknowledgementDetail { get; private set; }

        /// <summary>
        /// Contains details regarding acknowledgement for the copy operation stream
        /// </summary>
        /// <value>
        /// <para>Returns RemoteReplicatorAcknowledgementDetail object for copy operation stream. See <see cref="System.Fabric.Query.RemoteReplicatorAcknowledgementDetail" /> for more information.</para>
        /// </value>
        public RemoteReplicatorAcknowledgementDetail CopyStreamAcknowledgementDetail { get; private set; }

        internal static unsafe RemoteReplicatorAcknowledgementStatus CreateFromNative(NativeTypes.FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_STATUS nativeResultItem)
        {
            return new RemoteReplicatorAcknowledgementStatus
            {
                ReplicationStreamAcknowledgementDetail =
                    RemoteReplicatorAcknowledgementDetail.CreateFromNative(*(NativeTypes.FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL*)nativeResultItem.ReplicationStreamAcknowledgementDetails),
                CopyStreamAcknowledgementDetail = 
                    RemoteReplicatorAcknowledgementDetail.CreateFromNative(*(NativeTypes.FABRIC_REMOTE_REPLICATOR_ACKNOWLEDGEMENT_DETAIL*)
                    nativeResultItem.CopyStreamAcknowledgementDetails)
            };
        }
    }
}