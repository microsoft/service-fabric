// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT // specifies that the minimum required Windows Platform

// Microsoft Azure Service Fabric will be available on Windows 8 only. However in order to 
// allow testing to progress the minimum supported version is set to Vista
// or W2K8. 
//
// TBD: Change to Windows 8
#define _WIN32_WINNT 0x0600 

#endif
