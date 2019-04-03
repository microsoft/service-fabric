// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;
using System.Fabric;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class ScalingPolicyDescriptionTest
    {
        public ScalingPolicyDescriptionTest()
        {
        }

        public ScalingPolicyDescriptionTest(InstanceCountScaleMechanismTest mechanism, AveragePartitionLoadTriggerTest trigger)
        {
            this.ScalingMechanism = mechanism;
            this.ScalingTrigger = trigger;
        }
        [DataMember]
        public AveragePartitionLoadTriggerTest ScalingTrigger
        {
            get;
            set;
        }

        [DataMember]
        public InstanceCountScaleMechanismTest ScalingMechanism
        {
            get;
            set;
        }
    }
}