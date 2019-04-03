// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

#define DRIVER_NAME       "KtlLogger"

#ifdef __cplusplus
extern "C"
{
NTSTATUS KtlLogMmProbeAndLockPagesNoException(
    __in PMDL Mdl,
    __in KPROCESSOR_MODE AccessMode,
    __in LOCK_OPERATION Operation
    );
};
#endif
