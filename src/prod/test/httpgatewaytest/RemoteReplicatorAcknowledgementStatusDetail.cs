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
    class RemoteReplicatorAcknowledgementStatusDetail
    {
        [DataMember(IsRequired = true)]
        public TimeSpan AverageReceiveDuration { get; private set; }

        [DataMember(IsRequired = true)]
        public TimeSpan AverageApplyDuration { get; private set; }

        [DataMember(IsRequired = true)]
        public long NotReceivedCount { get; private set; }

        [DataMember(IsRequired = true)]
        public long ReceivedAndNotAppliedCount { get; private set; }

        public static explicit operator RemoteReplicatorAcknowledgementStatusDetail(RemoteReplicatorAcknowledgementDetail v)
        {
            return new RemoteReplicatorAcknowledgementStatusDetail()
            {
                AverageReceiveDuration = v.AverageReceiveDuration,
                AverageApplyDuration = v.AverageApplyDuration,
                NotReceivedCount = v.NotReceivedCount,
                ReceivedAndNotAppliedCount = v.ReceivedAndNotAppliedCount
            };
        }
    }
}