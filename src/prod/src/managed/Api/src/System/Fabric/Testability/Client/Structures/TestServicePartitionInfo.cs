// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Globalization;
    using System.Text;

    [Serializable]
    public class TestServicePartitionInfo
    {
        public TestServicePartitionInfo()
        {
        }

        public string Location
        {
            get;
            set;
        }

        public bool IsPrimaryEndpoint
        {
            get;
            set;
        }

        public Uri Name
        {
            get;
            set;
        }

        public ServiceEndpointRole Role
        {
            get;
            set;
        }

        public Guid Id
        {
            get;
            set;
        }

        public ServicePartitionKind KeyType
        {
            get;
            set;
        }

        public object RangeHighKey
        {
            get;
            set;
        }

        public object RangeLowKey
        {
            get;
            set;
        }

        public int PartitionIdentifier
        {
            get;
            set;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();

            stringBuilder.AppendFormat(CultureInfo.InvariantCulture, "Name: {0}", this.Name);
            stringBuilder.AppendLine();
            stringBuilder.AppendFormat(CultureInfo.InvariantCulture, "Location: '{0}'", this.Location);
            stringBuilder.AppendLine();
            stringBuilder.AppendFormat(CultureInfo.InvariantCulture, "Id:{0} Role:{1}", this.Id, this.Role);
            stringBuilder.AppendLine();

            return stringBuilder.ToString();
        }
    }
}


#pragma warning restore 1591