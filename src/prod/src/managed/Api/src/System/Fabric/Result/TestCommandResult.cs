// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    /// <summary>
    /// Base class for the result objects.
    /// </summary>
    /// <remarks>
    /// This class conditionally contains the Exception
    /// </remarks>
    [Serializable]
    public abstract class TestCommandResult
    {
        /// <summary>
        /// The base class for other test command result classes to derive from.  The only property in this class is Exception.
        /// </summary>
        protected TestCommandResult()
        { }

        /// <summary>
        /// This property contains an exception representing the reason a test command faulted.  It is not valid unless the corresponding TestCommandProgressState is Faulted. 
        /// </summary>
        /// <value>The Exception object.</value>
        public Exception Exception
        {
            get;
            internal set;
        }
    }
}