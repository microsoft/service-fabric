// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct Guid;
    struct LargeInteger;
    class ErrorCode;

    class VariableArgument
    {
    private:
        enum Enum
        {
            TypeInvalid = 0,
            TypeInplaceObject = 1,
            TypeBool = 2,
            TypeSignedNumber = 3,
            TypeUnsignedNumber = 4,
            TypeLargeInteger = 5,
            TypeGuid = 6,
            TypeDouble = 7,
            TypeDateTime = 8,
            TypeStopwatchTime = 9,
            TypeTimeSpan = 10,
            TypeChar = 11,
            TypeWChar = 12,
            TypeString = 13,
            TypeWString = 14,
            TypeStringCollection = 15,
            TypePointer = 16,
            Type_FABRIC_EPOCH = 17,
            Type_FABRIC_OPERATION_TYPE = 18,
            Type_FABRIC_OPERATION_METADATA = 19,
            Type_FABRIC_SERVICE_PARTITION_ACCESS_STATUS = 20,
            Type_FABRIC_REPLICA_SET_QUORUM_MODE = 21,
            Type_FABRIC_REPLICA_ROLE = 22,
            Type_FABRIC_HEALTH_STATE = 23,
            Type_FABRIC_HEALTH_REPORT_KIND = 24,
            Type_FABRIC_QUERY_SERVICE_PARTITION_STATUS = 25,
            Type_FABRIC_QUERY_NODE_STATUS = 26,
            Type_FABRIC_PARTITION_KEY_TYPE = 27,
            Type_FABRIC_SERVICE_PARTITION_KIND = 28,
            Type_FABRIC_SERVICE_LOAD_METRIC_WEIGHT = 29,
            Type_FABRIC_SERVICE_CORRELATION_SCHEME = 30,
            Type_FABRIC_FAULT_TYPE = 31,
            Type_FABRIC_NODE_DEACTIVATION_INTENT = 32,
            Type_FABRIC_SERVICE_DESCRIPTION_KIND = 33,
            Type_FABRIC_PARTITION_SCHEME = 34,
            Type_FABRIC_PROPERTY_TYPE_ID = 35,
            Type_FABRIC_PROPERTY_BATCH_OPERATION_KIND = 36,
            Type_FABRIC_QUERY_SERVICE_STATUS = 37,
            Type_XmlNodeType = 38,
            TypeErrorCode = 39,
            TypeStdException = 40,
            TypeStdErrorCategory = 41,
            TypeStdErrorCode = 42,
            TypeStdSystemError = 43,
            Type_FABRIC_HEALTH_EVALUATION_KIND = 44,
            Type_FABRIC_QUERY_SERVICE_OPERATION_NAME = 45,
            Type_FABRIC_QUERY_REPLICATOR_OPERATION_NAME = 46,
            Type_FABRIC_SERVICE_KIND = 47,
            Type_FABRIC_HEALTH_STATE_FILTER = 48,
            Type_FABRIC_NODE_DEACTIVATION_TASK_TYPE = 49,
            Type_FABRIC_TEST_COMMAND_PROGRESS_STATE = 50,
            Type_FABRIC_TEST_COMMAND_TYPE = 51,
            Type_FABRIC_PACKAGE_SHARING_POLICY_SCOPE = 52,
            Type_FABRIC_SERVICE_ENDPOINT_ROLE = 53,
            Type_FABRIC_APPLICATION_UPGRADE_STATE = 54,
            Type_FABRIC_ROLLING_UPGRADE_MODE = 55,
            Type_FABRIC_UPGRADE_FAILURE_REASON = 56,
            Type_FABRIC_UPGRADE_DOMAIN_STATE = 57,
            Type_FABRIC_UPGRADE_STATE = 58,
            Type_FABRIC_SERVICE_TYPE_REGISTRATION_STATUS = 59,
            Type_FABRIC_QUERY_SERVICE_REPLICA_STATUS = 60,
            Type_FABRIC_UPGRADE_KIND = 61,
            Type_FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS = 62,
            Type_FABRIC_MOVE_COST = 63,
            Type_FABRIC_NODE_DEACTIVATION_STATUS = 64,
            Type_FABRIC_PLACEMENT_POLICY_TYPE = 65,
            Type_FABRIC_SERVICE_REPLICA_KIND = 66,
            Type_FABRIC_PARTITION_SELECTOR_TYPE = 67,
            Type_FABRIC_DATA_LOSS_MODE = 68,
            Type_FABRIC_QUORUM_LOSS_MODE = 69,
            Type_FABRIC_RESTART_PARTITION_MODE = 70,
            Type_FABRIC_NODE_TRANSITION_TYPE = 71,
            Type_FABRIC_CHAOS_EVENT_KIND = 72,
            Type_FABRIC_CHAOS_STATUS = 73,
            Type_FABRIC_RECONFIGURATION_PHASE = 74,
            Type_FABRIC_RECONFIGURATION_TYPE = 75,
            Type_FABRIC_PROVISION_APPLICATION_TYPE_KIND = 76,
            TypeTextWritablePointer = 77,
            Type_FABRIC_DIAGNOSTICS_SINKS_KIND = 78,
            Type_FABRIC_NETWORK_TYPE = 79,
            Type_FABRIC_NETWORK_STATUS = 80
        };

        struct StringValue
        {
            size_t size_;
            char const* buffer_;
        };

        struct WStringValue
        {
            size_t size_;
            wchar_t const* buffer_;
        };

        union Value
        {
            bool valueBool_;
            int64 valueInt64_;
            uint64 valueUInt64_;
            LargeInteger const* valueLargeInteger_;
            Guid const* valueGuid_;
            double valueDouble_;
            char valueChar_;
            wchar_t valueWChar_;
            StringValue valueString_;
            WStringValue valueWString_;
            StringCollection const* valueStringCollection_;
            void const* valuePointer_;
            FABRIC_EPOCH const* value_FABRIC_EPOCH_;
            FABRIC_OPERATION_METADATA const* value_FABRIC_OPERATION_METADATA_;
            std::exception const* valueStdException_;
            std::error_category const* valueStdErrorCategory_;
            std::error_code const* valueStdErrorCode_;
            std::system_error const* valueStdSystemError_;
        };

    public:
        VariableArgument();
        VariableArgument(bool value);
        VariableArgument(int64 value);
        VariableArgument(int32 value);
        VariableArgument(int16 value);
#ifndef PLATFORM_UNIX
        // Not needed on Linux since they are not considered a distinct type
        VariableArgument(LONG value);
        VariableArgument(uint32 value);
#endif
        VariableArgument(uint64 value);
        VariableArgument(uint16 value);
        VariableArgument(UCHAR value);
        VariableArgument(DWORD value);
        VariableArgument(LargeInteger const & value);
        VariableArgument(Guid const & value);
        VariableArgument(float value);
        VariableArgument(double value);
        VariableArgument(DateTime value);
        VariableArgument(StopwatchTime value);
        VariableArgument(TimeSpan value);
        VariableArgument(char value);
        VariableArgument(wchar_t value);
        VariableArgument(char const* value);
        VariableArgument(_In_ char * value);
        VariableArgument(wchar_t const* value);
        VariableArgument(_In_ wchar_t * value);
        VariableArgument(std::string const & value);
        VariableArgument(std::wstring const & value);
        VariableArgument(literal_holder<char> const & value);
        VariableArgument(literal_holder<wchar_t> const & value);
        VariableArgument(GlobalWString const & value);
        VariableArgument(StringCollection const & value);
        VariableArgument(FABRIC_EPOCH const & value);
        VariableArgument(FABRIC_OPERATION_METADATA const & value);
        VariableArgument(FABRIC_OPERATION_TYPE value);
        VariableArgument(FABRIC_SERVICE_PARTITION_ACCESS_STATUS value);
        VariableArgument(FABRIC_REPLICA_SET_QUORUM_MODE value);
        VariableArgument(FABRIC_REPLICA_ROLE value);
        VariableArgument(FABRIC_HEALTH_STATE value);
        VariableArgument(FABRIC_HEALTH_REPORT_KIND value);
        VariableArgument(FABRIC_HEALTH_EVALUATION_KIND value);
        VariableArgument(FABRIC_QUERY_SERVICE_PARTITION_STATUS value);
        VariableArgument(FABRIC_QUERY_NODE_STATUS value);
        VariableArgument(FABRIC_PARTITION_KEY_TYPE value);
        VariableArgument(FABRIC_SERVICE_PARTITION_KIND value);
        VariableArgument(FABRIC_SERVICE_LOAD_METRIC_WEIGHT value);
        VariableArgument(FABRIC_SERVICE_CORRELATION_SCHEME value);
        VariableArgument(FABRIC_FAULT_TYPE value);
        VariableArgument(FABRIC_NODE_DEACTIVATION_INTENT value);
        VariableArgument(FABRIC_SERVICE_DESCRIPTION_KIND value);
        VariableArgument(FABRIC_PARTITION_SCHEME value);
        VariableArgument(FABRIC_PROPERTY_TYPE_ID value);
        VariableArgument(FABRIC_PROPERTY_BATCH_OPERATION_KIND value);
        VariableArgument(FABRIC_QUERY_SERVICE_STATUS value);
        VariableArgument(FABRIC_QUERY_SERVICE_OPERATION_NAME value);
        VariableArgument(FABRIC_QUERY_REPLICATOR_OPERATION_NAME value);
        VariableArgument(FABRIC_SERVICE_KIND value);
        VariableArgument(FABRIC_HEALTH_STATE_FILTER value);
        VariableArgument(FABRIC_NODE_DEACTIVATION_TASK_TYPE value);
        VariableArgument(FABRIC_TEST_COMMAND_PROGRESS_STATE value);
        VariableArgument(FABRIC_TEST_COMMAND_TYPE value);
        VariableArgument(FABRIC_PACKAGE_SHARING_POLICY_SCOPE value);
        VariableArgument(FABRIC_SERVICE_ENDPOINT_ROLE value);
        VariableArgument(FABRIC_APPLICATION_UPGRADE_STATE value);
        VariableArgument(FABRIC_ROLLING_UPGRADE_MODE value);
        VariableArgument(FABRIC_UPGRADE_FAILURE_REASON value);
        VariableArgument(FABRIC_UPGRADE_DOMAIN_STATE value);
        VariableArgument(FABRIC_UPGRADE_STATE value);
        VariableArgument(FABRIC_SERVICE_TYPE_REGISTRATION_STATUS value);
        VariableArgument(FABRIC_QUERY_SERVICE_REPLICA_STATUS value);
        VariableArgument(FABRIC_UPGRADE_KIND value);
        VariableArgument(FABRIC_STATEFUL_SERVICE_FAILOVER_SETTINGS_FLAGS value);
        VariableArgument(FABRIC_MOVE_COST value);
        VariableArgument(FABRIC_NODE_DEACTIVATION_STATUS value);
        VariableArgument(FABRIC_PLACEMENT_POLICY_TYPE value);
        VariableArgument(FABRIC_SERVICE_REPLICA_KIND value);
        VariableArgument(FABRIC_PARTITION_SELECTOR_TYPE value);
        VariableArgument(FABRIC_DATA_LOSS_MODE value);
        VariableArgument(FABRIC_QUORUM_LOSS_MODE value);
        VariableArgument(FABRIC_RESTART_PARTITION_MODE value);
        VariableArgument(FABRIC_NODE_TRANSITION_TYPE value);
        VariableArgument(FABRIC_CHAOS_EVENT_KIND value);
        VariableArgument(FABRIC_CHAOS_STATUS value);
        VariableArgument(FABRIC_RECONFIGURATION_PHASE value);
        VariableArgument(FABRIC_RECONFIGURATION_TYPE value);
        VariableArgument(FABRIC_PROVISION_APPLICATION_TYPE_KIND value);
        VariableArgument(FABRIC_DIAGNOSTICS_SINKS_KIND value);
        VariableArgument(FABRIC_NETWORK_TYPE value);
        VariableArgument(FABRIC_NETWORK_STATUS value);
        VariableArgument(XmlNodeType value);
        VariableArgument(ErrorCode const & value);
        VariableArgument(std::exception const & value);
        VariableArgument(std::error_category const & value);
        VariableArgument(std::error_code const & value);
        VariableArgument(std::system_error const & value);

        template <typename T>
        VariableArgument(T* value)
            : type_(std::is_base_of<ITextWritable, T>::value ? TypeTextWritablePointer : TypePointer)
        {
            value_.valuePointer_ = value;
        }

        template <typename T>
        VariableArgument(T const* value)
            : type_(std::is_base_of<ITextWritable, T>::value ? TypeTextWritablePointer : TypePointer)
        {
            value_.valuePointer_ = value;
        }

        template <typename T>
        VariableArgument(std::vector<T> const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritableCollection<std::vector<T>>(value);
        }

        template <typename T>
        VariableArgument(std::set<T> const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritableCollection<std::set<T>>(value);
        }

        template <typename T1, typename T2>
        VariableArgument(std::pair<T1, T2> const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritablePair<T1, T2>(value);
        }

        template <typename T1, typename T2>
        VariableArgument(std::map<T1, T2> const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritableCollection<std::map<T1, T2>>(value);
        }

        template <typename T>
        VariableArgument(std::unique_ptr<T> const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritableWrapper<T>(value.get());
        }

        template <typename T>
        VariableArgument(std::shared_ptr<T> const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritableWrapper<T>(value.get());
        }

        template <typename T>
        VariableArgument(T const & value)
            : type_(TypeInplaceObject)
        {
            new (&value_) TextWritableWrapper<T>(&value);
        }

        bool IsValid() const;

        void WriteTo(TextWriter& w, FormatOptions const & format) const;

    private:
        Enum type_;
        Value value_;
    };
}
