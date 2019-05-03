// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __COMPATIBILITYFLAGS_H__
#define __COMPATIBILITYFLAGS_H__

enum CompatibilityFlag {
#define COMPATFLAGDEF(name) compat##name,
#include "compatibilityflagsdef.h"
    compatCount,
};

#endif // __COMPATIBILITYFLAGS_H__
