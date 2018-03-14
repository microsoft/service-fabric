// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationTransportSettings
            : public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
        {
            DENY_COPY(ServiceCommunicationTransportSettings);

        public:
            ServiceCommunicationTransportSettings()
                : maxMessageSize_(0)
                , maxConcurrentCalls_(0)
                , maxQueueSize_(INT_MAX)/*Default Queue Size*/
                , securitySettings_()
                , operationTimeout_(Common::TimeSpan::MaxValue)
                , keepAliveTimeout_(Common::TimeSpan::MaxValue)
                , connectTimeout_(Common::TimeSpan::FromSeconds(5))
            {
            }

            ServiceCommunicationTransportSettings(
                int maxMessageSize,
                int maxConcurrentCalls,
                int maxQueueSize,
                Transport::SecuritySettings securitySettings,
                Common::TimeSpan operationTimeout,
                Common::TimeSpan keepAliveTimeout,
                Common::TimeSpan connectTimeout)
                : maxMessageSize_(maxMessageSize)
                , maxConcurrentCalls_(maxConcurrentCalls)
                , securitySettings_(securitySettings)
                , operationTimeout_(operationTimeout)
                , keepAliveTimeout_(keepAliveTimeout)
                , maxQueueSize_(maxQueueSize)
                , connectTimeout_(connectTimeout)
            {
            }

            static Common::ErrorCode FromPublicApi(
                __in FABRIC_SERVICE_TRANSPORT_SETTINGS const& settings,
                __out ServiceCommunicationTransportSettingsUPtr & output);

            static Common::ErrorCode FromPublicApi(
                __in FABRIC_TRANSPORT_SETTINGS const& settings,
                __out ServiceCommunicationTransportSettingsUPtr & output);

            bool operator == (ServiceCommunicationTransportSettings const & rhs) const;
            bool operator != (ServiceCommunicationTransportSettings const & rhs) const;

            __declspec(property(get = get_MaxMessageSize)) int MaxMessageSize;
            int get_MaxMessageSize() const { return maxMessageSize_; }

            __declspec(property(get = get_MaxConcurrentCalls)) int MaxConcurrentCalls;
            int get_MaxConcurrentCalls() const { return maxConcurrentCalls_; }

            __declspec(property(get = get_MaxQueueSize)) int MaxQueueSize;
            int get_MaxQueueSize() const { return maxQueueSize_; }

            __declspec(property(get = get_OperationTimeout)) Common::TimeSpan OperationTimeout;
            Common::TimeSpan get_OperationTimeout() const { return operationTimeout_; }

            __declspec(property(get = get_ConnectTimeout)) Common::TimeSpan ConnectTimeout;
            Common::TimeSpan get_ConnectTimeout() const { return connectTimeout_; }

            __declspec(property(get = get_KeepAliveTimeout)) Common::TimeSpan KeepAliveTimeout;
            Common::TimeSpan get_KeepAliveTimeout() const { return keepAliveTimeout_; }

            __declspec(property(get = get_SecuritySetting)) Transport::SecuritySettings const & SecuritySetting;
            Transport::SecuritySettings const & get_SecuritySetting() const { return securitySettings_; }

        private:
            static Common::ErrorCode Validate(
                __in ServiceCommunicationTransportSettingsUPtr & toBeValidated);

            int maxMessageSize_;
            int maxConcurrentCalls_;
            int maxQueueSize_;
            Transport::SecuritySettings securitySettings_;
            Common::TimeSpan operationTimeout_;
            Common::TimeSpan keepAliveTimeout_;
            Common::TimeSpan connectTimeout_;
        };
    }
}
