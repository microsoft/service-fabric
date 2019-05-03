// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Linq;
    using System.Reflection;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    static class Utility
    {
        public static PropertyInfo GetPropertyInfo<T>(T obj, string name)
        {
            PropertyInfo pi = typeof(T).GetProperties().Where(p => p.Name == name).First();
            if (pi == null)
            {
                Assert.Fail("unable to find property: " + name);
            }
            return pi;
        }

        public static void ValidateThatSetPropertyOperationSucceeds<TInstance>
            (TInstance instance, 
             string propertyName, 
             object val)
        {
            PropertyInfo pi = GetPropertyInfo(instance, propertyName);

            pi.SetValue(instance, (object)val, null);

            Assert.AreEqual(val.ToString(), pi.GetValue(instance, null).ToString());
        }

        public static void ValidateThatGetPropertyOperationSucceeds<TInstance>
            (TInstance instance,
             string propertyName)
        {
            PropertyInfo pi = GetPropertyInfo(instance, propertyName);

            pi.GetValue(instance, null);            
        }

        public static void ValidateThatSetPropertyOperationFails<TInstance>
                (TInstance instance,
                 string propertyName,
                 object val)
        {
            PropertyInfo pi = GetPropertyInfo(instance, propertyName);

            try
            {
                pi.SetValue(instance, (object)val, null);
                Assert.Fail("property was settable when it should not be " + propertyName);
            }
            catch (TargetInvocationException ex)
            {
                Assert.IsTrue(ex.InnerException.GetType() == typeof(InvalidOperationException));
            }
        }

        public static void ValidateThatGetPropertyOperationFails<TInstance>
            (TInstance instance,
             string propertyName)
        {
            PropertyInfo pi = GetPropertyInfo(instance, propertyName);

            try
            {
                pi.GetValue(instance, null);
                Assert.Fail("property was gettable when it should not be " + propertyName);
            }
            catch (TargetInvocationException ex)
            {
                Assert.IsTrue(ex.InnerException.GetType() == typeof(InvalidOperationException));
            }
        }

        public static void TestPropertySetterThrowsArgumentNullException<T>
            (T obj, string propertyName)
        {
            PropertyInfo pi = GetPropertyInfo(obj, propertyName);

            string[] values = new string[] { string.Empty, null };

            foreach (var item in values)
            {
                try
                {
                    pi.SetValue(obj, item, null);
                    Assert.Fail("Setting property " + propertyName + " succeeded when it should throw arg null ex");
                }
                catch (TargetInvocationException ex)
                {
                    Assert.IsTrue(ex.InnerException.GetType() == typeof(ArgumentNullException));
                }    
            }
        }
    }
}