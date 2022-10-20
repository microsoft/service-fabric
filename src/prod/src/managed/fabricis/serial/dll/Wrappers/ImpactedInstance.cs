// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using Globalization;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;
    using IImpactDetail = Microsoft.WindowsAzure.ServiceRuntime.Management.Public.IImpactDetail;

    internal sealed class ImpactedInstance
    {
        public ImpactedInstance(string id, IEnumerable<ImpactReason> impactReasons)
            : this(id, impactReasons, null)
        {
        }

        public ImpactedInstance(string id, IEnumerable<ImpactReason> impactReasons, IImpactDetail impactDetail)
        {
            this.Id = id;
            this.ImpactReasons = impactReasons;
            this.ImpactDetail = impactDetail;
        }

        public string Id { get; private set; }
        public IEnumerable<ImpactReason> ImpactReasons { get; private set; }
        public IImpactDetail ImpactDetail { get; private set; }

        public override string ToString()
        {
            string result = String.Format(CultureInfo.InvariantCulture, "{0}[{1}]", this.Id, String.Join(",", this.ImpactReasons));

            if (this.ImpactDetail != null)
            {
                result += String.Format(CultureInfo.InvariantCulture,
                    "[Compute={0},Disk={1},Network={2},OS={3},Duration={4}]",
                    this.ImpactDetail.ComputeImpact,
                    this.ImpactDetail.DiskImpact,
                    this.ImpactDetail.NetworkImpact,
                    this.ImpactDetail.OsImpact,
                    this.ImpactDetail.EstimatedImpactDurationInSeconds);
            }

            return result;
        }
    }
}