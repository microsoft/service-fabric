// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

typedef void * EventProviderHandle;
typedef void * ProviderMapHandle;
typedef void (*TracePointModuleFunctionPtr)(ProviderMapHandle h, LPCWSTR userData);
typedef void (*TracePointFunctionPtr)(PCEVENT_DESCRIPTOR EventDescriptor, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData, void * context);
typedef GUID ProviderId;
typedef USHORT EventId;

