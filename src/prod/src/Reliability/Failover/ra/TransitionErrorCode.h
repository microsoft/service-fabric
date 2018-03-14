// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class TransitionErrorCode
        {
            DEFAULT_COPY_ASSIGNMENT(TransitionErrorCode);

        public:
            explicit TransitionErrorCode(ProxyErrorCode const & proxyErrorCode);
            explicit TransitionErrorCode(Common::ErrorCode errorCode);

			__declspec(property(get = get_ProxyErrorCode)) ProxyErrorCode const & ProxyEC;
			ProxyErrorCode const & get_ProxyErrorCode() const { return proxyErrorCode_; }

			__declspec(property(get = get_ErrorCode)) Common::ErrorCode const & EC;
			ErrorCode const & get_ErrorCode() const { return errorCode_; }

            bool IsError(Common::ErrorCodeValue::Enum value) const
            {
                return errorCode_.IsError(value);
            }

            bool IsSuccess() const
            {
                return errorCode_.IsSuccess();
            }

            Common::ErrorCodeValue::Enum ReadValue() const
            {
                return errorCode_.ReadValue();
            }

        private:
            ProxyErrorCode proxyErrorCode_;
            Common::ErrorCode errorCode_;
        };
    }
}
