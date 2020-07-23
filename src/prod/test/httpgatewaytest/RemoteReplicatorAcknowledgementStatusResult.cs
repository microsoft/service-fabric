// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Query;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class RemoteReplicatorAcknowledgementStatusResult
    {
        [DataMember(IsRequired = true)]
        public RemoteReplicatorAcknowledgementStatusDetail CopyStreamAcknowledgementDetail { get; private set; }

        [DataMember(IsRequired = true)]
        public RemoteReplicatorAcknowledgementStatusDetail ReplicationStreamAcknowledgementDetail { get; private set; }

        public static explicit operator RemoteReplicatorAcknowledgementStatusResult(RemoteReplicatorAcknowledgementStatus v)
        {
            return new RemoteReplicatorAcknowledgementStatusResult()
            {
                CopyStreamAcknowledgementDetail = (RemoteReplicatorAcknowledgementStatusDetail) v.CopyStreamAcknowledgementDetail,
                ReplicationStreamAcknowledgementDetail = (RemoteReplicatorAcknowledgementStatusDetail) v.CopyStreamAcknowledgementDetail
            };
        }
    }
}