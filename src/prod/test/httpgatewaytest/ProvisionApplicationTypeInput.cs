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
    class ProvisionApplicationTypeInput
    {
        public ProvisionApplicationTypeInput(string input)
        {
            ApplicationTypeBuildPath = input;
        }

        [DataMember]
        public string ApplicationTypeBuildPath
        {
            get;
            set;
        }
    }
}