// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Linq;

    internal class VariantId : IEquatable<VariantId>
    {
        private Variant[] variantArgs;

        private int currentIndex;

        public VariantId(int count)
        {
            this.variantArgs = new Variant[count];
            this.currentIndex = 0;
        }

        /// <summary>
        /// Add element
        /// </summary>
        /// <param name="oneVariant"></param>
        public void AddIdElement(Variant oneVariant)
        {
            if (this.currentIndex >= this.variantArgs.Length)
            {
                throw new InvalidOperationException("No more Variants can be Added");
            }

            this.variantArgs[this.currentIndex++] = oneVariant;
        }

        /// <inheritdoc />
        /// <remark>
        /// The ordering of the individual variant is important and this
        /// equals function takes that into account.
        /// </remark>
        public bool Equals(VariantId other)
        {
            if (other == null)
            {
                return false;
            }

            if (this.variantArgs.Length != other.variantArgs.Length)
            {
                return false;
            }

            for (int i = 0; i < this.variantArgs.Length; ++i)
            {
                if (!this.variantArgs[i].Equals(other.variantArgs[i]))
                {
                    return false;
                }
            }

            return true;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                foreach (Variant one in this.variantArgs)
                {
                    hash = hash * 23 + one.GetHashCode();
                }

                return hash;
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            string str = string.Empty;
            if (this.variantArgs.Length == 0)
            {
                return string.Empty;
            }

            return this.variantArgs.Aggregate(str, (current, one) => current + (one + " ")).Trim();
        }
    }
}