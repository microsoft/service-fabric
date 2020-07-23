// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceAddressTracker;
    class FabricServiceAddressChangeHandler
    {
        void AddressChanged(ServiceAddressTracker const & source);
    };
}
