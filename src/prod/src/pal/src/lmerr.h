// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NERR_Success            0       /* Success */

#define NERR_BASE       2100

#define NERR_GroupNotFound      (NERR_BASE+120) /* The group name could not be found. */
#define NERR_UserNotFound       (NERR_BASE+121) /* The user name could not be found. */
#define NERR_GroupExists        (NERR_BASE+123) /* The group already exists. */
#define NERR_UserExists         (NERR_BASE+124) /* The account already exists. */

#ifdef __cplusplus
}
#endif
