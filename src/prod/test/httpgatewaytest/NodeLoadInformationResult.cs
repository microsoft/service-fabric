// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class NodeLoadInformationResult
    {
        public NodeLoadInformationResult()
        {
            NodeLoadMetricInformation = new List<NodeLoadMetricInformationResult>();
        }

        [DataMember(IsRequired = true)]
        public string NodeName
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public List<NodeLoadMetricInformationResult> NodeLoadMetricInformation
        {
            get;
            set;
        }

    }
}