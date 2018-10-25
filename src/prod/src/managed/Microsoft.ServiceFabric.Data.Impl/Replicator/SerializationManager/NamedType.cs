// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;

    internal class NamedType : IEquatable<NamedType>
    {
        private const string AllNames = "fabricReserved:/AllNames";

        private static Uri allNamesUri = new Uri(AllNames);

        private Uri name;

        private Type type;

        public NamedType(Uri name, Type type)
        {
            if (null == name)
            {
                this.name = allNamesUri;
            }
            else
            {
                this.name = name;
            }

            this.type = type;
        }

        public bool Equals(NamedType other)
        {
            // TODO: Requires test.
            if (0
                != Uri.Compare(
                    this.name,
                    other.name,
                    UriComponents.AbsoluteUri,
                    UriFormat.SafeUnescaped,
                    StringComparison.Ordinal))
            {
                return false;
            }

            return this.type.Equals(other.type);
        }

        public override bool Equals(object obj)
        {
            var namedType = obj as NamedType;

            if (namedType == null)
            {
                // TODO: Consider Assert.
                return false;
            }

            return this.Equals(namedType);
        }

        public override int GetHashCode()
        {
            return this.name.GetHashCode() ^ this.type.GetHashCode();
        }
    }
}