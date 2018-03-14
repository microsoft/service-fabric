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
            class EntityJobItemTraceInformation
            {
            public:
                static EntityJobItemTraceInformation CreateWithName(
                    std::wstring const & activityId,
                    JobItemName::Enum name);

                static EntityJobItemTraceInformation CreateWithMetadata(
                    std::wstring const & activityId,
                    std::wstring const & traceMetadata);

                void WriteToEtw(uint16 contextSequenceId) const;
                void WriteTo(Common::TextWriter & w, Common::FormatOptions const & format) const;
            private:
                EntityJobItemTraceInformation(
                    std::wstring const * activityId,
                    std::wstring const * traceMetadata,
                    JobItemName::Enum name);

                std::wstring const * activityId_;
                std::wstring const * traceMetadata_;
                JobItemName::Enum name_;
            };
        }
    }
}

