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
    class PropertyBatchInfo
    {

        public PropertyBatchInfo()
        {
            Kind = "";
            Properties = new Dictionary<string, PropertyInfo>();
            ErrorMessage = null;
            OperationIndex = -1;
        }

        [DataMember]
        public string Kind
        {
            get;
            set;
        }

        [DataMember]
        public IDictionary<string, PropertyInfo> Properties
        {
            get;
            set;
        }

        [DataMember]
        public string ErrorMessage
        {
            get;
            set;
        }

        [DataMember]
        public long OperationIndex
        {
            get;
            set;
        }
    }
}