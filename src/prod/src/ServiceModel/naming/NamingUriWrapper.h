// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NamingUriWrapper
    {
    public:
        NamingUriWrapper() {};
        NamingUriWrapper(std::wstring const &uri);
        ~NamingUriWrapper() {};

        __declspec(property(get=get_UriString)) std::wstring const &UriString;
        std::wstring const& get_UriString() const { return uriString_; }

        Common::ErrorCode FromPublicApi(
            __in FABRIC_URI const & uriString);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_URI & serviceDescription) const;

    private:
        std::wstring uriString_;
    };
}
