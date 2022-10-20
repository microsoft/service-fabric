// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Threading.Tasks;

    public sealed class OperationProcessorInfo
    {
        /// <summary>
        /// The function that processes operations
        /// </summary>
        public Func<IOperation, Task> Callback { get; set; }

        /// <summary>
        /// If this is true then the Processor function will be called multiple times
        /// </summary>
        public bool SupportsConcurrentProcessing { get; set; }
    }
}