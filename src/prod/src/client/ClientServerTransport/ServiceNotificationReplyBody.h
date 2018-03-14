// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceNotificationReplyBody : public ServiceModel::ClientServerMessageBody
    {
        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        ServiceNotificationReplyBody() 
            : error_()
        { 
        }

        ServiceNotificationReplyBody(
            Common::ErrorCode const & error)
            : error_(error)
        {
        }

        __declspec (property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        FABRIC_FIELDS_01(error_);
        
    private:
        Common::ErrorCode error_;
    };
}

