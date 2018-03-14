// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        /*
            Wrapper class around the ErrorCode
            RAP <-> RA communication needs the error message to be serialized as well
            However, ErrorCode itself is designed to only serialize the error code value and is non extensible

            On RAP when the object is generated the message from the error code is stored in the message_ field
            
            When the object is deserialized then the message is also stored in the message_ field

            Whenever the ErrorCode is read then the message is taken from the message_ field and put in the error code
            At this point the message_ has been duplicated

            TODO: Optimize the above
        */
        class ProxyErrorCode : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_ASSIGNMENT(ProxyErrorCode)
        
        public:
            ProxyErrorCode() :
                isDeserialized_(false)
            {
            }

            static ProxyErrorCode CreateUserApiError(Common::ErrorCode && errorCode, Common::ApiMonitoring::ApiNameDescription && api)
            {
                ASSERT_IF(api.IsInvalid, "Cannot pass Invalid here");
                return ProxyErrorCode(std::move(errorCode), std::move(api));
            }

            static ProxyErrorCode CreateRAPError(Common::ErrorCode && errorCode)
            {
                return ProxyErrorCode(std::move(errorCode), Common::ApiMonitoring::ApiNameDescription());
            }

            static ProxyErrorCode Create(Common::ErrorCode && errorCode, Common::ApiMonitoring::ApiNameDescription && desc)
            {
                return ProxyErrorCode(std::move(errorCode), std::move(desc));
            }

            static ProxyErrorCode CreateRAPError(Common::ErrorCodeValue::Enum value)
            {
                return ProxyErrorCode(Common::ErrorCode(value), Common::ApiMonitoring::ApiNameDescription());
            }

            static ProxyErrorCode CreateRAPError(Common::ErrorCode const & value)
            {
                return ProxyErrorCode(Common::ErrorCode(value), Common::ApiMonitoring::ApiNameDescription());
            }

            static ProxyErrorCode Success()
            {
                return ProxyErrorCode(Common::ErrorCode::Success(), Common::ApiMonitoring::ApiNameDescription());
            }

            static ProxyErrorCode RAProxyOperationIncompatibleWithCurrentFupState()
            {
                return CreateRAPError(Common::ErrorCodeValue::RAProxyOperationIncompatibleWithCurrentFupState);
            }

            ProxyErrorCode(ProxyErrorCode && other) :
                errorCode_(std::move(other.errorCode_)),
                message_(std::move(other.message_)),
                isDeserialized_(std::move(other.isDeserialized_)),
                api_(std::move(other.api_))
            {
            }

            ProxyErrorCode(ProxyErrorCode const& other) :
                errorCode_(other.errorCode_),
                message_(other.message_),
                isDeserialized_(other.isDeserialized_),
                api_(other.api_)
            {
            }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            __declspec(property(get = get_ErrorCode)) Common::ErrorCode const & ErrorCode;
            Common::ErrorCode const & get_ErrorCode() const
            {
                if (!isDeserialized_)
                {
                    auto copy = message_;
                    errorCode_ = Common::ErrorCode(errorCode_.ReadValue(), std::move(copy));
                    isDeserialized_ = true;
                }

                return errorCode_;
            }

            __declspec(property(get = get_Api)) Common::ApiMonitoring::ApiNameDescription const & Api;
            Common::ApiMonitoring::ApiNameDescription const & get_Api() const { return api_; }
            
            // There's scenario that even it's a RAP error but api is still valid, like RAProxyStateChangedOnDatatLoss.
			__declspec(property(get = get_IsUserApiFailure)) bool IsUserApiFailure;
			bool get_IsUserApiFailure() const { return !IsError(ErrorCodeValue::RAProxyStateChangedOnDataLoss) && !IsSuccess() && api_.IsValid; }

            Common::ErrorCodeValue::Enum ReadValue() const
            {
                return ErrorCode.ReadValue();
            }

            bool IsError(Common::ErrorCodeValue::Enum value) const
            {
                return ErrorCode.IsError(value);
            }

            bool IsSuccess() const
            {
                return ErrorCode.IsSuccess();
            }

            FABRIC_FIELDS_03(errorCode_, message_, api_);

        private:
            ProxyErrorCode(Common::ErrorCode && errorCode, Common::ApiMonitoring::ApiNameDescription && api) :
                errorCode_(errorCode.ReadValue()),
                message_(errorCode.TakeMessage()),
                isDeserialized_(false),
                api_(std::move(api))
            {
				if (errorCode.IsError(ErrorCodeValue::RAProxyDemoteCompleted) || errorCode.IsError(ErrorCodeValue::RAProxyStateChangedOnDataLoss))
				{
					TESTASSERT_IF(IsUserApiFailure, "ProxyErrorCode {0} should not be UserApiFailure", *this);
				}
            }

            mutable Common::ErrorCode errorCode_;
            mutable bool isDeserialized_;
            std::wstring message_;
            Common::ApiMonitoring::ApiNameDescription api_;
        };
    }
}
