// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// Defines for things specific to this test dll
namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class TestLog
            {
            public:
                static void WriteError(std::wstring const & msg)
                {
                    {
                        Common::AcquireExclusiveLock grab(*lock_);                    
                        BOOST_FAIL(Common::formatString(msg));
                    }
                    
                    Trace.WriteError("Test", "{0}", msg);
                }

                static void WriteInfo(std::wstring const & msg)
                {
                    {
                        Common::AcquireExclusiveLock grab(*lock_);                
                        BOOST_TEST_MESSAGE(Common::formatString(msg));
                    }

                    Trace.WriteInfo("Test", "{0}", msg);
                }

            private:
                static Common::Global<Common::ExclusiveLock> lock_;
            };
        }
    }
}
