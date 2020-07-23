// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Management;
using namespace BackupRestoreAgentComponent;

Global<BAEventSource> BAEventSource::Events = make_global<BAEventSource>();
