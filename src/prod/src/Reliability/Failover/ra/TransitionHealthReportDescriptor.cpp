// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;

TransitionHealthReportDescriptor::TransitionHealthReportDescriptor(ProxyErrorCode const & proxyErrorCode) : proxyErrorCode_(proxyErrorCode), errorCode_(proxyErrorCode_.ErrorCode)
{
}

TransitionHealthReportDescriptor::TransitionHealthReportDescriptor(ErrorCode const & errorCode) : proxyErrorCode_(), errorCode_(errorCode)
{
}

TransitionHealthReportDescriptor::TransitionHealthReportDescriptor(TransitionErrorCode const & transitionErrorCode) : proxyErrorCode_(transitionErrorCode.ProxyEC), errorCode_(transitionErrorCode.EC)
{
}

std::wstring TransitionHealthReportDescriptor::GenerateReportDescriptionInternal(Health::HealthContext const & c) const
{
	// If it's not an API failure
	if (proxyErrorCode_.IsError(ErrorCodeValue::Enum::Success))
	{
		if(errorCode_.IsError(ErrorCodeValue::Enum::ApplicationHostCrash))
		{
			return wformatString(" {0}. The application host has crashed.", c.NodeName);
		}
		else
		{
			return wformatString(" {0}. Launching application host has failed.", c.NodeName);

		}
	}

	// Following is for API failure case
	TESTASSERT_IF(proxyErrorCode_.Api.IsInvalid, "ApiNameDescription {0} is Invalid.", proxyErrorCode_.Api);

	// RAP sends the health report as part of the string
	// If it is native code then RAP will not send any string so RA should send the HRESULT
	if (proxyErrorCode_.ErrorCode.Message.empty())
	{
		return wformatString(" {0}. {1}", c.NodeName, proxyErrorCode_.ErrorCode.ToHResult());
	}
	else
	{
		return wformatString(" {0}. API call: {1}; Error = {2}", c.NodeName, proxyErrorCode_.Api.ToString(), proxyErrorCode_.ErrorCode.Message);
	}
}

