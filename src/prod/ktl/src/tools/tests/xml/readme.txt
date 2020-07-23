The XML tests require some setup on a test machine to be run successfully

1. A test file must be copied a specific location on the test machine

     copy %_NTTREE%\ktltest\testfile0.xml    c:\temp\Ktl\XmlTest\testfile0.xml

2. The console version of the test will automatically run XMLDiff on the input and output files. 
   XMLDiff.exe application is assumed to be located in the same directory as the test is run. Note
   that .Net needs to be installed for xmldiff to work.

     copy %_NTTREE%\ktltest\xmldiff.exe    .

3. To run the user flavor of the test DoXmlDiff.cmd must be in the current directory

     copy %_NTTREE%\ktltest\DoXmlDiff.cmd    .

4. To launch the XML tests enter the following command. Note c: is the same drive used in step 1.

     xmltests XmlBasicTest c:


5. To launch the DOM tests enter the following command

    xmltests DomBasicTest