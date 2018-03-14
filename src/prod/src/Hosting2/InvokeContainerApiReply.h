// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class InvokeContainerApiReply : public Serialization::FabricSerializable
    {
    public:
        InvokeContainerApiReply();
        InvokeContainerApiReply(
            Common::ErrorCode const & error,
            std::wstring const & errorMessage,
            ContainerApiExecutionResponse const & apiExecResponse);

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_ErrorMessage)) std::wstring const & ErrorMessage;
        std::wstring const & get_ErrorMessage() const { return errorMessage_; }

        __declspec(property(get = get_ApiExecResponse)) ContainerApiExecutionResponse const & ApiExecResponse;
        ContainerApiExecutionResponse const & get_ApiExecResponse() const { return apiExecResponse_; }

        ContainerApiExecutionResponse && TakeApiExecResponse()
        {
            return move(apiExecResponse_);
        }

        std::wstring && TakeErrorMessage()
        {
            return move(errorMessage_);
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(error_, errorMessage_, apiExecResponse_);

    private:
        Common::ErrorCode error_;
        std::wstring errorMessage_;
        ContainerApiExecutionResponse apiExecResponse_;
    };
}
