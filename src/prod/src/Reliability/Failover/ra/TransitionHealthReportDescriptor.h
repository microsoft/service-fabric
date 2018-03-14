// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class TransitionHealthReportDescriptor : public Health::HealthReportDescriptorBase
        {
            DENY_COPY(TransitionHealthReportDescriptor);
            
         public:
            explicit TransitionHealthReportDescriptor(ProxyErrorCode const & proxyErrorCode);
            explicit TransitionHealthReportDescriptor(Common::ErrorCode const & errorCode);
            explicit TransitionHealthReportDescriptor(TransitionErrorCode const & transitionErrorCode);

            virtual ~TransitionHealthReportDescriptor() = default;

            std::wstring GenerateReportDescriptionInternal(Health::HealthContext const & c) const override;

         private:
            ProxyErrorCode proxyErrorCode_;
            Common::ErrorCode errorCode_;
		};
	}
}
