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
    class ApplicationUpgradeProgressResult
    {
        public ApplicationUpgradeProgressResult()
        {
            UpgradeDomains = new List<UpgradeDomainStatus>();
        }

        [DataMember(IsRequired=true)]
        public string Name
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string TypeName
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string TargetApplicationTypeVersion
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public List<UpgradeDomainStatus> UpgradeDomains
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.ApplicationUpgradeState UpgradeState
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string NextUpgradeDomain
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.RollingUpgradeMode RollingUpgradeMode
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.UpgradeDomainProgress CurrentUpgradeDomainProgress
        {
            get;
            set;
        }
    }
}