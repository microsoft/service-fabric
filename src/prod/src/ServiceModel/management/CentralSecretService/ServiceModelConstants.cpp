// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

using namespace Management::CentralSecretService;

wstring const ServiceModelConstants::SecretResourceNameSeparator(L"::");
wstring const ServiceModelConstants::SecretResourceNameFormat(L"{0}" + ServiceModelConstants::SecretResourceNameSeparator + L"{1}");