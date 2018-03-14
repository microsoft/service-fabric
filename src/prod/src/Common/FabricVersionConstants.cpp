// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

Global<FabricCodeVersion> FabricCodeVersion::Invalid   = make_global<FabricCodeVersion>(0,0,0,0);
Global<FabricCodeVersion> FabricCodeVersion::Default   = make_global<FabricCodeVersion>(1,0,960,0);
Global<FabricCodeVersion> FabricCodeVersion::Baseline  = make_global<FabricCodeVersion>(2,0,0,0);
Global<FabricCodeVersion> FabricCodeVersion::MinCompatible  = make_global<FabricCodeVersion>(1,0,960,0);

Global<FabricConfigVersion> FabricConfigVersion::Invalid = make_global<FabricConfigVersion>();
Global<FabricConfigVersion> FabricConfigVersion::Default = make_global<FabricConfigVersion>(L"__default__");

Global<FabricVersion> FabricVersion::Invalid  = make_global<FabricVersion>(FabricCodeVersion::Invalid, FabricConfigVersion::Invalid);
Global<FabricVersion> FabricVersion::Baseline = make_global<FabricVersion>(FabricCodeVersion::Baseline, FabricConfigVersion::Invalid);
Global<FabricVersion> FabricVersion::Default  = make_global<FabricVersion>(FabricCodeVersion::Default, FabricConfigVersion::Default);

Global<FabricVersionInstance> FabricVersionInstance::Invalid  = make_global<FabricVersionInstance>(FabricVersion::Invalid, 0);
Global<FabricVersionInstance> FabricVersionInstance::Baseline = make_global<FabricVersionInstance>(FabricVersion::Baseline, 0);
Global<FabricVersionInstance> FabricVersionInstance::Default  = make_global<FabricVersionInstance>(FabricVersion::Default, 0);
