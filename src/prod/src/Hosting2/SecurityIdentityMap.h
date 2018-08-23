// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    template <class T> class SecurityIdentityMap
    {
        DENY_COPY(SecurityIdentityMap)

    public:
        SecurityIdentityMap<T>();
        ~SecurityIdentityMap<T>();

        Common::ErrorCode Add(
            std::wstring const & objectId,
            T const & obj);

        Common::ErrorCode Get(std::wstring const &, __out T & obj);
        Common::ErrorCode Contains(std::wstring const &, __out bool & contains);
        Common::ErrorCode Remove(std::wstring const &, __out T & obj);
        std::vector<T> GetAll();

        std::vector<T> Close();

    private:
        Common::RwLock lock_;
        std::map<std::wstring, T, Common::IsLessCaseInsensitiveComparer<std::wstring>> map_;
        bool isClosed_;
    };
}
