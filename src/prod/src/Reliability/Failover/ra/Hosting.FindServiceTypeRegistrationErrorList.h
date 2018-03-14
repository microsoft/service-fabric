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
            class FindServiceTypeRegistrationErrorList
            {
                DENY_COPY(FindServiceTypeRegistrationErrorList);

            public:
                FindServiceTypeRegistrationErrorList(FindServiceTypeRegistrationErrorList && other);

                FindServiceTypeRegistrationErrorCode TranslateError(Common::ErrorCode && error) const;

                static FindServiceTypeRegistrationErrorList CreateErrorListForNewReplica();

                static FindServiceTypeRegistrationErrorList CreateErrorListForExistingReplica();

            private:
                bool IsRetryableError(Common::ErrorCodeValue::Enum error) const;

                class ErrorType
                {
                public:
                    enum Enum
                    {
                        Fatal = 0,
                        Retryable = 1,
                    };
                };

                FindServiceTypeRegistrationErrorList(
                    std::map<Common::ErrorCodeValue::Enum, ErrorType::Enum> && values,
                    bool isUnknownErrorFatal);

                std::map<Common::ErrorCodeValue::Enum, ErrorType::Enum> const values_;
                bool const isUnknownErrorFatal_;
            };
        }
    }
}



