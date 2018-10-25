// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Tracer that is used for logging information, errors.
    /// </summary>
    internal interface ITracer
    {
        string Type { get; }

        /// <summary>
        /// Logs error 
        /// </summary>
        void WriteError(string type, string format, params object[] args);

        /// <summary>
        /// Logs information.
        /// </summary>
        void WriteInfo(string type, string format, params object[] args);

        /// <summary>
        /// Logs verbose information.
        /// </summary>
        void WriteNoise(string type, string format, params object[] args);

        /// <summary>
        /// Logs warning.
        /// </summary>
        void WriteWarning(string type, string format, params object[] args);
    }
}