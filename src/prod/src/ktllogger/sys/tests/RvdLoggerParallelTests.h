// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef KMUNIT_FRAMEWORK
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif
#endif

NTSTATUS
ParallelLogTest();

