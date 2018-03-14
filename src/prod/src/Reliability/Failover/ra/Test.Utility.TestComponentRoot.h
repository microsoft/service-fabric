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
            class TestComponentRoot : public Common::ComponentRoot
            {
                DENY_COPY(TestComponentRoot);

            public:
                TestComponentRoot(UnitTestContext & utContext) : utContext_(utContext)
                {
                }

                __declspec(property(get = get_UTContext)) UnitTestContext & UTContext;
                UnitTestContext & get_UTContext() const
                {
                    return utContext_;
                }

            private:
                UnitTestContext & utContext_;
            };
        }
    }
}
