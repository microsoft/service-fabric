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
    class InstanceCountScaleMechanismTest
    {
        public InstanceCountScaleMechanismTest()
        {
            Kind = Fabric.Description.ScalingMechanismKind.ScalePartitionInstanceCount;
        }

        [DataMember]
        public System.Fabric.Description.ScalingMechanismKind Kind
        {
            get;
            set;
        }

        [DataMember]
        public int MinInstanceCount
        {
            get;
            set;
        }

        [DataMember]
        public int MaxInstanceCount
        {
            get;
            set;
        }

        [DataMember]
        public int ScaleIncrement
        {
            get;
            set;
        }
    }
}