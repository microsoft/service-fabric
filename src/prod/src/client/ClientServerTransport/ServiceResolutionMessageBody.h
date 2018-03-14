// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceResolutionMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        ServiceResolutionMessageBody()
            : name_()
            , request_()
            , bypassCache_(false)
        {
        }

        ServiceResolutionMessageBody(
            Common::NamingUri const & name,
            ServiceResolutionRequestData const & request)
          : name_(name)
          , request_(request)
          , bypassCache_(false)
        {
        }

        ServiceResolutionMessageBody(
            Common::NamingUri const & name,
            ServiceResolutionRequestData const & request,
            bool bypassCache)
          : name_(name)
          , request_(request)
          , bypassCache_(bypassCache)
        {
        }
        
        
        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_Request)) ServiceResolutionRequestData const & Request;
        ServiceResolutionRequestData const & get_Request() const { return request_; }

        __declspec(property(get=get_BypassCache)) bool BypassCache;
        bool get_BypassCache() const { return bypassCache_; }

        FABRIC_FIELDS_03(name_, request_, bypassCache_);

    private:
        Common::NamingUri name_;
        ServiceResolutionRequestData request_;
        bool bypassCache_;
    };
}
