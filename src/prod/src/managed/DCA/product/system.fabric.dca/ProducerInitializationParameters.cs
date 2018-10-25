// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    internal struct ProducerInitializationParameters
    {
        // Application instance ID of the application for which we are collecting data
        internal string ApplicationInstanceId;

        // Name of the section containing the producer's settings in the cluster manifest 
        internal string SectionName;

        // Full path to the Windows Fabric log directory
        internal string LogDirectory;

        // Full path to the Windows Fabric work directory
        internal string WorkDirectory;

        // List of consumer sinks to which data needs to be supplied
        internal IEnumerable<object> ConsumerSinks;
    }
}