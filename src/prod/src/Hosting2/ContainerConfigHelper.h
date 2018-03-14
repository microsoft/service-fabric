// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerConfigHelper
    {
    public:
        static Common::ErrorCode DecryptValue(std::wstring const& encryptedValue, __out std::wstring & decryptedValue);
        static Common::ErrorCode ParseDriverOptions(std::vector<ServiceModel::DriverOptionDescription> const& driverOpts, __out std::map<wstring, wstring> & logDriverOpts);
    };
}
