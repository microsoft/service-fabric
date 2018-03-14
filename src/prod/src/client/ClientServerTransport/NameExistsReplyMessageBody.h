// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NameExistsReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        NameExistsReplyMessageBody() {}

        NameExistsReplyMessageBody(
            bool nameExists,
            UserServiceState::Enum userServiceState)
            : nameExists_(nameExists)
            , userServiceState_(userServiceState)
        {
        }

        __declspec (property(get=get_NameExists)) bool NameExists;
        bool get_NameExists() const { return nameExists_; }

        __declspec(property(get=get_UserServiceState)) UserServiceState::Enum UserServiceState;
        UserServiceState::Enum get_UserServiceState() const { return userServiceState_; }

        FABRIC_FIELDS_02(nameExists_, userServiceState_);

    private:

        bool nameExists_;
        UserServiceState::Enum userServiceState_;
    };
}
