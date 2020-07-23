// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common
{
    using System;

    public interface IHasUniqueIdentity
    {
        /// <summary>
        /// Get Unique Identity
        /// </summary>
        /// <returns></returns>
        Guid GetUniqueIdentity();
    }
}
