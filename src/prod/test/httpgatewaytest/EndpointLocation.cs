// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;
using System.Runtime.InteropServices.ComTypes;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    public class EndpointLocation
    {
        [DataMember(IsRequired = true)]
        public ServiceEndpointRole Kind
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string Address
        {
            get;
            set;
        }
    }
}