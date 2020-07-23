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
    class PropertyValue
    {
        public PropertyValue()
        {
            Kind = "Invalid";
            Data = "";
        }

        [DataMember(Order = 0)]
        public string Kind
        {
            get;
            set;
        }

        [DataMember(Order = 1)]
        private string Data
        {
            get;
            set;
        }

        public string StringData
        {
            get;
            set;
        }

        public long Int64Data
        {
            get;
            set;
        }

        public Guid GuidData
        {
            get;
            set;
        }

        // Called immediately after deserialization.
        public bool OnDeserialized()
        {
            bool success = true;
            switch (Kind)
            {
                case "Int64":
                    long value;
                    success = long.TryParse(Data, out value);
                    if (success)
                    {
                        Int64Data = value;
                    }
                    break;
                case "String":
                    StringData = Data;
                    break;
                case "Guid":
                    Guid guidValue;
                    success = Guid.TryParse(Data, out guidValue);
                    if (success)
                    {
                        GuidData = guidValue;
                    }
                    break;
            }
            return success;
        }

        // Called immediately before serialization.
        public void OnSerialize()
        {
            switch (Kind)
            {
                case "Int64":
                    Data = Int64Data.ToString();
                    break;
                case "String":
                    Data = StringData;
                    break;
                case "Guid":
                    Data = GuidData.ToString();
                    break;
            }
        }
    }

    [Serializable]
    [DataContract]
    class BinaryPropertyValue
    {
        public BinaryPropertyValue()
        {
            Kind = "Binary";
            Data = new byte[0];
        }

        [DataMember(Order = 0)]
        public string Kind
        {
            get;
            set;
        }

        [DataMember(Order = 1)]
        public byte[] Data
        {
            get;
            set;
        }
    }

    [Serializable]
    [DataContract]
    class DoublePropertyValue
    {
        public DoublePropertyValue()
        {
            Kind = "Double";
            Data = -1;
        }

        [DataMember(Order = 0)]
        public string Kind
        {
            get;
            set;
        }

        [DataMember(Order = 1)]
        public double Data
        {
            get;
            set;
        }
    }
}