// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class UnreliableTransportSpecification
    {
        DENY_COPY(UnreliableTransportSpecification);

    public:
        UnreliableTransportSpecification(
            std::wstring const & name,
            std::wstring const & source,
            std::wstring const & destination, 
            std::wstring const & actionFilter, 
            double probabilityToApply, 
            Common::TimeSpan delay, 
            Common::TimeSpan delaySpan,
            int priority, 
            int applyCount = -1,
            Common::StringMap const & filters = Common::StringMap());

        bool Match(std::wstring const & source, std::wstring const & destination, std::wstring const & action);
        
        bool Match(std::wstring const & source, std::wstring const & destination, Transport::Message const & message);

        Common::TimeSpan GetDelay();

        int GetPriority() const { return this->priority_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring const & GetName() const { return name_; }

        static Common::WStringLiteral const MatchAll;

    private:
        std::wstring name_;
        std::wstring source_;
        std::wstring destination_;
        std::wstring action_;
        double probabilityToApply_;
        Common::TimeSpan delay_;
        double delaySpan_;
        int priority_;
        LONG applyCount_;
        bool countActive_;
        Common::Random random_;
        Common::StringMap filters_;
        RWLOCK(UnreliableTransportSpecification, lock_);
    };

    typedef std::unique_ptr<UnreliableTransportSpecification> UnreliableTransportSpecificationUPtr;
}
