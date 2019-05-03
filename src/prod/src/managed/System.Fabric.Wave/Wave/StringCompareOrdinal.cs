// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Compares two strings using ordinal comarison.
    /// </summary>
    internal class StringComparerOrdinal : IComparer<string>, IEqualityComparer<string>
    {
        public int Compare(string x, string y)
        {
            return string.Compare(x, y, StringComparison.Ordinal);
        }

        public bool Equals(string x, string y)
        {
            return string.Equals(x, y, StringComparison.Ordinal);
        }

        public int GetHashCode(string obj)
        {
            return obj.GetHashCode();
        }
    }
}