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
    class PropertyMetadata
    {
        public PropertyMetadata()
        {
            TypeId = "";
            CustomTypeId = "";
            Parent = "";
            SizeInBytes = 0;
            LastModifiedUtcTimestamp = "";
            SequenceNumber = "";
        }

        [DataMember]
        public string TypeId
        {
            get;
            set;
        }

        [DataMember]
        public string CustomTypeId
        {
            get;
            set;
        }

        [DataMember]
        public string Parent
        {
            get;
            set;
        }

        [DataMember]
        public int SizeInBytes
        {
            get;
            set;
        }

        [DataMember]
        public string LastModifiedUtcTimestamp
        {
            get;
            set;
        }

        [DataMember]
        public string SequenceNumber
        {
            get;
            set;
        }
    }
}