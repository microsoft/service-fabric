// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostedServiceMap
    {
        DENY_COPY(HostedServiceMap)

    public:
        typedef std::function<void(HostedServiceSPtr &)> Operation;

        HostedServiceMap();
        ~HostedServiceMap();

        Common::ErrorCode Add(HostedServiceSPtr const & hostedService); 
        Common::ErrorCode Get(std::wstring const &, __out HostedServiceSPtr & hostedService);
        Common::ErrorCode Contains(std::wstring const &, __out bool & contains);
        Common::ErrorCode Remove(std::wstring const &);
        Common::ErrorCode Perform(HostedServiceMap::Operation const & visitor, std::wstring key);

        std::vector<HostedServiceSPtr> Close();



    private:
        Common::RwLock lock_;
        std::map<std::wstring, HostedServiceSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> map_;
        bool isClosed_;
    };
}
