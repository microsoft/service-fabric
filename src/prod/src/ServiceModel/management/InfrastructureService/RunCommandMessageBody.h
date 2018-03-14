// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace InfrastructureService
    {
        class RunCommandMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            RunCommandMessageBody() : command_() { }

            RunCommandMessageBody(std::wstring const & command) : command_(command) { }

            __declspec(property(get=get_Command)) std::wstring const & Command;

            std::wstring const & get_Command() const { return command_; }

            FABRIC_FIELDS_01(command_);

        private:
            std::wstring command_;
        };
    }
}
