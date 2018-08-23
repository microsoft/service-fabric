// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    /// <summary>
    /// Enums to keep track of Positions.
    /// </summary>
    [Flags]
    internal enum Position
    {
        None = 0,
        Zero = 1,
        One = 2,
        Two = 4,
        Three = 8,
        Four = 16,
        Five = 32,
        Six = 64,
        Seven = 128,
        Eight = 256,
        Nine = 512,
        Ten = 1024,
        Eleven = 2048,
        Twelve = 4096,
        Thirteen = 8192
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
    internal class ProvisionalMetadataAttribute : Attribute
    {
        public ProvisionalMetadataAttribute()
        {
            this.FlushAndClose = false;
        }

        /// <summary>
        /// Gets or Sets the position of elements from the argument list that together form a unique ID.
        /// </summary>
        public Position PositionOfIdElements { get; set; }

        /// <summary>
        /// Gets or Sets the provisional time in MS. The Trace is flushed if there is no update to the ID within this duration.
        /// </summary>
        public uint ProvisionalTimeInMs { get; set; }

        /// <summary>
        /// Gets or Sets the position of elements from the argument list that are not part of the final trace.
        /// </summary>
        /// <remarks>
        /// This comes in handy where you may want to pass an additional parameter to be used as ID but doesn't want it traced.
        /// In future we should really remove this. The real fix here is to have better story for ID which today is a string but 
        /// doesn't have to be that way.
        /// </remarks>
        public Position PositionToExcludeFromTrace { get; set; }

        /// <summary>
        /// Gets or sets the value which dictate that it is the end of the chain and this trace should be flushed
        /// immediately. Any earlier traces which are still in cache and part of this chain are discarded.
        /// </summary>
        public bool FlushAndClose { get; set; }
    }
}