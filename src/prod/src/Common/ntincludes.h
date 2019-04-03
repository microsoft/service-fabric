// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#pragma warning(push)
#pragma warning(disable: 4201) // nameless struct/union
#include <ntdef.h>
#include <nt.h>
#include <ntrtl.h>
#include <ntverp.h>
#include <ntrtl_x.h>
#include <nturtl.h>
#include <ntregapi.h>
#pragma warning(pop)

#include <WinSock2.h>
#include <windef.h>

#undef max
#undef min

#include <functional>
#include <exception>
#include <system_error>
#include <limits>
#include <tuple>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <algorithm>
#include <memory>
#include <set>
#include <bitset>
