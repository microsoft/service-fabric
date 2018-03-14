// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Reliability
{
	namespace ReconfigurationType
	{
		void WriteToTextWriter(TextWriter & w, Enum const& value)
		{
			switch (value)
			{
				case Other:
					w << "Other";
					return;
				case SwapPrimary:
					w << "SwapPrimary";
					return;
				case Failover:
					w << L"Failover";
					return;
                case None:
                    w << L"None";
                    return;
				default:
					Assert::CodingError("unknown value for enum {0}", static_cast<int>(value));
					return;
			}
		}

		ENUM_STRUCTURED_TRACE(ReconfigurationType, Other, LastValidEnum);

		::FABRIC_RECONFIGURATION_TYPE Reliability::ReconfigurationType::ConvertToPublicReconfigurationType(ReconfigurationType::Enum toConvert)
        {
            switch (toConvert)
            {
            case Other:
                return ::FABRIC_RECONFIGURATION_TYPE_OTHER;
            case SwapPrimary:
                return ::FABRIC_RECONFIGURATION_TYPE_SWAPPRIMARY;
            case Failover:
                return ::FABRIC_RECONFIGURATION_TYPE_FAILOVER;
            case None:
                return ::FABRIC_RECONFIGURATION_TYPE_NONE;
            default:
                Common::Assert::CodingError("Unknown Reconfiguration Type");
            }
        }
	}
}
