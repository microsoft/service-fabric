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
    class PartitionDescription
    {
        public PartitionDescription()
        {
            PartitionScheme = Fabric.Description.PartitionScheme.Invalid;
            Names = new List<string>();
            Count = 0;
            LowKey = "";
            HighKey = "";
        }

        [DataMember]
        public System.Fabric.Description.PartitionScheme PartitionScheme
        {
            get;
            set;
        }

        [DataMember]
        public int Count
        {
            get;
            set;
        }

        [DataMember]
        public List<string> Names
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