// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class ServiceProgress : public Common::IFabricJsonSerializable
    {
    public:

        ServiceProgress()
            : countWarehouses_(0)
            , countDistricts_(0)
            , countNewOrders_(0)
            , countCustomers_(0)
            , countOrders_(0)
            , countHistory_(0)
            , countStock_(0)
            , countOrderLine_(0)
            , countItems_(0)
            , lastCommittedSequenceNumber_(0)
			, currentDataLossNumber_(0)
        {
        }

        PROPERTY(LONG64, countWarehouses_, Warehouses)
        PROPERTY(LONG64, countDistricts_, Districts)
        PROPERTY(LONG64, countNewOrders_, NewOrders)
        PROPERTY(LONG64, countCustomers_, Customers)
        PROPERTY(LONG64, countOrders_, Orders)
        PROPERTY(LONG64, countHistory_, History)
        PROPERTY(LONG64, countStock_, Stock)
        PROPERTY(LONG64, countOrderLine_, OrderLine)
        PROPERTY(LONG64, countItems_, Items)
        PROPERTY(LONG64, lastCommittedSequenceNumber_, LastCommittedSequenceNumber)
		PROPERTY(LONG64, currentDataLossNumber_, CurrentDataLossNumber)

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        {
            w.Write(ToString());
        }

        std::wstring ToString() const
        {
            return Common::wformatString(
                "CountWarehouses = '{0}', "
                "CountDistricts = '{1}', "
                "CountCustomers = {2}, "
                "CountNewOrders = '{3}', "
                "CountOrders ='{4}', "
                "CountHistory = '{5}', "
                "CountStock = '{6}', "
                "CountOrderLine = '{7}', "
                "CountItems = '{8}'",
                countWarehouses_,
                countDistricts_,
                countCustomers_,
                countNewOrders_,
                countOrders_,
                countHistory_,
                countStock_,
                countOrderLine_,
                countItems_);
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"CountWarehouses", countWarehouses_)
            SERIALIZABLE_PROPERTY(L"CountDistricts", countDistricts_)
            SERIALIZABLE_PROPERTY(L"CountCustomers", countCustomers_)
            SERIALIZABLE_PROPERTY(L"CountNewOrders", countNewOrders_)
            SERIALIZABLE_PROPERTY(L"CountOrders", countOrders_)
            SERIALIZABLE_PROPERTY(L"CountHistory", countHistory_)
            SERIALIZABLE_PROPERTY(L"CountStock", countStock_)
            SERIALIZABLE_PROPERTY(L"CountOrderLine", countOrderLine_)
            SERIALIZABLE_PROPERTY(L"CountItems", countItems_)
            SERIALIZABLE_PROPERTY(L"LastCommittedSequenceNumber", lastCommittedSequenceNumber_)
			SERIALIZABLE_PROPERTY(L"CurrentDataLossNumber", currentDataLossNumber_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        LONG64 countWarehouses_;
        LONG64 countDistricts_;
        LONG64 countNewOrders_;
        LONG64 countCustomers_;
        LONG64 countOrders_;
        LONG64 countHistory_;
        LONG64 countStock_;
        LONG64 countOrderLine_;
        LONG64 countItems_;
        LONG64 lastCommittedSequenceNumber_;
		LONG64 currentDataLossNumber_;
    };
}
