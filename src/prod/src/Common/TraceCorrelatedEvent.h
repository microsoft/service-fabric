// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct TraceCorrelatedEventBase
    {
    private:
        DENY_COPY(TraceCorrelatedEventBase);

    protected:
        TraceCorrelatedEventBase() { }
        static void TraceConstantString(uint16 contextSequenceId, StringLiteral const & optionalText);

    public:

        virtual void WriteTo(TextWriter&, FormatOptions const&) const {};
        virtual void WriteToEtw(uint16) const {};
    };

    template<typename T>
    struct TraceCorrelatedEvent : public TraceCorrelatedEventBase
    {
    private:
        DENY_COPY(TraceCorrelatedEvent);

    public:
        TraceCorrelatedEvent(T const & object)
            : object_(object)
        {
        }

        void WriteTo(TextWriter& w, FormatOptions const& option) const
        {
            object_.WriteTo(w, option);
        }

        void WriteToEtw(uint16 contextSequenceId) const
        {
            object_.WriteToEtw(contextSequenceId);
        }

    private:
        T const & object_;
    };
}
