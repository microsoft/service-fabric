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
    class ProvisionFabricInput
    {
        public ProvisionFabricInput(string codeFilePath, string clusterManFilePath)
        {
            CodeFilePath = codeFilePath;
            ClusterManifestFilePath = clusterManFilePath;
        }

        [DataMember]
        public string CodeFilePath
        {
            get;
            set;
        }

        [DataMember]
        public string ClusterManifestFilePath
        {
            get;
            set;
        }
    }
}