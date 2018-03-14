// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <windows.h>

#include "Common/WaitHandle.h"
#include "ComUnknownBase.h"
#include "ComPointer.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    class ComUnknownBaseTest
    {
    };

    GUID IID_IPlant = {0xc4277e5d,0xf904,0x448c,{0x9d,0xb3,0x70,0xe1,0x62,0x00,0xb6,0x42}};
    GUID IID_IFruit = {0x00144491,0x3edb,0x4843,{0x97,0x28,0xf2,0xc0,0x12,0xdc,0x94,0x21}};
    GUID IID_IColor = {0x7edd768d,0xee25,0x4ac2,{0x94,0x10,0x0a,0x01,0x46,0xf0,0xe3,0xd3}};
    GUID IID_ComOrange = {0x1131c218,0x6555,0x453b,{0xa4,0xf7,0xc9,0x77,0x57,0xf8,0x3a,0x7b}};

    namespace Implementor
    {
        enum Enum { ComPlant, ComFruit1, ComFruit2, ComOrange, ComEggplant1, ComEggplant2 };
    }

    //
    // Sample interfaces.
    //
    // ComUnknownBase does arithmetic on the pointer, so we need to verify that the methods we are
    // calling are actually going to the method we expect.  We do this by validating that the
    // method returns the correct implementor and IID value corresponding to the class and
    // interface respectively that we are calling.
    //
    class IPlant : public IUnknown
    {
    public:
        virtual HRESULT GetPlantImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid) = 0;
    };

    class IFruit : public IPlant
    {
    public:
        virtual HRESULT GetFruitImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid) = 0;
    };

    class IColor : public IUnknown
    {
    public:
        virtual HRESULT GetColorImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid) = 0;
    };

    //
    // Implementations of the interfaces
    //

    //
    // Implements a single simple interface.
    // Supports subclassing by other implementations.
    //
    class ComPlant : public IPlant, protected ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComPlant,
            IID_IPlant,
            IPlant)
    public:
        virtual HRESULT GetPlantImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComPlant;
            *iid = IID_IPlant;
            return S_OK;
        }
    };

    //
    // Implements two related interfaces explicitly.
    //
    class ComFruit1 : public IFruit, private ComUnknownBase
    {
        COM_INTERFACE_LIST2(
            ComFruit1,
            IID_IPlant,
            IPlant,
            IID_IFruit,
            IFruit)
    public:
        virtual HRESULT GetPlantImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComFruit1;
            *iid = IID_IPlant;
            return S_OK;
        }

        virtual HRESULT GetFruitImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComFruit1;
            *iid = IID_IFruit;
            return S_OK;
        }
    };

    //
    // Implements two related interfaces by delegating one to a base class.
    //
    class ComFruit2 : public IFruit, public ComPlant
    {
        COM_INTERFACE_AND_DELEGATE_LIST(
            ComFruit2,
            IID_IFruit,
            IFruit,
            ComPlant)

    public:
        virtual HRESULT GetPlantImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            return this->ComPlant::GetPlantImplementor(implementor, iid);
        }

        virtual HRESULT GetFruitImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComFruit2;
            *iid = IID_IFruit;
            return S_OK;
        }
    };

    //
    // Implements both related and unrelated interfaces.
    //
    class ComOrange : public IFruit, public IColor, private ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(ComOrange)
        COM_INTERFACE_ITEM(IID_IUnknown, IFruit)
        COM_INTERFACE_ITEM(IID_IPlant, IPlant)
        COM_INTERFACE_ITEM(IID_IFruit, IFruit)
        COM_INTERFACE_ITEM(IID_IColor, IColor)
        COM_INTERFACE_ITEM(IID_ComOrange, ComOrange)
        END_COM_INTERFACE_LIST()

    public:
        virtual HRESULT GetPlantImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComOrange;
            *iid = IID_IPlant;
            return S_OK;
        }

        virtual HRESULT GetFruitImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComOrange;
            *iid = IID_IFruit;
            return S_OK;
        }

        virtual HRESULT GetColorImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComOrange;
            *iid = IID_IColor;
            return S_OK;
        }

        HRESULT GetOrangeImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComOrange;
            *iid = IID_ComOrange;
            return S_OK;
        }
    };

    //
    // Implements two unrelated interfaces explicitly.
    //
    class ComEggplant1 : public IPlant, public IColor, private ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(ComEggplant1)
        COM_INTERFACE_ITEM(IID_IUnknown, IPlant)
        COM_INTERFACE_ITEM(IID_IPlant, IPlant)
        COM_INTERFACE_ITEM(IID_IColor, IColor)
        END_COM_INTERFACE_LIST()

    public:
        virtual HRESULT GetPlantImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComEggplant1;
            *iid = IID_IPlant;
            return S_OK;
        }

        virtual HRESULT GetColorImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComEggplant1;
            *iid = IID_IColor;
            return S_OK;
        }
    };

    //
    // Implements two unrelated interfaces by delegating one to a base class.
    //
    class ComEggplant2 : public ComPlant, public IColor
    {
        COM_INTERFACE_AND_DELEGATE_LIST(
            ComEggplant2,
            IID_IColor,
            IColor,
            ComPlant)

    public:
        virtual HRESULT GetColorImplementor(
            __out Implementor::Enum * implementor,
            __out GUID * iid)
        {
            *implementor = Implementor::ComEggplant2;
            *iid = IID_IColor;
            return S_OK;
        }
    };

    void CheckPlant(
        __in ComPointer<IUnknown> & unknown,
        __in Implementor::Enum expectedImplementor)
    {
        Implementor::Enum actualImplementor;
        GUID actualIID;
        ComPointer<IPlant> plant(unknown, IID_IPlant);

        plant->GetPlantImplementor(&actualImplementor, &actualIID);
        VERIFY_ARE_EQUAL(actualImplementor, expectedImplementor);
        VERIFY_ARE_EQUAL(actualIID, IID_IPlant);
    }

    void CheckFruit(
        __in ComPointer<IUnknown> & unknown,
        __in Implementor::Enum expectedImplementor)
    {
        Implementor::Enum actualImplementor;
        GUID actualIID;
        ComPointer<IFruit> fruit(unknown, IID_IFruit);

        fruit->GetFruitImplementor(&actualImplementor, &actualIID);
        VERIFY_ARE_EQUAL(actualImplementor, expectedImplementor);
        VERIFY_ARE_EQUAL(actualIID, IID_IFruit);
    }

    void CheckColor(
        __in ComPointer<IUnknown> & unknown,
        __in Implementor::Enum expectedImplementor)
    {
        Implementor::Enum actualImplementor;
        GUID actualIID;
        ComPointer<IColor> color(unknown, IID_IColor);

        color->GetColorImplementor(&actualImplementor, &actualIID);
        VERIFY_ARE_EQUAL(actualImplementor, expectedImplementor);
        VERIFY_ARE_EQUAL(actualIID, IID_IColor);
    }

    void CheckOrange(
        __in ComPointer<IUnknown> & unknown,
        __in Implementor::Enum expectedImplementor)
    {
        Implementor::Enum actualImplementor;
        GUID actualIID;
        ComPointer<ComOrange> orange(unknown, IID_ComOrange);

        orange->GetOrangeImplementor(&actualImplementor, &actualIID);
        VERIFY_ARE_EQUAL(actualImplementor, expectedImplementor);
        VERIFY_ARE_EQUAL(actualIID, IID_ComOrange);
    }

    void CheckNotFruit(__in ComPointer<IUnknown> & unknown)
    {
        ComPointer<IFruit> fruit(unknown, IID_IFruit);
        VERIFY_IS_FALSE(fruit);
    }

    void CheckNotColor(__in ComPointer<IUnknown> & unknown)
    {
        ComPointer<IColor> color(unknown, IID_IColor);
        VERIFY_IS_FALSE(color);
    }

    void CheckNotOrange(__in ComPointer<IUnknown> & unknown)
    {
        ComPointer<ComOrange> orange(unknown, IID_ComOrange);
        VERIFY_IS_FALSE(orange);
    }

    BOOST_FIXTURE_TEST_SUITE(ComUnknownBaseTestSuite,ComUnknownBaseTest)

    BOOST_AUTO_TEST_CASE(PlantTest)
    {
        auto unknown = make_com<ComPlant,IUnknown>();

        CheckPlant(unknown, Implementor::ComPlant);
        CheckNotFruit(unknown);
        CheckNotColor(unknown);
        CheckNotOrange(unknown);
    }

    BOOST_AUTO_TEST_CASE(Fruit1Test)
    {
        auto unknown = make_com<ComFruit1,IUnknown>();

        CheckPlant(unknown, Implementor::ComFruit1);
        CheckFruit(unknown, Implementor::ComFruit1);
        CheckNotColor(unknown);
        CheckNotOrange(unknown);
    }

    BOOST_AUTO_TEST_CASE(Fruit2Test)
    {
        auto fruit = make_com<ComFruit2>();
        ComPointer<IUnknown> unknown(fruit, IID_IUnknown);

        CheckPlant(unknown, Implementor::ComPlant);
        CheckFruit(unknown, Implementor::ComFruit2);
        CheckNotColor(unknown);
        CheckNotOrange(unknown);
    }

    BOOST_AUTO_TEST_CASE(OrangeTest)
    {
        auto orange = make_com<ComOrange>();
        ComPointer<IUnknown> unknown(orange, IID_IUnknown);

        CheckPlant(unknown, Implementor::ComOrange);
        CheckFruit(unknown, Implementor::ComOrange);
        CheckColor(unknown, Implementor::ComOrange);
        CheckOrange(unknown, Implementor::ComOrange);
    }

    BOOST_AUTO_TEST_CASE(Eggplant1Test)
    {
        auto eggplant = make_com<ComEggplant1,IColor>();
        ComPointer<IUnknown> unknown(eggplant, IID_IUnknown);

        CheckPlant(unknown, Implementor::ComEggplant1);
        CheckNotFruit(unknown);
        CheckColor(unknown, Implementor::ComEggplant1);
        CheckNotOrange(unknown);
    }

    BOOST_AUTO_TEST_CASE(Eggplant2Test)
    {
        auto eggplant = make_com<ComEggplant2,IColor>();
        ComPointer<IUnknown> unknown(eggplant, IID_IUnknown);

        CheckPlant(unknown, Implementor::ComPlant);
        CheckNotFruit(unknown);
        CheckColor(unknown, Implementor::ComEggplant2);
        CheckNotOrange(unknown);
    }

#ifndef  PLATFORM_UNIX  //LINUXTODO: move COM runtime related code to separate file
    static DWORD __stdcall RunSTAThreadCallback(void *)
    {
        VERIFY_SUCCEEDED(::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
        VERIFY_IS_FALSE(ComUtility::CheckMTA().IsSuccess());
        ::CoUninitialize();
        return 0;
    }

    BOOST_AUTO_TEST_CASE(STATest)
    {
        VERIFY_IS_TRUE(ComUtility::CheckMTA().IsSuccess());
        VERIFY_SUCCEEDED(::CoInitializeEx(NULL, COINIT_MULTITHREADED));
        VERIFY_IS_TRUE(ComUtility::CheckMTA().IsSuccess());

        HANDLE staThread = CreateThread(NULL, 0, RunSTAThreadCallback, NULL, 0, NULL);
        VERIFY_IS_NOT_NULL(staThread);
#if !defined(PLATFORM_UNIX)
        WaitForSingleObject(staThread, INFINITE);
#else
        pthread_join((pthread_t)staThread, NULL);
#endif
        CloseHandle(staThread);
        ::CoUninitialize();
    }
#endif
    BOOST_AUTO_TEST_SUITE_END()
}
