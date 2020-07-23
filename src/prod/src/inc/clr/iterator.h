// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef ITERATOR_H_
#define ITERATOR_H_

#include "contract.h"

namespace HIDDEN {
// These prototypes are not for direct use - they are only here to illustrate the 
// iterator pattern

template <typename CONTAINER, typename ELEMENT>
class ScannerPrototype
{
  public:

    typedef ScannerPrototype<CONTAINER, ELEMENT> Iterator;

    ELEMENT &operator*();
    ELEMENT *operator->();

    Iterator &operator++();
    Iterator operator++(int);
    Iterator &operator--();
    Iterator operator--(int);

    bool operator==(const Iterator &i);
    bool operator!=(const Iterator &i);
};

template <typename CONTAINER, typename ELEMENT>
class EnumeratorPrototype
{
  public:
    
    typedef EnumeratorPrototype<CONTAINER, ELEMENT> Iterator;

    ELEMENT &operator*();
    ELEMENT *operator->();

    Iterator &operator++();
    Iterator operator++(int);

    bool operator==(const Iterator &i);
    bool operator!=(const Iterator &i);
};

template <typename CONTAINER, typename ELEMENT>
class IndexerPrototype
{
  public:
    typedef IndexerPrototype<CONTAINER, ELEMENT> Iterator;

    ELEMENT &operator*();
    ELEMENT *operator->();
    ELEMENT &operator[](int index);

    Iterator &operator++();
    Iterator operator++(int);
    Iterator &operator--();
    Iterator operator--(int);

    Iterator &operator+=(SCOUNT_T index);
    Iterator &operator-=(SCOUNT_T index);

    Iterator operator+(SCOUNT_T index);
    Iterator operator-(SCOUNT_T index);

    SCOUNT_T operator-(Iterator &i);

    bool operator==(const Iterator &i);
    bool operator!=(const Iterator &i);
    bool operator<(const Iterator &i);
    bool operator<=(const Iterator &i);
    bool operator>(const Iterator &i);
    bool operator>=(const Iterator &i);
};

template <typename ELEMENT>
class EnumerablePrototype
{
    typedef EnumeratorPrototype<EnumerablePrototype, ELEMENT> Iterator;

    Iterator Begin();
    Iterator End();
};

template <typename ELEMENT>
class ScannablePrototype
{
    typedef ScannerPrototype<ScannablePrototype, ELEMENT> Iterator;

    Iterator Begin();
    Iterator End();
};

template <typename ELEMENT>
class IndexablePrototype
{
    typedef IndexerPrototype<IndexablePrototype, ELEMENT> Iterator;

    Iterator Begin();
    Iterator End();
};

};

// --------------------------------------------------------------------------------
// EnumeratorBase, ScannerBase, and IndexerBase are abstract classes 
// describing basic operations for iterator functionality at the different levels.
// 
// You
// 1. Use the classes as a pattern (don't derive from them), and plug in your own 
//      class into the Enumerator/Scanner/Indexer templates below.
// 2. Subclass the AbstractEnumerator/AbstractScanner/AbstractIndexer classes
// --------------------------------------------------------------------------------

namespace HIDDEN
{
// These prototypes are not for direct use - they are only here to illustrate the 
// pattern of the BASE class for the Iterator templates

template <typename ELEMENT, typename CONTAINER>
class EnumeratorBasePrototype
{
  protected:
    EnumeratorBasePrototype(CONTAINER *container, BOOL begin);
    ELEMENT &Get() const;
    void Next();
    BOOL Equal(const EnumeratorBasePrototype &i) const;
    CHECK DoCheck() const;
};

template <typename ELEMENT, typename CONTAINER>
class ScannerBasePrototype
{
 protected:
    ScannerBasePrototype(CONTAINER *container, BOOL begin);
    ELEMENT &Get() const;
    void Next();
    void Previous();
    BOOL Equal(const ScannerBasePrototype &i) const;
    CHECK DoCheck() const;
};

template <typename ELEMENT, typename CONTAINER>
class IndexerBasePrototype
{
 protected:
    IndexerBasePrototype(CONTAINER *container, SCOUNT_T delta);
    ELEMENT &GetAt(SCOUNT_T delta) const;
    void Skip(SCOUNT_T delta);
    SCOUNT_T Subtract(const IndexerBasePrototype &i) const;
    CHECK DoCheck(SCOUNT_T delta) const;
};

};


template <typename CONTAINER>
class CheckedIteratorBase
{
  protected:
#if defined(_DEBUG)
    const CONTAINER *m_container;
    int m_revision;
#endif

    CHECK CheckRevision() const
    {
        LIMITED_METHOD_CONTRACT;
        SUPPORTS_DAC;
#if defined(_DEBUG)
        __if_exists(CONTAINER::m_revision)
        {
            CHECK_MSG(m_revision == m_container->m_revision, 
                      "Use of Iterator after container has been modified");
        }
#endif
        CHECK_OK;
    }

    CheckedIteratorBase()
    {
        LIMITED_METHOD_DAC_CONTRACT;
#if defined(_DEBUG)
        m_container = NULL;
#endif
    }

    CheckedIteratorBase(const CONTAINER *container)
    {
        LIMITED_METHOD_CONTRACT;
#if defined(_DEBUG)
        m_container = container;
        __if_exists(CONTAINER::m_revision)
        {
            m_revision = m_container->m_revision;
        }
#endif
    }

    void Resync(const CONTAINER *container)
    {
        LIMITED_METHOD_DAC_CONTRACT;

#if defined(_DEBUG)
        __if_exists(CONTAINER::m_revision)
        {
            m_revision = m_container->m_revision;
        }
#endif
    }
    
#if defined(_DEBUG)
    const CONTAINER *GetContainerDebug() const
    {
        LIMITED_METHOD_CONTRACT;
        return m_container;
    }
#endif

  public:
    CHECK Check() const
    {
        WRAPPER_NO_CONTRACT;
        SUPPORTS_DAC;
        CHECK(CheckRevision());
        CHECK_OK;
    }

    CHECK CheckContainer(const CONTAINER *container) const
    {
        WRAPPER_NO_CONTRACT;
#if defined(_DEBUG)
        CHECK(container == m_container);
#endif
        CHECK_OK;
    }

};


// --------------------------------------------------------------------------------
// Enumerator, Scanner, and Indexer provide a template to produce an iterator 
// on an existing class with a single iteration variable.
//
// The template takes 3 type parameters:
// CONTAINER - type of object being interated on 
// ELEMENT - type of iteration
// BASE - base type of the iteration.  This type must follow the pattern described 
//      by the above Prototypes
// --------------------------------------------------------------------------------

template <typename ELEMENT, typename SUBTYPE>
class Enumerator
{
 private:
    const SUBTYPE *This() const
    {
        return (const SUBTYPE *) this;
    }

    SUBTYPE *This()
    {
        return (SUBTYPE *)this;
    }

  public:

    Enumerator()
    {
    }

    CHECK CheckIndex() const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck());
#endif
        CHECK_OK;
    }

    ELEMENT &operator*() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return This()->Get();
    }
    ELEMENT *operator->() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return &(This()->Get());
    }
    SUBTYPE &operator++()
    {
        PRECONDITION(CheckPointer(This()));

        This()->Next();
        return *This();
    }
    SUBTYPE operator++(int)
    {
        PRECONDITION(CheckPointer(This()));

        SUBTYPE i = *This();
        This()->Next();
        return i;
    }
    bool operator==(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Equal(i);
    }
    bool operator!=(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return !This()->Equal(i);
    }
};

template <typename ELEMENT, typename SUBTYPE>
class Scanner 
{
 private:
    const SUBTYPE *This() const
    {
        return (const SUBTYPE *)this;
    }

    SUBTYPE *This()
    {
        return (SUBTYPE *)this;
    }

  public:

    Scanner()
    {
    }

    CHECK CheckIndex() const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck());
#endif
        CHECK_OK;
    }

    ELEMENT &operator*() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return This()->Get();
    }
    ELEMENT *operator->() const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex());

        return &This()->Get();
    }
    SUBTYPE &operator++()
    {
        PRECONDITION(CheckPointer(This()));

        This()->Next();
        return *This();
    }
    SUBTYPE operator++(int)
    {
        PRECONDITION(CheckPointer(This()));

        SUBTYPE i = *This();
        This()->Next();
        return i;
    }
    SUBTYPE &operator--()
    {
        PRECONDITION(CheckPointer(This()));

        This()->Previous();
        return *This();
    }
    SUBTYPE operator--(int)
    {
        PRECONDITION(CheckPointer(this));

        SUBTYPE i = *This();
        This()->Previous();
        return i;
    }
    bool operator==(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Equal(i);
    }
    bool operator!=(const SUBTYPE &i) const
    {
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return !This()->Equal(i);
    }
};

template <typename ELEMENT, typename SUBTYPE>
class Indexer
{
 private:
    const SUBTYPE *This() const
    {
        return (const SUBTYPE *)this;
    }

    SUBTYPE *This()
    {
        return (SUBTYPE *)this;
    }

  public:

    Indexer()
    {
        LIMITED_METHOD_DAC_CONTRACT;
    }
    
    CHECK CheckIndex() const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck(0));
#endif
        CHECK_OK;
    }

    CHECK CheckIndex(int index) const
    {
#if defined(_DEBUG)
        CHECK(This()->DoCheck(index));
#endif
        CHECK_OK;
    }

    ELEMENT &operator*() const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex(0));

        return *(ELEMENT*)&This()->GetAt(0);
    }
    ELEMENT *operator->() const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex(0));

        return &This()->GetAt(0);
    }
    ELEMENT &operator[](int index) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(This()->CheckIndex(index));
        return *(ELEMENT*)&This()->GetAt(index);
    }
    SUBTYPE &operator++()
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(1);
        return *This();
    }
    SUBTYPE operator++(int)
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        This()->Skip(1);
        return i;
    }
    SUBTYPE &operator--()
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(-1);
        return *This();
    }
    SUBTYPE operator--(int)
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        This()->Skip(-1);
        return i;
    }
    SUBTYPE &operator+=(SCOUNT_T index)
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(index);
        return *This();
    }
    SUBTYPE operator+(SCOUNT_T index) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        i.Skip(index);
        return i;
    }
    SUBTYPE &operator-=(SCOUNT_T index)
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        This()->Skip(-index);
        return *This();
    }
    SUBTYPE operator-(SCOUNT_T index) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        SUBTYPE i = *This();
        i.Skip(-index);
        return i;
    }
    SCOUNT_T operator-(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        
        return This()->Subtract(i);
    }
    bool operator==(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Subtract(i) == 0;
    }
    bool operator!=(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());

        return This()->Subtract(i) != 0;
    }
    bool operator<(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) < 0;
    }
    bool operator<=(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) <= 0;
    }
    bool operator>(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) > 0;
    }
    bool operator>=(const SUBTYPE &i) const
    {
        WRAPPER_NO_CONTRACT;
        PRECONDITION(CheckPointer(This()));
        PRECONDITION(i.Check());
        return This()->Subtract(i) >= 0;
    }
};

#endif  // ITERATOR_H_
