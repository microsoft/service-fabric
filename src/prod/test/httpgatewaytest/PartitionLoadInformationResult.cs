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
    class PartitionLoadInformationResult
    {
        public PartitionLoadInformationResult()
        {
            PrimaryLoadMetricReports = new List<LoadMetricReportResult>();
            SecondaryLoadMetricReports = new List<LoadMetricReportResult>();
        }

        [DataMember(IsRequired = true)]
        public Guid PartitionId
        {
            get;
            set;
        }

        [DataMember(IsRequired = true)]
        public List<LoadMetricReportResult> PrimaryLoadMetricReports { get; set; }

        [DataMember(IsRequired = true)]
        public List<LoadMetricReportResult> SecondaryLoadMetricReports { get; set; }

    }
}