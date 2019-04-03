// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Query;
using System.Runtime.Serialization;

namespace System.Fabric.Test.HttpGatewayTest
{
    [Serializable]
    [DataContract]
    class NodesResult
    {
        [DataMember(IsRequired = true)]
        public string Name
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string IpAddressOrFQDN
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string Type
        {
            get;
            set;
        }

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
        public NodeStatus NodeStatus
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string NodeUpTimeInSeconds
        {
            get;
            set;
        }

        public TimeSpan NodeUpTime
        {
            get
            {
                if (NodeUpTimeInSeconds.Length == 0)
                    return TimeSpan.MinValue;
                else
                    return TimeSpan.FromSeconds(Convert.ToDouble(NodeUpTimeInSeconds));
            }
        }

        [DataMember(IsRequired = true)]
        public string NodeDownTimeInSeconds
        {
            get;
            set;
        }

        public TimeSpan NodeDownTime
        {
            get
            {
                if (NodeDownTimeInSeconds.Length == 0)
                    return TimeSpan.MinValue;
                else
                    return TimeSpan.FromSeconds(Convert.ToDouble(NodeDownTimeInSeconds));
            }
        }

        [DataMember(IsRequired = true)]
        public bool IsSeedNode
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string UpgradeDomain
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string FaultDomain
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public System.Fabric.Health.HealthState HealthState
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public NodeId Id
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public long InstanceId
        {
            get;
            set;
        }
    }
}