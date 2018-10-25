// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Result;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// Base class for the progress objects.
    /// </summary>
    /// <remarks>
    /// This class returns the TestCommandProgressState
    /// </remarks>
    public abstract class TestCommandProgress
    {
        /// <summary>
        /// This API supports the Service Fabric platform and is not meant to be called from your code
        /// </summary>
        protected internal TestCommandProgress()
        { }

        /// <summary>
        /// Gets the State at which the action is now: 
        /// Running, Completed, Faulted, or Invalid
        /// </summary>
        /// <returns>The state of the test command.</returns>
        public TestCommandProgressState State
        {
            get;
            internal set;
        }

        /// <summary>
        /// Returns the string representation of the State
        /// </summary>
        /// <returns>String representation of the State</returns>
        public override string ToString()
        {
            return string.Format("State: {0}", this.State);
        }
    }
}