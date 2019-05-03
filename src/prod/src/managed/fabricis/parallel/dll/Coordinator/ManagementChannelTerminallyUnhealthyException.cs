// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Runtime.Serialization;

    /// <summary>
    /// Exception when the <see cref="AzureParallelInfrastructureCoordinator"/> reports that the channel is unhealthy after sufficient retry.
    /// At this point, exiting the process is the only resort.
    /// </summary>
#if !DotNetCoreClr
    [Serializable]
#endif
    internal class ManagementChannelTerminallyUnhealthyException : ManagementException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementChannelTerminallyUnhealthyException"/> class.
        /// </summary>
        public ManagementChannelTerminallyUnhealthyException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementChannelTerminallyUnhealthyException"/> class.
        /// </summary>
        /// <param name="message">The message that describes the exception.</param>
        public ManagementChannelTerminallyUnhealthyException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Constructor. Initializes a new instance of the <see cref="ManagementChannelTerminallyUnhealthyException"/> class
        /// with the specified error message and optionally, an inner exception. 
        /// </summary>
        /// <param name="message">The message that describes the exception.</param>
        /// <param name="innerException">
        /// The exception that is the cause of the current exception. 
        /// Optional parameter that defaults to null.
        /// </param>
        public ManagementChannelTerminallyUnhealthyException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementChannelTerminallyUnhealthyException"/> class by
        /// deserializing it from a persistent data source.
        /// </summary>
        /// <param name="info">Serialization information object that stores all information regarding serialization.</param>
        /// <param name="context">Object that describes the context for serialization/deserialization.</param>
#if !DotNetCoreClr
        protected ManagementChannelTerminallyUnhealthyException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
#endif
    }
}