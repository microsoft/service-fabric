// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Dynamic;
    using System.Fabric.Data.Common;
    using System.Fabric.Messaging.Stream;
    using Microsoft.ServiceFabric.Replicator;
    using System.Fabric.Store;
    using System.Globalization;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Used to propagate waves within replicator transactions.
    /// </summary>
    internal class WaveLockContext : LockContext
    {
        /// <summary>
        /// Wave controller.
        /// </summary>
        private WaveController waveController;

        /// <summary>
        /// Wave to propagate.
        /// </summary>
        private string waveId;

        /// <summary>
        /// Outboundstreams on which to propagate wave.
        /// </summary>
        private List<IMessageStream> outboundStreams;

        /// <summary>
        /// Replicator transaction enlisted.
        /// </summary>
        private Transaction replicatorTransaction;

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="waveController"></param>
        /// <param name="waveId"></param>
        public WaveLockContext(WaveController waveController, string waveId, List<IMessageStream> outboundStreams, Transaction replicatorTransaction)
        {
            this.waveController = waveController;
            this.waveId = waveId;
            this.outboundStreams = outboundStreams;
            this.replicatorTransaction = replicatorTransaction;
        }

        /// <summary>
        /// Propagates a wave if needed.
        /// </summary>
        public override void Unlock(LockContextMode mode)
        {
            Task.Factory.StartNew(async () => { await this.waveController.ResumeWavePropagation(this.waveId, this.outboundStreams, this.replicatorTransaction); });
        }

        /// <summary>
        /// Used for replicator transaction enlistment.
        /// </summary>
        /// <param name="stateOrKey"></param>
        /// <returns></returns>
        protected override bool IsEqual(object stateOrKey)
        {
            // Should never be called.
            Diagnostics.Assert(false, "should never be called");
            return false;
        }
    }
}