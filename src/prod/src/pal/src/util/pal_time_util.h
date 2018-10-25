// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#define WINDOWS_LINUX_EPOCH_BIAS  116444736000000000ULL

FILETIME UnixTimeToFILETIME(time_t sec, long nsec);
