//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ModelV2
    {
        class ModelType
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        {
        public:
            ModelType() = default;

            virtual Common::ErrorCode TryValidate(std::wstring const &traceId) const = 0;

        protected:
            std::wstring GetNextTraceId(std::wstring const &traceId, std::wstring const &traceSuffix) const
            {
                if (traceId.empty())
                {
                    return traceSuffix;
                }
                else
                {
                    return Common::wformatString("{0}.{1}", traceId, traceSuffix);
                }
            }
        };
    }
}

