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
    class ClusterLoadInformationResult
    {
        public ClusterLoadInformationResult()
        {
            LoadMetricInformation = new List<LoadMetricInformationResult>();
        }

        [DataMember(IsRequired = true)]
        public string LastBalancingStartTimeUtc
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public string LastBalancingEndTimeUtc
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public List<LoadMetricInformationResult> LoadMetricInformation
        {
            get;
            set;
        }

    }
}