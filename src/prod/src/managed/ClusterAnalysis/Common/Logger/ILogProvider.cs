// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Log
{
    /// <summary>
    /// Contract for log service provider
    /// </summary>
    public interface ILogProvider
    {
        /// <summary>
        /// Create an Instance of Logger
        /// </summary>
        /// <param name="logIdentifier"></param>
        /// <returns></returns>
        ILogger CreateLoggerInstance(string logIdentifier);
    }
}