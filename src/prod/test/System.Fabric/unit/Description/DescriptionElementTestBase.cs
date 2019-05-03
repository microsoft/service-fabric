// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    /// <summary>
    /// Base class for testing description elements
    /// </summary>
    /// <typeparam name="TDescription"></typeparam>
    /// <typeparam name="TInfo"></typeparam>
    internal abstract class DescriptionElementTestBase<TDescription, TInfo>
    {
        /// <summary>
        /// Function that takes the info object and returns the corresponding description for it
        /// Usually the .CreateFromNative function
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        public abstract TDescription Factory(TInfo info);

        /// <summary>
        /// Returns the default info object which is convertable to the default description object
        /// </summary>
        /// <returns></returns>
        public abstract TInfo CreateDefaultInfo();

        /// <summary>
        /// Returns the defautl description object 
        /// </summary>
        /// <returns></returns>
        public abstract TDescription CreateDefaultDescription();

        /// <summary>
        /// The comparer for this description object
        /// </summary>
        /// <param name="expected"></param>
        /// <param name="actual"></param>
        public abstract void Compare(TDescription expected, TDescription actual);

        /// <summary>
        /// Use this to author tests
        /// Tests take in a function that will transform the Default Info object (which represents the native object)
        /// They will then parse the transformed default info object to the corresponding description object
        /// The test method will then validate this with a transform default description object
        /// </summary>
        public void ParseTestHelper(Action<TInfo> setupTransform, Action<TDescription> expectedTransform)
        {
            TInfo initial = this.CreateDefaultInfo();

            if (setupTransform != null)
            {
                setupTransform(initial);
            }

            TDescription actual = this.Factory(initial);

            TDescription expected = this.CreateDefaultDescription();

            if (expectedTransform != null)
            {
                expectedTransform(expected);
            }

            this.Compare(expected, actual);
        }

        public void ParseTestHelperEx(Action<TInfo> setupTransform, Func<TDescription> expectedFactory)
        {
            TInfo initial = this.CreateDefaultInfo();

            if (setupTransform != null)
            {
                setupTransform(initial);
            }

            TDescription actual = this.Factory(initial);

            TDescription expected = expectedFactory();

            this.Compare(expected, actual);
        }

        public void ParseTestHelperWithCustomValidator(Action<TInfo> setupTransform, Action<TDescription> validator)
        {
            TInfo initial = this.CreateDefaultInfo();

            if (setupTransform != null)
            {
                setupTransform(initial);
            }

            TDescription actual = this.Factory(initial);

            validator(actual);
        }

        /// <summary>
        /// Use this to test error cases
        /// Do something to the setupTransform that should cause the parse function to throw
        /// </summary>
        /// <typeparam name="TException"></typeparam>
        /// <param name="setupTransform"></param>
        public void ParseErrorTestHelper<TException>(Action<TInfo> setupTransform) where TException : Exception
        {
            TInfo setup = this.CreateDefaultInfo();

            if (setupTransform != null)
            {
                setupTransform(setup);
            }

            TestUtility.ExpectException<TException>(() => this.Factory(setup));
        }

        public void ParsePassingNullEmptyStringThrowsErrorTestHelper(Action<TInfo, string> setter)
        {
            this.ParseErrorTestHelper<ArgumentNullException>((info) => setter(info, string.Empty));
            this.ParseErrorTestHelper<ArgumentNullException>((info) => setter(info, null));
        }
    }
}