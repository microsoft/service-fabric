// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    internal abstract class ActionPolicy : IActionPolicy
    {
        public abstract Task ApplyAsync(Guid activityId, CoordinatorContext coordinatorContext);

        public virtual void Reset()
        {            
        }
    }
}