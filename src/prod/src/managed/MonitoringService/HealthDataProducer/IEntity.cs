// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents an entity in the Service Fabric cluster that can be processed to read and consume health data.
    /// </summary>
    public interface IEntity
    {
        /// <summary>
        /// This method implements the core logic of processing health data from an entity.
        /// </summary>
        /// <param name="cancellationToken">CancellationToken instance.</param>
        /// <returns>Sequence of child entities.</returns>
        Task<IEnumerable<IEntity>> ProcessAsync(CancellationToken cancellationToken);
    }
}