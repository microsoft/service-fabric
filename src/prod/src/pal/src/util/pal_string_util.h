// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string>

namespace Pal {

std::wstring utf8to16(const char *str);

std::string utf16to8(const wchar_t *wstr);

}
