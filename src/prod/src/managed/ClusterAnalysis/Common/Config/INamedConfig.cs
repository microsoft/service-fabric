// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Config
{
    public interface INamedConfig
    {
        /// <summary>
        /// Name of the Configuration
        /// </summary>
        string Name { get; }
    }
}