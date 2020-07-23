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
    class ResolveServicePartitionResult
    {
        public ResolveServicePartitionResult()
        {
            PartitionInformation = new PartitionInformation();
            Endpoints = new List<EndpointLocation>();
        }

        [DataMember(IsRequired = true)]
        public string Name
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public PartitionInformation PartitionInformation
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public List<EndpointLocation> Endpoints
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string Version
        {
            get;
            set;
        }
    }
}