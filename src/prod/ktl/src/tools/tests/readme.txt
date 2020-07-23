This wiki describes how to build and run the KTL unit tests. For more detailed
information consult http://sharepoint/sites/RVD/Team/Documents/Dual%20mode%20unit%20test%20framework.docx.
That document also describes how to add a new test.

Building:

	It should be sufficient to simply do build -cz under \nt\base\fabric\shared\ktl. The test loader
and binaries should be binplaced into %_NTTREE%\ktltest while the symbols binplaced under 
%_NTTREE%\symbols.pri\ktltest.

	The test loader is built as TestLoader.dll

	Typically there are the following binaries for each test. Note that some tests may only have 
        user components if they can't run in kernel. One example is KHttp test. Below is the list
        for the XYZ unit test.

	* XYZTests.exe           -> console test app
        * XYZUser.dll            -> user mode test dll
        * XYZKernel.inf          -> kernel driver INF installation file
        * XYZKernel.sys          -> kernel driver binary
        * XYZKernelTestInput.xml -> user and kernel test input config file

	You may want to set NT_SIGNCODE = 1 in your razzle environment if your target machine requires
signed binaries.

Running the tests:

	Most tests come in 3 flavors: console, user and kernel. Console tests are exe that
can be run from the command line and easily under the debugger. It is recommended that
this be used for initial debugging. User flavor is built as a user mode dll for testing in
user mode while kernel flavor is built as a kernel driver for testing in kernel mode. The user 
and kernel flavors are used for automation under TAEF. 


	To setup test machine for running tests:

		* To setup TAEF copy the entire directory tree under 
                  \\wexrelease\Tools\SPARTA\TAEF\\v2.9.3\Binaries\Release\x64\TestExecution
                  to a local directory on your test machine. 

                * Copy WdfCoInstaller01009.dll to that local directory. Typically it should be somewhere
                  under %windir% however you can always find it as part of the WDK which is at
                  http://www.microsoft.com/en-us/download/details.aspx?displaylang=en&id=11800

		* From your %_NTTREE%\ktltest directory, copy testloader.dll to that local
                  directory

                * Next copy all of the files related to your test to that local directory

        To run the tests (assuming test name is XYZ):

                * Run console version for debugging or quick sanity check:  

                      XYZTests.exe

                * Run the user version under TAEF:

                    Te.exe testloader.dll /p:target=user /p:dll=XYZUser /p:c=RunTests 
                                          /p:configfile=Table:XYZTestInput.xml#Test

                * Run the kernel version under TAEF:

                    Te.exe testloader.dll /p:target=kernel /p:driver=XYZkernel /p:c=RunTests 
                                          /p:configfile=Table:XYZTestInput.xml#Test

                * Run both kernel and user version under TAEF:

                    Te.exe testloader.dll /p:target=both /p:driver=XYZkernel /p:dll=XYZUser /p:c=RunTests 
                                          /p:configfile=Table:XYZTestInput.xml#Test


Useful things for running tests:

	* EnableVerifier.cmd - this batch file will enable driver verifier for all kernel mode test drivers
