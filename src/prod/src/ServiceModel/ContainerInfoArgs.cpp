// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

GlobalWString ContainerInfoArgs::Tail = make_global<std::wstring>(L"Tail");
GlobalWString ContainerInfoArgs::Previous = make_global<std::wstring>(L"Previous");