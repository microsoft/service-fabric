// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Health
        {
            class HealthReportDescriptorBase
            {
                DENY_COPY(HealthReportDescriptorBase);

            public:
                virtual ~HealthReportDescriptorBase() {}                
                
                std::wstring GenerateReportDescription(HealthContext const & c) const
                {
                    // todo: localization - all the health reports are broken
                    auto desc = GenerateReportDescriptionInternal(c);

                    // if the health report is empty then just add the description text
                    // if the health report is not empty then we want one blank line 
                    // between the health report text and the description
                    // depending on the terminating character add the appropriate new lines
                    if (!desc.empty())
                    {
                        desc += L"\r\n";
                        if (*desc.rbegin() != L'\n')
                        {
                            desc += L"\r\n";
                        }
                    }

                    desc += L"For more information see: http://aka.ms/sfhealth";

                    return desc;
                }

            protected:
                HealthReportDescriptorBase() {};

            private:
                virtual std::wstring GenerateReportDescriptionInternal(HealthContext const & c) const = 0;
            };


            typedef unique_ptr<HealthReportDescriptorBase> IHealthDescriptorUPtr;
            typedef shared_ptr<HealthReportDescriptorBase> IHealthDescriptorSPtr;
        }
    }
}
