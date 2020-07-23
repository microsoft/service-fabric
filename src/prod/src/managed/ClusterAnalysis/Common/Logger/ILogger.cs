// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Log
{
    /// <summary>
    /// Simple logger Interface
    /// </summary>
    public interface ILogger
    {
        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="format"></param>
        /// <param name="message"></param>
        void LogMessage(string format, params object[] message);

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="format"></param>
        /// <param name="message"></param>
        void LogWarning(string format, params object[] message);

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="format"></param>
        /// <param name="message"></param>
        void LogError(string format, params object[] message);

        /// <summary>
        /// Flush logs
        /// </summary>
        void Flush();
    }
}