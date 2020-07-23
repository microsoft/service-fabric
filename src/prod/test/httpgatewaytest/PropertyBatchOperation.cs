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
    class PropertyBatchOperation
    {

        public PropertyBatchOperation()
        {
            Kind = "";
            PropertyName = "";
            Exists = null;
            SequenceNumber = null;
            IncludeValue = null;
            CustomTypeId = null;
            Value = null;
        }

        [DataMember(Order = 0)]
        public string Kind
        {
            get;
            set;
        }

        [DataMember(Order = 1)]
        public string PropertyName
        {
            get;
            set;
        }

        [DataMember(Order = 2, EmitDefaultValue = false)]
        public bool? Exists
        {
            get;
            set;
        }

        [DataMember(Order = 3, EmitDefaultValue = false)]
        public string SequenceNumber
        {
            get;
            set;
        }

        [DataMember(Order = 4, EmitDefaultValue = false)]
        public bool? IncludeValue
        {
            get;
            set;
        }

        [DataMember(Order = 5, EmitDefaultValue = false)]
        public string CustomTypeId
        {
            get;
            set;
        }

        [DataMember(Order = 6, EmitDefaultValue = false)]
        public PropertyValue Value
        {
            get;
            set;
        }
    }
}