// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class ApplicationUpgradeDescriptionInput
    {
        public ApplicationUpgradeDescriptionInput()
        {
            Name = "";
            TargetApplicationTypeVersion = "";
            UpgradeKind = Fabric.Description.UpgradeKind.Invalid;
            RollingUpgradeMode = Fabric.RollingUpgradeMode.Invalid;
            UpgradeReplicaSetTimeoutInSeconds = 0;
            ForceRestart = false;
            Parameters = new Dictionary<string, string>();
        }

        [DataMember]
        public string Name
        {
            get;
            set;
        }

        [DataMember]
        public string TargetApplicationTypeVersion
        {
            get;
            set;
        }

        [DataMember]
        public Dictionary<string, string> Parameters
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