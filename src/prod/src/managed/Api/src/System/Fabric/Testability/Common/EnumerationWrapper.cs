// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System;
    using System.Fabric;
    using System.Globalization;

    internal class EnumerationWrapper
    {
        private readonly object enumerationResult;

        public EnumerationWrapper(NameEnumerationResult enumerationResult)
        {
            ThrowIf.Null(enumerationResult, "enumerationResult");
            this.enumerationResult = enumerationResult;
        }

        public EnumerationWrapper(PropertyEnumerationResult enumerationResult)
        {
            ThrowIf.Null(enumerationResult, "enumerationResult");

            this.enumerationResult = enumerationResult;
        }

        public TEnumerationResult RetrieveEnumeration<TEnumerationResult>()
        {
            if (!(this.enumerationResult.GetType() == typeof(TEnumerationResult)))
            {
                throw new InvalidCastException(
                    string.Format(
                    CultureInfo.InvariantCulture,
                    "EnumerationResult {0} cannot be converted to type {1}",
                    this.enumerationResult.GetType(), 
                    typeof(TEnumerationResult)));
            }

            return (TEnumerationResult)this.enumerationResult;
        }
    }
}

#pragma warning restore 1591