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
    class RestartCodePackageData
    {
        public RestartCodePackageData()
        {
            this.CodePackageInstanceId = "0";
            this.ServiceManifestName = string.Empty;
            this.CodePackageName = string.Empty;
        }

        [DataMember(IsRequired = true)]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string CodePackageName
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string CodePackageInstanceId
        {
            get;
            set;
        }
    }
}