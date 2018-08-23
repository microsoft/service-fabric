// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    /// <summary>
    /// Specifies if items returned during enumeration of reliable collections should be 
    /// unordered or ordered.
    /// </summary>
    public enum EnumerationMode : int
    {
        /// <summary>
        /// Results are returned as an unordered enumeration. Fastest enumeration mode.
        /// </summary>
        Unordered = 0,

        /// <summary>
        /// Results are be returned as an ordered enumeration.
        /// </summary>
        Ordered = 1,
    }
}