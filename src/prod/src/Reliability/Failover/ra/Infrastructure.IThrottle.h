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
            // Represents a time based throttle
            class IThrottle
            {
            public:
                
                // Return the number of elements that would cause the throttle to be hit
                virtual int GetCount(Common::StopwatchTime now) = 0;

                // Tell the throttle that 'count' elements happened at a particular time 
                virtual void Update(int count, Common::StopwatchTime now) = 0;

                virtual ~IThrottle() {}
            };
        }
    }
}



