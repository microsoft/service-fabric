// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
	namespace HostType
	{
		void WriteToTextWriter(TextWriter & w, Enum const & val)
		{
			switch (val)
			{
			case Invalid: w << "Invalid"; return;
			case ExeHost: w << "ExeHost"; return;
			case ContainerHost: w << "ContainerHost"; return;
			default: w << "HostType(" << static_cast<ULONG>(val) << ')'; return;
			}
		}

		FABRIC_HOST_TYPE ToPublicApi(Enum const & val)
		{
			switch (val)
			{
			case Invalid:
				return FABRIC_HOST_TYPE_INVALID;
			case ExeHost:
				return FABRIC_HOST_TYPE_EXE_HOST;
			case ContainerHost:
				return FABRIC_HOST_TYPE_CONTAINER_HOST;
			default:
				Common::Assert::CodingError("Unknown HostType.");
			}
		}

		ErrorCode FromPublicApi(FABRIC_HOST_TYPE const & publicVal, __out Enum & val)
		{
			switch (publicVal)
			{
			case FABRIC_HOST_TYPE_INVALID:
				val = Invalid;
				return ErrorCodeValue::Success;
			case FABRIC_HOST_TYPE_EXE_HOST:
				val = ExeHost;
				return ErrorCodeValue::Success;
			case FABRIC_HOST_TYPE_CONTAINER_HOST:
				val = ContainerHost;
				return ErrorCodeValue::Success;
			default:
				return ErrorCodeValue::InvalidArgument;
			}
		}

        Enum FromEntryPointType(EntryPointType::Enum const & entryPointType)
        {
            switch (entryPointType)
            {
            case EntryPointType::Exe:
                return ExeHost;
            case EntryPointType::ContainerHost:
                return ContainerHost;
            default:
                return Invalid;
            }
        }

		ENUM_STRUCTURED_TRACE(HostType, FirstValidEnum, LastValidEnum)
	}
}

