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
    // Each description kind gets its own class so that the serializer
    // properly handles the "Value" field.

    [Serializable]
    [DataContract]
    class PropertyDescription
    {
        public PropertyDescription()
        {
            PropertyName = "";
            Value = new PropertyValue();
            CustomTypeId = "";
        }

        [DataMember]
        public string PropertyName
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
        public PropertyValue Value
        {
            get;
            set;
        }

        public void OnSerialize()
        {
            Value.OnSerialize();
        }
    }

    [Serializable]
    [DataContract]
    class BinaryPropertyDescription
    {
        public BinaryPropertyDescription()
        {
            PropertyName = "";
            Value = new BinaryPropertyValue();
            CustomTypeId = "";
        }

        [DataMember]
        public string PropertyName
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
        public BinaryPropertyValue Value
        {
            get;
            set;
        }
    }

    [Serializable]
    [DataContract]
    class DoublePropertyDescription
    {
        public DoublePropertyDescription()
        {
            PropertyName = "";
            Value = new DoublePropertyValue();
            CustomTypeId = "";
        }

        [DataMember]
        public string PropertyName
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
        public DoublePropertyValue Value
        {
            get;
            set;
        }
    }
}