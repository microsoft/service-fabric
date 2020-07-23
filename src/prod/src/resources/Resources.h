// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

extern "C" int LoadStringResource(UINT id, __out_ecount(bufferMax) LPWSTR buffer, int bufferMax);

#include <inc/resourceids.h>

