// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    internal enum ActivityEventType
    {
        Unknown,

        /// <summary>
        /// State changes of an object.
        /// </summary>
        ChangeState,

        /// <summary>
        /// A regular operation. E.g. entering and exiting a method.
        /// </summary>
        Operation,
    }
}