// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PropertyValueDescription;
    using PropertyValueDescriptionSPtr = std::shared_ptr<PropertyValueDescription>;
    using PropertyValueDescriptionUPtr = std::unique_ptr<PropertyValueDescription>;

    class PropertyValueDescription : public Common::IFabricJsonSerializable
    {
    public:
        PropertyValueDescription();

        explicit PropertyValueDescription(FABRIC_PROPERTY_TYPE_ID const & kind);

        virtual ~PropertyValueDescription();

        static PropertyValueDescriptionSPtr CreateSPtr(FABRIC_PROPERTY_TYPE_ID kind);
        
        __declspec(property(get=get_Kind)) FABRIC_PROPERTY_TYPE_ID const & Kind;
        FABRIC_PROPERTY_TYPE_ID const & get_Kind() const { return kind_; }

        virtual Common::ErrorCode GetValueBytes(__out Common::ByteBuffer & buffer)
        {
            UNREFERENCED_PARAMETER(buffer);
            return Common::ErrorCodeValue::InvalidArgument;
        }

        virtual Common::ErrorCode Verify() { return Common::ErrorCodeValue::InvalidArgument; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        JSON_TYPE_ACTIVATOR_METHOD(PropertyValueDescription, FABRIC_PROPERTY_TYPE_ID, kind_, CreateSPtr)

    private:
        FABRIC_PROPERTY_TYPE_ID kind_;
    };

    template<FABRIC_PROPERTY_TYPE_ID>
    class PropertyValueTypeActivator
    {
    public:
        static PropertyValueDescriptionSPtr CreateSPtr()
        {
            return std::make_shared<PropertyValueDescription>();
        }
    };

    // All types are essentially the same, except for type, enum, and how the ByteBuffer is filled
    // Defining classes here using a macro so we don't have a lot of files with near-identical classes.
#define PROPERTY_VALUE_DESCRIPTION(Name, Enum, Type)                                                \
        class Name##PropertyValueDescription : public PropertyValueDescription                      \
        {                                                                                           \
        public:                                                                                     \
            Name##PropertyValueDescription()                                                        \
                : PropertyValueDescription(Enum)                                                    \
                , value_()                                                                          \
            {                                                                                       \
            }                                                                                       \
                                                                                                    \
            explicit Name##PropertyValueDescription(Type && value)                                  \
                : PropertyValueDescription(Enum)                                                    \
                , value_(std::move(value))                                                          \
            {                                                                                       \
            }                                                                                       \
                                                                                                    \
            ~Name##PropertyValueDescription() {};                                                   \
                                                                                                    \
            __declspec(property(get=get_Value)) Type const & Value;                                 \
            Type const & get_Value() const { return value_; }                                       \
                                                                                                    \
            Common::ErrorCode GetValueBytes(__out Common::ByteBuffer & buffer) override;            \
                                                                                                    \
            Common::ErrorCode Verify()                                                              \
            {                                                                                       \
                if (Kind == Enum)                                                                   \
                {                                                                                   \
                    return Common::ErrorCode::Success();                                            \
                }                                                                                   \
                return Common::ErrorCode::Success();                                                \
            }                                                                                       \
                                                                                                    \
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()                                                    \
                SERIALIZABLE_PROPERTY_CHAIN()                                                       \
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Data, value_)                        \
            END_JSON_SERIALIZABLE_PROPERTIES()                                                      \
                                                                                                    \
        private:                                                                                    \
            Type value_;                                                                            \
        };                                                                                          \
                                                                                                    \
        using Name##PropertyValueDescriptionSPtr = std::shared_ptr<Name##PropertyValueDescription>; \
                                                                                                    \
        template<> class PropertyValueTypeActivator<Enum>                                           \
        {                                                                                           \
        public:                                                                                     \
            static PropertyValueDescriptionSPtr CreateSPtr()                                        \
            {                                                                                       \
                return std::make_shared<Name##PropertyValueDescription>();                          \
            }                                                                                       \
        };

    PROPERTY_VALUE_DESCRIPTION(Binary, FABRIC_PROPERTY_TYPE_BINARY, Common::ByteBuffer);
    PROPERTY_VALUE_DESCRIPTION(Int64, FABRIC_PROPERTY_TYPE_INT64, int64);
    PROPERTY_VALUE_DESCRIPTION(Double, FABRIC_PROPERTY_TYPE_DOUBLE, double);
    PROPERTY_VALUE_DESCRIPTION(String, FABRIC_PROPERTY_TYPE_WSTRING, std::wstring);
    PROPERTY_VALUE_DESCRIPTION(Guid, FABRIC_PROPERTY_TYPE_GUID, Common::Guid);
}
