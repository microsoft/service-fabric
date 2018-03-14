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
            class ConfigurationManager
            {
            public:
                void InitializeAssertConfig(
                    bool testAssertEnabled);

                void InitializeTraceConfig(
                    int consoleTraceLevel, 
                    int fileTraceLevel, 
                    int etwTraceLevel);
            };
        }
    }
}



