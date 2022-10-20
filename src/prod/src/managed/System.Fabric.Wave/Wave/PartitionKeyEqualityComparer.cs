// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Messaging.Stream;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Compares two partition keys.
    /// </summary>
    [Serializable]
    public class PartitionKeyEqualityComparer : IEqualityComparer<PartitionKey>
    {
        public bool Equals(PartitionKey x, PartitionKey y)
        {
            if (null == x)
            {
                if (null == y)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if (null == y)
                {
                    return false;
                }
                else
                {
                    if (x.Kind == y.Kind && 0 == Uri.Compare(x.ServiceInstanceName, y.ServiceInstanceName, UriComponents.AbsoluteUri, UriFormat.UriEscaped, StringComparison.Ordinal))
                    {
                        if (PartitionKind.Numbered == x.Kind)
                        {
                            return x.PartitionRange.IntegerKeyLow == y.PartitionRange.IntegerKeyLow && x.PartitionRange.IntegerKeyHigh == y.PartitionRange.IntegerKeyHigh;
                        }

                        if (PartitionKind.Named == x.Kind)
                        {
                            return 0 == string.Compare(x.PartitionName, y.PartitionName, false);
                        }

                        return true;
                    }

                    return false;
                }
            }
        }

        public int GetHashCode(PartitionKey obj)
        {
            return obj.GetHashCode();
        }
    }
}