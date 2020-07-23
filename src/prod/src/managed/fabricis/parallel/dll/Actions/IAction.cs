// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    /// <summary>
    /// Wrapper interface for an action that acts on tenant jobs, repair tasks etc.
    /// </summary>
    internal interface IAction
    {
        Task ExecuteAsync(Guid activityId);

        /// <summary>
        /// Just used for simpler serialization/deserialization purpose.
        /// </summary>
        string Name { get; }

        ActionType ActionType { get; }
    }
}