// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Dynamic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    
    /// <summary>
    /// Base class for wave feedback.
    /// </summary>
    [Serializable]
    public abstract class WaveFeedback : DynamicObject
    {
        /// <summary>
        /// Used for deserialization.
        /// </summary>
        public WaveFeedback()
        {
        }

        /// <summary>
        /// The wave id for this wave result.
        /// </summary>
        internal string WaveId { get; set; }
    }
}