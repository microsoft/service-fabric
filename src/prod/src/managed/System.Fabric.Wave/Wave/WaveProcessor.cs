// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Messaging.Stream;
    using Microsoft.ServiceFabric.Replicator;
    using System.Fabric.Store;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    public interface IEchoProcessor
    {
        /// <summary>
        /// Used to invoke echo processing in this wave.
        /// </summary>
        /// <param name="replicatorTransaction">Processing transaction.</param>
        /// <param name="wave">Wave on which the echo is being sent.</param>
        /// <param name="target">Target for the echo.</param>
        /// <param name="WaveFeedback">Wave result from this source.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task OnEchoAsync(Transaction replicatorTransaction, IWave wave, PartitionKey target, WaveFeedback waveFeedback, CancellationToken cancellationToken);
    }

    public interface IWaveProcessor : IEchoProcessor
    {
        /// <summary>
        /// Called when a new wave enters the system.
        /// </summary>
        /// <param name="replicatorTransaction">Processing transaction.</param>
        /// <param name="wave">New wave to process.</param>
        /// <param name="source">Source for the wave.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IEnumerable<PartitionKey>> OnWaveStartedAsync(Transaction replicatorTransaction, IWave wave, PartitionKey source, CancellationToken cancellationToken);

        /// <summary>
        /// Called when an existent wave enters the system.
        /// </summary>
        /// <param name="replicatorTransaction">Processing transaction.</param>
        /// <param name="wave">Existent wave to process.</param>
        /// <param name="source">Source for the wave.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IEnumerable<WaveFeedback>> OnWaveExistentEchoAsync(Transaction replicatorTransaction, IWave wave, PartitionKey source, CancellationToken cancellationToken);

        /// <summary>
        /// Called when the wave enters the completion phase.
        /// </summary>
        /// <param name="replicatorTransaction">Processing transaction.</param>
        /// <param name="wave">Wave to process.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task<IEnumerable<WaveFeedback>> OnWaveCompletedEchoAsync(Transaction replicatorTransaction, IWave wave, CancellationToken cancellationToken);
    }
}