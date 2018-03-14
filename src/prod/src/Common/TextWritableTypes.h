// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <typename T1, typename T2>
    class TextWritablePair : public ITextWritable
    {
    public:
        TextWritablePair(std::pair<T1, T2> const & value)
            : value_(&value)
        {
        }

        virtual void WriteTo(TextWriter & w, FormatOptions const &) const
        {
            w.Write(value_->first);
            w.Write(':');
            w.Write(value_->second);
        }

    private:
        std::pair<T1, T2> const* value_;
    };

    template <typename T>
    class TextWritableCollection : public ITextWritable
    {
    public:
        TextWritableCollection(T const & values)
            : values_(&values)
        {
        }
        
        TextWritableCollection & operator =(TextWritableCollection const & other)
        {
            values_ = other.values_;
    
            return *this;
        }

        virtual void WriteTo(TextWriter & w, FormatOptions const & option) const
        {
            w.WriteChar('(');

            int count = 0;
            for (auto it = values_->cbegin(); it != values_->cend(); ++it)
            {
                if (count++ > 0)
                {
                    w.WriteChar(' ');
                }

                w.Write(*it);

                if (option.width != 0 && count >= option.width)
                {
                    w.Write(" ...");
                    __if_exists(T::size)
                    {
                        w.Write(" count={0}", values_->size());
                    }
                    break;
                }
            }

            w.WriteChar(')');
        }

    private:
        T const* values_;
    };

    template <typename T>
    class TextWritableWrapper : public ITextWritable
    {
    public:
        TextWritableWrapper(T const * value)
            : t_(value)
        {
        }

        virtual void WriteTo(TextWriter & w, FormatOptions const & format) const
        {
            if (!t_)
            {
                w.Write("nullptr");
            }
            else
            {
                __if_exists (T::WriteTo)
                {
                    t_->WriteTo(w, format);
                }
                __if_not_exists (T::WriteTo)
                {
                    UNREFERENCED_PARAMETER(format);
                    WriteToTextWriter(w, *t_);
                }
            }
        }

    private:
        T const* t_;
    };

    template <typename T>
    extern void WriteToTextWriter(Common::TextWriter &, const T &);
}
