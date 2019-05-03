// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Signals that a wave processing has been faulted.
    /// </summary>
    [Serializable]
    public class WaveFaultedFeedback : WaveFeedback
    {
        #region Instance Members

        /// <summary>
        /// Keeps the excpetion that produces the faulted wave.
        /// </summary>
        private Exception fault;

        #endregion

        public WaveFaultedFeedback(Exception fault)
        {
            this.fault = fault;
        }

        public Exception Fault
        {
            get
            {
                return this.fault;
            }
        }
    }
}