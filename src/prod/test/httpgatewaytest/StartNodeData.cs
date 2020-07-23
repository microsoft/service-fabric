// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class StartNodeData
    {
        public StartNodeData()
        {          
            this.IpAddressOrFQDN = "1.2.3.4";
            this.ClusterConnectionPort = 567;
            this.NodeInstanceId = "0";
        }
  
        [DataMember(IsRequired = true)]
        public string IpAddressOrFQDN
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public int ClusterConnectionPort
        {
            get;
            set;
        }
        
        [DataMember(IsRequired = true)]
        public string NodeInstanceId
        {
            get;
            set;
        }
    }
}