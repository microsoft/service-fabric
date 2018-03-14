// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Reliability
{
	namespace ReconfigurationResult
	{
		void WriteToTextWriter(TextWriter & w, Enum const& value)
		{
			switch (value)
			{
                case Invalid:
                    w << "Invalid";
                    return;
				case Completed:
					w << "Completed";
					return;
				case AbortSwapPrimary:
					w << "AbortSwapPrimary";
					return;
				case ChangeConfiguration:
					w << L"ChangeConfiguration";
					return;
				case Aborted:
					w << L"Aborted";
					return;
                case DemoteCompleted:
                    w << L"DemoteCompleted";
                    return;
				default:
					Assert::CodingError("unknown value for enum {0}", static_cast<int>(value));
					return;
			}
		}

		ENUM_STRUCTURED_TRACE(ReconfigurationResult, Completed, LastValidEnum);
	}
}
