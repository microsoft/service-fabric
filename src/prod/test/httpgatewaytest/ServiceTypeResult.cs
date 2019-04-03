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
    public class ServiceTypeResult
    {
        [DataMember(IsRequired=true)]
        public ServiceTypeDescription ServiceTypeDescription
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string ServiceManifestName
        {
            get;
            set;
        }
        [DataMember(IsRequired = true)]
        public string ServiceManifestVersion
        {
            get;
            set;
        }
    }
}