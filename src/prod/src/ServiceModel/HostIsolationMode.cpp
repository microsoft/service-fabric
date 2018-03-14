// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
	namespace HostIsolationMode
	{
		void WriteToTextWriter(TextWriter & w, Enum const & val)
		{
			switch (val)
			{
			case None: w << "None"; return;
			case Process: w << "Process"; return;
			case HyperV: w << "HyperV"; return;
			default: w << "HostIsolationMode(" << static_cast<ULONG>(val) << ')'; return;
			}
		}

		FABRIC_HOST_ISOLATION_MODE ToPublicApi(Enum const & val)
		{
			switch (val)
			{
			case None:
				return FABRIC_HOST_ISOLATION_MODE_NONE;
			case Process:
				return FABRIC_HOST_ISOLATION_MODE_PROCESS;
			case HyperV:
				return FABRIC_HOST_ISOLATION_MODE_HYPER_V;
			default:
				Common::Assert::CodingError("Unknown HostIsolationMode.");
			}
		}

		ErrorCode FromPublicApi(FABRIC_HOST_ISOLATION_MODE const & publicVal, __out Enum & val)
		{
			switch (publicVal)
			{
			case FABRIC_HOST_ISOLATION_MODE_NONE:
				val = None;
				return ErrorCodeValue::Success;
			case FABRIC_HOST_ISOLATION_MODE_PROCESS:
				val = Process;
				return ErrorCodeValue::Success;
			case FABRIC_HOST_ISOLATION_MODE_HYPER_V:
				val = HyperV;
				return ErrorCodeValue::Success;
			default:
				return ErrorCodeValue::InvalidArgument;
			}
		}

        Enum FromContainerIsolationMode(ContainerIsolationMode::Enum val)
        {
            switch (val)
            {
            case ContainerIsolationMode::process:
                return HostIsolationMode::Process;
            case ContainerIsolationMode::hyperv:
                return HostIsolationMode::HyperV;
            default:
                Assert::CodingError("Unknown ContainerIsolationMode value {0}", (int)val);
            }
        }

		ENUM_STRUCTURED_TRACE(HostIsolationMode, FirstValidEnum, LastValidEnum)
	}
}

