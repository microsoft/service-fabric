// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Communication
        {
            // Holds both the source and the data generated
            // This is a structure so there is not much logic here
            class FMMessageDescription
            {
                DENY_COPY(FMMessageDescription);

            public:
                FMMessageDescription() {}

                FMMessageDescription(Infrastructure::EntityEntryBaseSPtr && source, FMMessageData && msg) :
                Source(std::move(source)),
                Message(std::move(msg))
                {
                }

                FMMessageDescription(FMMessageDescription && other) :
                Source(std::move(other.Source)),
                Message(std::move(other.Message))
                {
                }

                FMMessageDescription & operator=(FMMessageDescription && other)
                {
                    if (this != &other)
                    {
                        Source = std::move(other.Source);
                        Message = std::move(other.Message);
                    }

                    return *this;
                }

                // The partition that generated the message
                Infrastructure::EntityEntryBaseSPtr Source;

                FMMessageData Message;
            };
        }
    }
}



