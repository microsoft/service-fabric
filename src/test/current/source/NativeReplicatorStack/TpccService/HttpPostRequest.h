// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    // This is POST request body in JSON
    // {
    // 	 "Operation": "CheckWorkStatus"
    // }

    class HttpPostRequest : public Common::IFabricJsonSerializable
    {
    public:

        HttpPostRequest()
        {
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        _declspec(property(get = get_operation)) ClientOperationEnum::Enum Operation;
        ClientOperationEnum::Enum get_operation() const
        {
            return op_;
        }

        std::wstring ToString() const
        {
            using namespace ClientOperationEnum;

            switch (op_)
            {
				case ScheduleWork: {
					return L"ScheduleWork";
				}
				case CheckWorkStatus: {
					return L"CheckWorkStatus";
				}
				case GetWorkResult: {
					return L"GetWorkResult";
				}
				default: {
					return L"";
				}
            }
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(L"Operation", op_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        ClientOperationEnum::Enum op_;
    };
}
