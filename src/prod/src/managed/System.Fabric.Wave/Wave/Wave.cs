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
    using System.Fabric.Store;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Wave states.
    /// </summary>
    internal enum WaveState
    {
        Invalid,
        Created,
        Started,
        Processing,
    }

    /// <summary>
    /// Encapsulates wave properties and methods.
    /// </summary>
    public interface IWave
    {
        /// <summary>
        /// Identity of the wave.
        /// </summary>
        string Id { get; }

        /// <summary>
        /// Command initiated in this wave.
        /// </summary>
        WaveCommand Command { get; }

        /// <summary>
        /// Command processor for this wave.
        /// </summary>
        IWaveProcessor Processor { get; }

        /// <summary>
        /// Link on which the wave arrived.
        /// </summary>
        PartitionKey InboundLink { get; }

        /// <summary>
        /// Links on which the wave propagated.
        /// </summary>
        IEnumerable<PartitionKey> OutboundLinks { get; }
    }

    [Serializable]
    internal class Wave : DynamicObject, IWave, ICloneable
    {
        #region Instance Members

        /// <summary>
        /// Identity of the wave.
        /// </summary>
        private string identity;

        /// <summary>
        /// Command for this wave.
        /// </summary>
        private WaveCommand command;

        /// <summary>
        /// Link on which the wave arrived or was initiated.
        /// </summary>
        private PartitionKey inboundLink;

        /// <summary>
        /// Outbound feedback stream to reply on.
        /// </summary>
        private Uri outboundFeedbackStreamName;

        /// <summary>
        /// Outbound link information.
        /// </summary>
        private Dictionary<PartitionKey, bool> outboundLinks = new Dictionary<PartitionKey, bool>(new PartitionKeyEqualityComparer());

        /// <summary>
        /// Internal wave state.
        /// </summary>
        private WaveState state = WaveState.Invalid;

        /// <summary>
        /// True on the node where the wave is initiated.
        /// </summary>
        private bool isInitiator;

        /// <summary>
        /// Wave processor for this wave.
        /// </summary>
        [NonSerialized]
        private IWaveProcessor processor;

        #endregion

        /// <summary>
        /// Default Constructor. Used for deserialization.
        /// </summary>
        public Wave()
        {
        }

        /// <summary>
        /// Proper Constructor.
        /// </summary>
        /// <param name="command">Processor for this wave.</param>
        /// <param name="isInitiator">True on the node initiating the wave.</param>
        public Wave(WaveCommand command, bool isInitiator)
        {
            this.command = command;
            this.isInitiator = isInitiator;
            this.identity = Guid.NewGuid().ToString("N");
        }

        #region IWave Members

        public string Id
        {
            get
            {
                return this.identity;
            }

            internal set
            {
                this.identity = value;
            }
        }

        public WaveCommand Command
        {
            get
            {
                return this.command;
            }
        }

        public IWaveProcessor Processor
        {
            get
            {
                return this.processor;
            }

            internal set
            {
                this.processor = value;
            }
        }

        public PartitionKey InboundLink
        {
            get
            {
                return this.inboundLink;
            }
        }

        public IEnumerable<PartitionKey> OutboundLinks
        {
            get
            {
                return this.outboundLinks.Keys;
            }
        }

        #endregion

        /// <summary>
        /// Wave state property.
        /// </summary>
        public WaveState State
        {
            get
            {
                return this.state;
            }

            set
            {
                this.state = value;
            }
        }

        public bool IsInitiator
        {
            get
            {
                return this.isInitiator;
            }
        }

        public Uri OutboundFeedbackStreamName
        {
            get
            {
                return this.outboundFeedbackStreamName;
            }
        }

        /// <summary>
        /// True, if all echoes have returned for this wave.
        /// </summary>
        public bool AllEchoesReceived
        {
            get
            {
                return this.outboundLinks.Values.All(x => x);
            }
        }

        /// <summary>
        /// Sets the processor for this wave.
        /// </summary>
        /// <param name="processor"></param>
        public void SetProcessor(IWaveProcessor processor)
        {
            // Check arguments.
            if (null == processor)
            {
                throw new ArgumentNullException("processor");
            }

            this.processor = processor;
        }

        /// <summary>
        /// Sets the inbound link for this wave.
        /// </summary>
        /// <param name="inboundLink"></param>
        public void SetInboundLink(PartitionKey inboundLink)
        {
            this.inboundLink = inboundLink;
        }

        /// <summary>
        /// Sets the outbound link for this wave on which feedback is to be sent.
        /// </summary>
        /// <param name="inboundLink"></param>
        public void SetOutboundFeedbackStream(Uri outboundStreamName)
        {
            this.outboundFeedbackStreamName = outboundStreamName;
        }

        /// <summary>
        /// Need to figure out what links we need to propagate the wave to.
        /// </summary>
        /// <param name="outboundLinks"></param>
        public void SetOutboundLinks(IEnumerable<PartitionKey> outboundLinks)
        {
            if (null == outboundLinks)
            {
                this.outboundLinks = null;
            }
            else
            {
                this.outboundLinks.Clear();
                foreach (PartitionKey x in outboundLinks)
                {
                    this.outboundLinks.Add(x, false);
                }
            }
        }

        /// <summary>
        /// Echoes was received from a source.
        /// </summary>
        /// <param name="source"></param>
        public void SetFeedbackReceived(PartitionKey source)
        {
            Diagnostics.Assert(this.outboundLinks.ContainsKey(source), "outbound link should exist");
            Diagnostics.Assert(!this.outboundLinks[source], "feedback already received");
            this.outboundLinks[source] = true;
        }

        /// <summary>
        /// Override from object.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format("{0}:{1}:{2}:{3}:{4}", this.identity, this.state, this.isInitiator, null != this.inboundLink ? this.inboundLink.ToString() : "null", null != this.outboundFeedbackStreamName ? this.outboundFeedbackStreamName.OriginalString : "null");
        }

        #region ICloneable Members

        /// <summary>
        /// Deep copy of this wave.
        /// </summary>
        /// <returns></returns>
        public object Clone()
        {
            Wave deepClone = new Wave();
            deepClone.identity = this.identity;
            deepClone.command = this.command;
            deepClone.inboundLink = this.inboundLink;
            deepClone.outboundFeedbackStreamName = this.outboundFeedbackStreamName;
            deepClone.outboundLinks = new Dictionary<PartitionKey, bool>(new PartitionKeyEqualityComparer());
            foreach (KeyValuePair<PartitionKey, bool> x in this.outboundLinks)
            {
                deepClone.outboundLinks.Add(x.Key, x.Value);
            }

            deepClone.state = this.state;
            deepClone.isInitiator = this.isInitiator;
            deepClone.processor = this.processor;
            return deepClone;
        }

        #endregion
    }
}