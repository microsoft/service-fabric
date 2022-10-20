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
    public class GetServiceManifestInput : GetApplicationManifestInput
    {
        public GetServiceManifestInput(string applicationTypeVersion, string serviceManifestName)
            : base(applicationTypeVersion)
        {
        }

        [DataMember]
        public string ServiceManifestName
        {
            get;
            set;
        }
    }
}