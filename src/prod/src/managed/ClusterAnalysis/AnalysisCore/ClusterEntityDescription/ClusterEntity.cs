// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common.Util;

    [DataContract]
    public class ClusterEntity : BaseEntity
    {
        public ClusterEntity(string name)
        {
            Assert.IsNotEmptyOrNull(name, "Cluster Name can't be null/empty");
            this.ClusterName = name;
        }

        [DataMember]
        public string ClusterName { get; private set; }

        public override bool Equals(BaseEntity other)
        {
            if (other == null)
            {
                return false;
            }

            ClusterEntity otherObj = other as ClusterEntity;
            if (otherObj == null)
            {
                return false;
            }

            return this.ClusterName == otherObj.ClusterName;
        }

        public override int GetUniqueIdentity()
        {
            return this.ClusterName.GetHashCode();
        }
    }
}