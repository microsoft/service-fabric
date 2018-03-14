// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Hosting
        {
            class FindServiceTypeRegistrationErrorCode
            {
            public:
                FindServiceTypeRegistrationErrorCode() :
                isRetryable_(false)
                {
                }

                FindServiceTypeRegistrationErrorCode(Common::ErrorCode && errorCode, bool isRetryable) :
                errorCode_(std::move(errorCode)),
                isRetryable_(isRetryable)
                {
                }

                __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
                Common::ErrorCode const & get_Error() const { return errorCode_; }

                __declspec(property(get = get_IsRetryable)) bool IsRetryable;
                bool get_IsRetryable() const { return isRetryable_; }

                bool IsSuccess() const { return errorCode_.IsSuccess(); }

            private:
                Common::ErrorCode errorCode_;
                bool isRetryable_;
            };
        }
    }
}


