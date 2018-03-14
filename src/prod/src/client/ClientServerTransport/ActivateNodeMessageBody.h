// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ActivateNodeMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        ActivateNodeMessageBody()
        {
        }

        explicit ActivateNodeMessageBody(std::wstring nodeName)
            : nodeName_(nodeName)
        {
        }

        __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring get_NodeName() const { return nodeName_; }

        FABRIC_FIELDS_01(nodeName_);

    private:

        std::wstring nodeName_;
    };
}
