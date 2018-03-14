// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral TraceType_ContainerConfigHelper = "ContainerConfigHelper";

ErrorCode ContainerConfigHelper::DecryptValue(std::wstring const& encryptedValue, __out std::wstring & decryptedValue)
{
    SecureString decryptedString;
    auto error = CryptoUtility::DecryptText(encryptedValue, decryptedString);

    if (error.IsSuccess())
    {
        decryptedValue = decryptedString.GetPlaintext();
    }

    return error;
}

ErrorCode ContainerConfigHelper::ParseDriverOptions(std::vector<DriverOptionDescription> const& driverOpts, __out map<wstring, wstring> & logDriverOpts)
{
    ErrorCode error(ErrorCodeValue::Success);

    for (auto iter = driverOpts.begin(); iter != driverOpts.end(); ++iter)
    {
        wstring decryptedValue = iter->Value;

        if (StringUtility::AreEqualCaseInsensitive(iter->IsEncrypted, L"true"))
        {
            error = DecryptValue(iter->Value, decryptedValue);
            if (!error.IsSuccess())
            {
                TraceError(
                    TraceTaskCodes::Hosting,
                    TraceType_ContainerConfigHelper,
                    "Unable to decrypt the value for the Driver Option Name:{0} error:{1}",
                    iter->Name,
                    error);

                return ErrorCode(error.ReadValue(),
                    wformatString(StringResource::Get(IDS_HOSTING_DecryptDriverOptionFailed), iter->Name, error.ErrorCodeValueToString()));
            }
        }
        logDriverOpts[iter->Name] = move(decryptedValue);
    }

    return error;
}
