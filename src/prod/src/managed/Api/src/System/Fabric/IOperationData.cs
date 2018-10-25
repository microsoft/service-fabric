// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;

    /// <summary>
    /// <para>Interface for the data being transferred.</para>
    /// </summary>
    public interface IOperationData : IEnumerable<ArraySegment<byte>>
    {
    }
}