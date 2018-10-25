// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Equality comparer for Uri which uses absolute Uri comparison, in contrast to default equality comparison (which e.g. does 
    /// not compare the fragment portion of the Uri)
    /// </summary>
    internal class AbsoluteUriEqualityComparer : IEqualityComparer<Uri>
    {
        private static readonly AbsoluteUriEqualityComparer PrivateComparer = new AbsoluteUriEqualityComparer();

        private AbsoluteUriEqualityComparer()
        {
        }

        public static AbsoluteUriEqualityComparer Comparer
        {
            get { return PrivateComparer; }
        }

        public bool Equals(Uri x, Uri y)
        {
            return
                Uri.Compare(
                    x,
                    y,
                    UriComponents.AbsoluteUri,
                    UriFormat.SafeUnescaped,
                    StringComparison.Ordinal) == 0;
        }

        public int GetHashCode(Uri obj)
        {
            return obj == null ? 0 : obj.GetHashCode();
        }
    }
}