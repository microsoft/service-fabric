// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            /*
            
                Defines the number of items to send to the fm
                For now it is a constant value and places a hard limit on the max 
                rate for the fm which is the config entry * the min interval between work
                because fm message sending wont be scheduled faster than that

                There are a few improvements here:
                - if the ra is slow to do the replica up work i.e some ft is locked then
                  the number of items being sent will be reduced significantly 
                  the throttle should have a rate which it needs to maintain
                  and use that to send messages

                - use the service too busy from fm to slow down or speed up the rate
            */
            class FMMessageThrottle : public Infrastructure::IThrottle
            {
            public:
                FMMessageThrottle(FailoverConfig const & cfg) :
                config_(&cfg.MaxNumberOfReplicasInMessageToFMEntry)
                {
                }

                int GetCount(Common::StopwatchTime now) override
                {
                    UNREFERENCED_PARAMETER(now);
                    return config_->GetValue();
                }

                void Update(int count, Common::StopwatchTime now) override
                {
                    UNREFERENCED_PARAMETER(count);
                    UNREFERENCED_PARAMETER(now);
                }

            private:
                IntConfigEntry const * config_;
            };
        }
    }
}


