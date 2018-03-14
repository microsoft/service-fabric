// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    ProgressVectorEntry::ProgressVectorEntry()
        : lastOperationLSN_(0)
    {
        epoch_.ConfigurationNumber = 0;
        epoch_.DataLossNumber = 0;
    }

    ProgressVectorEntry::ProgressVectorEntry(
        ::FABRIC_EPOCH const & epoch, 
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN)
        : epoch_(epoch)
        , lastOperationLSN_(lastOperationLSN)
    {
    }
    
    void ProgressVectorEntry::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("{0}.{1:X}.{2}", epoch_.DataLossNumber, epoch_.ConfigurationNumber, lastOperationLSN_);
    }

    // 
    // ProgressVectorData
    //

    ProgressVectorData::ProgressVectorData() 
        : progressVector_() 
    { 
    }

    ProgressVectorData::ProgressVectorData(vector<ProgressVectorEntry> && entries) 
        : progressVector_(move(entries)) 
    { 
    }

    bool ProgressVectorData::TryTruncate(::FABRIC_SEQUENCE_NUMBER upToLsn)
    {
        auto preSize = progressVector_.size();

        progressVector_.erase(remove_if(progressVector_.begin(), progressVector_.end(),
            [upToLsn](ProgressVectorEntry const & entry)
            {
                return (entry.LastOperationLSN > upToLsn);
            }), 
            progressVector_.end());

        return (progressVector_.size() < preSize);
    }

    void ProgressVectorData::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << L"( ";

        for (auto const & entry : progressVector_)
        {
            w << entry << L" ";
        }

        w << L")";
    }
}
