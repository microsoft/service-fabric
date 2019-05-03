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
    class FabricUpgradeDescriptionInput
    {
        [DataMember]
        public string CodeVersion
        {
            get;
            set;
        }

        [DataMember]
        public string ConfigVersion
        {
            get;
            set;
        }

        [DataMember]
        public System.Fabric.Description.UpgradeKind UpgradeKind
        {
            get;
            set;
        }

        [DataMember]
        public System.Fabric.RollingUpgradeMode RollingUpgradeMode
        {
            get;
            set;
        }

        [DataMember]
        public bool ForceRestart
        {
            get;
            set;
        }

        [DataMember]
        public Int32 UpgradeReplicaSetTimeoutInSeconds
        {
            get;
            set;
        }
    }
}