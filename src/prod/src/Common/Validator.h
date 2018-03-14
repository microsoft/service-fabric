// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<typename T>
    class Validator
    {
    public:
        virtual bool IsValid(T & value) const = 0;
    };

    class LessThan : public Validator<int>
    {
    public:
        LessThan(int target)
            : target_(target)
        {
        }

        bool IsValid(int & arg) const
        {
            return (arg < target_);
        }

    private:
        int target_;
    };

    class NoLessThan : public Validator<int>
    {
    public:
        NoLessThan(int target)
            : target_(target)
        {
        }

        bool IsValid(int & arg) const
        {
            return !(arg < target_);
        }

    private:
        int target_;
    };

    class GreaterThan : public Validator<int>
    {
    public:
        GreaterThan(int target)
            : target_(target)
        {
        }

        bool IsValid(int & arg) const
        {
            return (arg > target_);
        }

    private:
        int target_;
    };

    class UIntGreaterThan : public Validator<uint>
    {
    public:
        UIntGreaterThan(uint target)
            : target_(target)
        {
        }

        bool IsValid(uint & arg) const
        {
            return (arg > target_);
        }

    private:
        uint target_;
    };

    class UIntNoLessThan : public Validator<uint>
    {
    public:
        UIntNoLessThan(uint target)
            : target_(target)
        {
        }

        bool IsValid(uint & arg) const
        {
            return (arg >= target_);
        }

    private:
        uint target_;
    };


    class TimeSpanGreaterThan : public Validator<Common::TimeSpan>
    {
    public:
        TimeSpanGreaterThan(Common::TimeSpan const & target)
            : target_(target)
        {
        }

        bool IsValid(Common::TimeSpan & arg) const
        {
            return (arg > target_);
        }

    private:
        Common::TimeSpan target_;
    };

    class TimeSpanNoLessThan : public Validator<Common::TimeSpan>
    {
    public:
        TimeSpanNoLessThan(Common::TimeSpan const & target)
            : target_(target)
        {
        }

        bool IsValid(Common::TimeSpan & arg) const
        {
            return (arg >= target_);
        }

    private:
        Common::TimeSpan target_;
    };

    template<typename T>
    class InRange : public Validator<T>
    {
    public:
        InRange(T low, T high)
            : low_(low),
              high_(high)
        {
        }

        bool IsValid(T & arg) const
        {
            return (arg >= low_ && arg <= high_);
        }

    private:
        T low_;
        T high_;
    };

    class NotEmpty : public Validator<std::wstring>
    {
    public:
        bool IsValid(std::wstring & arg) const
        {
            return (!arg.empty());
        }
    };
}
