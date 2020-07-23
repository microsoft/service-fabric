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
    class ServiceGroupMemberMembersResult
    {
        public ServiceGroupMemberMembersResult()
        {
            ServiceKind = Query.ServiceKind.Invalid;
            ServiceName = "";
            ServiceTypeName = "";
            InitializationData = new Byte[] { };
            ServiceLoadMetrics = new Byte[] { };
        }

        [DataMember]
        public System.Fabric.Query.ServiceKind ServiceKind
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceName
        {
            get;
            set;
        }

        [DataMember]
        public string ServiceTypeName
        {
            get;
            set;
        }

        [DataMember]
        public Byte[] InitializationData
        {
            get;
            set;
        }

        [DataMember]
        public Byte[] ServiceLoadMetrics
        {
            get;
            set;
        }
    }
}