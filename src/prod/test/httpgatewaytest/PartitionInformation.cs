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
    class PartitionInformation
    {
        [DataMember]
        public System.Fabric.ServicePartitionKind ServicePartitionKind
        {
            get;
            set;
        }

        [DataMember]
        public System.Guid Id
        {
            get;
            set;
        }

        [DataMember]
        public string Name
        {
            get;
            set;
        }

        [DataMember]
        public string LowKey
        {
            get;
            set;
        }

        [DataMember]
        public string HighKey
        {
            get;
            set;
        }
    }
}