// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace TpccService
{
    // This is the response in JSON
    //{
    //	"Message": "",
    //		"WorkStatus" : "InProgress",
    //		"Progress" : {
    //		"CountWarehouses": "0",
    //			"CountDistricts" : "0",
    //			"CountCustomers" : "0",
    //			"CountNewOrders" : "0",
    //			"CountOrders" : "0",
    //			"CountHistory" : "0",
    //			"CountStock" : "0",
    //			"CountOrderLine" : "0",
    //			"CountItems" : "0",
    //			"LastCommittedSequenceNumber" : "0"
    //	},
    //	"Metrics": {
    //		"MemoryUsage": "0"
    //	}
    //}

    class HttpPostResponse : public Common::IFabricJsonSerializable
    {
    public:

        HttpPostResponse()
            : msg_(L"")
            , errorCode_(Common::ErrorCodeValue::Success)
            , workStatus_(WorkStatusEnum::Unknown)
            , progress_()
        {
        }

        PROPERTY(std::wstring, msg_, Message)
        PROPERTY(Common::ErrorCode, errorCode_, ErrorCode)
        PROPERTY(WorkStatusEnum::Enum, workStatus_, WorkStatus)
        PROPERTY(ServiceProgress, progress_, Progress)
        PROPERTY(ServiceMetrics, metrics_, Metrics)

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "Msg = '{0}'",
                msg_);
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Message", msg_)
            //SERIALIZABLE_PROPERTY(L"ErrorCode", Common::wformatString("'{0}'", errorCode_))
            SERIALIZABLE_PROPERTY_ENUM(L"WorkStatus", workStatus_)
            SERIALIZABLE_PROPERTY(L"Progress", progress_)
            SERIALIZABLE_PROPERTY(L"Metrics", metrics_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring msg_;
        Common::ErrorCode errorCode_;
        WorkStatusEnum::Enum workStatus_;
        ServiceProgress progress_;
        ServiceMetrics metrics_;
    };
}
