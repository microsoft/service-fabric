// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class DiagnosticsRef
        : public ModelType
        {
        public:
            DiagnosticsRef() = default;

            DEFAULT_MOVE_ASSIGNMENT(DiagnosticsRef)
            DEFAULT_MOVE_CONSTRUCTOR(DiagnosticsRef)
            DEFAULT_COPY_ASSIGNMENT(DiagnosticsRef)
            DEFAULT_COPY_CONSTRUCTOR(DiagnosticsRef)

            __declspec(property(get=get_Enabled, put=put_Enabled)) bool const & Enabled;
            bool const & get_Enabled() const { return enabled_; }
            void put_Enabled(bool const & value) { enabled_ = value; }

            __declspec(property(get=get_SinkRefs, put=put_SinkRefs)) std::vector<std::wstring> const & SinkRefs;
            std::vector<std::wstring> const & get_SinkRefs() const { return sinkRefs_; }
            void put_SinkRefs(std::vector<std::wstring> const & value) { sinkRefs_ = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"enabled", enabled_)
                SERIALIZABLE_PROPERTY(L"sinkRefs", sinkRefs_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_02(enabled_, sinkRefs_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(enabled_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(sinkRefs_)
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode TryValidate(std::wstring const &traceId) const override
            {
                (void)traceId;
                return Common::ErrorCodeValue::Success;
            }

        protected:
            bool enabled_;
            std::vector<std::wstring> sinkRefs_;
        };
    }
}