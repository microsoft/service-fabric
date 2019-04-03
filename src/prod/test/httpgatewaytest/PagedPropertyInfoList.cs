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
    class PagedPropertyInfoList
    {

        public PagedPropertyInfoList()
        {
            ContinuationToken = "";
            IsConsistent = false;
            Properties = new List<PropertyInfo>();
        }

        [DataMember]
        public string ContinuationToken
        {
            get;
            set;
        }

        [DataMember]
        public bool IsConsistent
        {
            get;
            set;
        }

        [DataMember]
        public List<PropertyInfo> Properties
        {
            get;
            set;
        }

        public bool OnDeserialized()
        {
            bool success = true;
            foreach (PropertyInfo property in Properties)
            {
                success = property.Value.OnDeserialized();
            }
            return success;
        }
    }

    [Serializable]
    [DataContract]
    class PagedBinaryPropertyInfoList
    {

        public PagedBinaryPropertyInfoList()
        {
            ContinuationToken = "";
            IsConsistent = false;
            Properties = new List<BinaryPropertyInfo>();
        }

        [DataMember]
        public string ContinuationToken
        {
            get;
            set;
        }

        [DataMember]
        public bool IsConsistent
        {
            get;
            set;
        }

        [DataMember]
        public List<BinaryPropertyInfo> Properties
        {
            get;
            set;
        }
    }

    [Serializable]
    [DataContract]
    class PagedDoublePropertyInfoList
    {

        public PagedDoublePropertyInfoList()
        {
            ContinuationToken = "";
            IsConsistent = false;
            Properties = new List<DoublePropertyInfo>();
        }

        [DataMember]
        public string ContinuationToken
        {
            get;
            set;
        }

        [DataMember]
        public bool IsConsistent
        {
            get;
            set;
        }

        [DataMember]
        public List<DoublePropertyInfo> Properties
        {
            get;
            set;
        }
    }
}