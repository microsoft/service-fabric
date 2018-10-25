// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.Collections.Generic;

    // This interface needs to be implemented by all DCA producers
#if DotNetCoreClrLinux
    public interface IDcaProducer : IDisposable
#else
    internal interface IDcaProducer : IDisposable
#endif
    {
        // Additional sections in the cluster manifest or application manifest from which
        // the producer reads configuration information. The producer's own section in 
        // the cluster manifest or application manifest need not be included here.
        IEnumerable<string> AdditionalAppConfigSections { get; }

        // Sections in the service manifest from which the producer reads configuration
        // information. 
        IEnumerable<string> ServiceConfigSections { get; }

        // Finish processing all pending data
        void FlushData();
    }
}