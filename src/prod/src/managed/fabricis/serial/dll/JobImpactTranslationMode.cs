// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Provides the impact of the job after evaluation by the <see cref="IJobImpactManager"/>.
    /// </summary>
    internal enum JobImpactTranslationMode
    {
        /// <summary>
        /// The impact is as published by the notification.
        /// </summary>
        Default,

        /// <summary>
        /// The impact is optimized based on learnings of the <see cref="IJobImpactManager"/>.
        /// </summary>
        Optimized,    
    }
}