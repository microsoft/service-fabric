// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class DeactivateNodeMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        DeactivateNodeMessageBody()
        {
        }

        explicit DeactivateNodeMessageBody(std::wstring const& nodeName, FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent)
            : nodeName_(nodeName), deactivationIntent_(deactivationIntent)
        {
        }

        __declspec (property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring get_NodeName() const { return nodeName_; }

        __declspec (property(get=get_DeactivationIntent)) FABRIC_NODE_DEACTIVATION_INTENT DeactivationIntent;
        FABRIC_NODE_DEACTIVATION_INTENT get_DeactivationIntent() const { return deactivationIntent_; }

        FABRIC_FIELDS_02(nodeName_, deactivationIntent_);

    private:

        std::wstring nodeName_;
        FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent_;
    };
}
