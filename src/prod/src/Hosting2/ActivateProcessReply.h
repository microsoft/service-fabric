// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ActivateProcessReply : public Serialization::FabricSerializable
    {
    public:
        ActivateProcessReply();
        ActivateProcessReply(
            std::wstring const & appServiceId,
            DWORD processId,
            Common::ErrorCode error,
            std::wstring const & errorMessage);
            

        __declspec(property(get=get_ProcessId)) DWORD ProcessId;
        DWORD get_ProcessId() const { return processId_; }

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

         __declspec(property(get=get_ApplicationServiceId)) std::wstring const & ApplicationServiceId;
        std::wstring const & get_AppServiceId() const { return appServiceId_; }

        __declspec(property(get = get_ErrorMessage)) std::wstring const & ErrorMessage;
        std::wstring const & get_ErrorMessage() const { return errorMessage_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(processId_, error_, errorMessage_);

    private:
        std::wstring appServiceId_;
        DWORD processId_;
        Common::ErrorCode error_;
        std::wstring errorMessage_;
    };
}
