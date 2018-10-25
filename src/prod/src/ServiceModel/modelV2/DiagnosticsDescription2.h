// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class DiagnosticsDescription2
        : public ModelType
        {
        public:
            DiagnosticsDescription2() = default;

            DEFAULT_MOVE_ASSIGNMENT(DiagnosticsDescription2)
            DEFAULT_MOVE_CONSTRUCTOR(DiagnosticsDescription2)
            DEFAULT_COPY_ASSIGNMENT(DiagnosticsDescription2)
            DEFAULT_COPY_CONSTRUCTOR(DiagnosticsDescription2)

            __declspec(property(get=get_Sinks, put=put_Sinks)) std::vector<DiagnosticsSinkBase> const & Sinks;
            std::vector<DiagnosticsSinkBase> const & get_Sinks() const { return sinks_; }
            void put_Sinks(std::vector<DiagnosticsSinkBase> const & value) { sinks_ = value; }

            __declspec(property(get=get_Enabled, put=put_Enabled)) bool const & Enabled;
            bool const & get_Enabled() const { return enabled_; }
            void put_Enabled(bool const & value) { enabled_ = value; }

            __declspec(property(get=get_DefaultSinkRefs, put=put_DefaultSinkRefs)) std::vector<std::wstring> const & DefaultSinkRefs;
            std::vector<std::wstring> const & get_DefaultSinkRefs() const { return defaultSinkRefs_; }
            void put_DefaultSinkRefs(std::vector<std::wstring> const & value) { defaultSinkRefs_ = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"sinks", sinks_)
                SERIALIZABLE_PROPERTY(L"enabled", enabled_)
                SERIALIZABLE_PROPERTY(L"defaultSinkRefs", defaultSinkRefs_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(sinks_, enabled_, defaultSinkRefs_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(sinks_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(enabled_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(defaultSinkRefs_)
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode TryValidate(std::wstring const &traceId) const override
            {
                (void)traceId;
                return Common::ErrorCodeValue::Success;
            }

        protected:
            std::vector<DiagnosticsSinkBase> sinks_;
            bool enabled_;
            std::vector<std::wstring> defaultSinkRefs_;
        };
    }
}