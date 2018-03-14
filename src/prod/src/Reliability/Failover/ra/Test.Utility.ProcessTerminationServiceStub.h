// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class ProcessTerminationServiceStub
            {
            public:
                typedef AsyncApiStub<std::wstring, bool> ApiCallList;

                ProcessTerminationServiceStub() : processTerminationCalls_(L"ProcessTerminationCalls")
                {
                    processTerminationService_ = std::make_shared<Common::ProcessTerminationService>([this](std::wstring const & detail)
                    {
                        auto op = processTerminationCalls_.OnCall(detail, [](Common::AsyncOperationSPtr const&) {}, Common::AsyncOperationSPtr());
                        ApiCallList::End(op);
                    });
                }

                __declspec(property(get = get_ProcessTerminationServiceObj)) Common::ProcessTerminationServiceSPtr const & ProcessTerminationServiceObj;
                Common::ProcessTerminationServiceSPtr const & get_ProcessTerminationServiceObj() const { return processTerminationService_; }

                __declspec(property(get = get_Calls)) ApiCallList & Calls;
                ApiCallList & get_Calls() { return processTerminationCalls_; }

            private:
                ApiCallList processTerminationCalls_;
                Common::ProcessTerminationServiceSPtr processTerminationService_;
            };
        }
    }
}
