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
    // Binary and Double values need their own class because they
    // are not sent as strings.

    [Serializable]
    [DataContract]
    class PropertyInfo
    {

        public PropertyInfo()
        {
            Name = "";
            Value = new PropertyValue();
            Metadata = new PropertyMetadata();
        }

        [DataMember]
        public string Name
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

        [DataMember]
        public PropertyMetadata Metadata
        {
            get;
            set;
        }

        public bool OnDeserialized()
        {
            return Value.OnDeserialized();
        }
    }

    [Serializable]
    [DataContract]
    class BinaryPropertyInfo
    {

        public BinaryPropertyInfo()
        {
            Name = "";
            Value = new BinaryPropertyValue();
            Metadata = new PropertyMetadata();
        }

        [DataMember]
        public string Name
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

        [DataMember]
        public PropertyMetadata Metadata
        {
            get;
            set;
        }
    }

    [Serializable]
    [DataContract]
    class DoublePropertyInfo
    {

        public DoublePropertyInfo()
        {
            Name = "";
            Value = new DoublePropertyValue();
            Metadata = new PropertyMetadata();
        }

        [DataMember]
        public string Name
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

        [DataMember]
        public PropertyMetadata Metadata
        {
            get;
            set;
        }
    }
}