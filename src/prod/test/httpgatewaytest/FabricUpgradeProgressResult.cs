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
    class FabricUpgradeProgressResult
    {
        [DataMember(IsRequired = true)]
        public string CodeVersion
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string ConfigVersion
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.FabricUpgradeState UpgradeState
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