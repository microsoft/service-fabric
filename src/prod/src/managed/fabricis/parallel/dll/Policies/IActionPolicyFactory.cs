// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;

    internal interface IActionPolicyFactory
    {
        /// <summary>
        /// Creates a set of policies that could be applied on jobs/repair tasks
        /// </summary>
        IList<IActionPolicy> Create();
    }
}