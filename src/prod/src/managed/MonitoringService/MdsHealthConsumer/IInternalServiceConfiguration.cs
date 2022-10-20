// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Internal
{
    using Microsoft.ServiceFabric.Monitoring.Interfaces;

    internal interface IInternalServiceConfiguration : IServiceConfiguration
    {
        string MdmAccountName { get; }

        string MdmNamespace { get; }

        string DataCenter { get; }
    }
}