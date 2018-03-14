// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class EntityAssertContext
            {
            public:
                EntityAssertContext(IEntityStateBase const * state) :
                    state_(state)
                {
                }

                // Fail if the condition is hit
                // Indicates an unexpected unhandlable condition and the calling code should not continue
                template<typename T>
                void FailIf(bool condition, Common::StringLiteral const & msg, T const & param) const
                {
                    if (condition)
                    {
                        Fail(msg, param);
                    }
                }

                // Fail if the condition is hit
                // Indicates an unexpected unhandlable condition and the calling code should not continue
                void FailIf(bool condition, Common::StringLiteral const & msg) const
                {
                    if (condition)
                    {
                        Fail(msg);
                    }
                }

                // Fails
                // Indicates an unexpected unhandlable condition and the calling code should not continue
                template<typename T>
                void Fail(Common::StringLiteral const & msg, T const & param) const
                {
                    Common::Assert::TestAssert("{0}", GetMessage(msg, param));
                }

                // Fails
                // Indicates an unexpected unhandlable condition and the calling code should not continue
                void Fail(Common::StringLiteral const & msg) const
                {
                    Common::Assert::TestAssert("{0}", GetMessage(msg));
                }

            private:
                template <typename T>
                std::wstring GetMessage(Common::StringLiteral const & msg, T const & param) const
                {
                    return Common::wformatString("{0}. Param = {1}\r\nState = {2}", msg, param, GetStateMessage());
                }

                std::wstring GetMessage(Common::StringLiteral const & msg) const
                {
                    return Common::wformatString("{0}. Param = \r\nState = {1}", msg, GetStateMessage());
                }

                std::wstring GetStateMessage() const
                {
                    if (state_ == nullptr)
                    {
                        return L""; 
                    }
                    else
                    {
                        return state_->ToString();
                    }
                }

                IEntityStateBase const * state_;
            };
        }
    }
}
