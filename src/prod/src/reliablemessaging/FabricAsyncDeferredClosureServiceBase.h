// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	class FabricAsyncDeferredClosureServiceBase : public Ktl::Com::FabricAsyncServiceBase
	{
	public:
		FabricAsyncDeferredClosureServiceBase()
		{
			SetDeferredCloseBehavior();
		}
	};
}
