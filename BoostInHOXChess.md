# Introduction #

This page describes how Boost libraries are installed to be used in HOXChess

## UPGRADE boost to 1.41.0 HOWTO ##
See the same procedure as that of 1.38.0 below.

## UPGRADE boost to 1.38.0 HOWTO ##

  * Download the SOURCE: **boost\_1\_38\_0.tar.bz2**
  * Also, download binary **bjam.exe** from the same place
    * I put it under folder _C:\Tmp\boost\_1\_38\_0\bjam_

  * Build the **bcp.exe** executable as follows:
    * _C:\Tmp\boost\_1\_38\_0\tools\bcp>..\..\bjam\bjam.exe_
    * The above command will create **.\dist\bin\bcp.exe**

  * Read: http://www.boost.org/doc/libs/1_38_0/tools/bcp/bcp.html
  * Under _C:\Tmp\boost\_1\_38\_0_
    * .\dist\bin\bcp.exe shared\_ptr enable\_shared\_from\_this tokenizer algorithm date\_time functional intrusive\_ptr format weak\_ptr c:\tmp
    * The above commands will copy all dependent files to c:\tmp\...
    * **Notes**: Here are the new headers added to support **Asio** library
      * _date\_time_
      * _functional_
      * _intrusive\_ptr_
      * _weak\_ptr_

## UPGRADE boost to 1.36.0 HOWTO ##

  * Download the SOURCE: **boost\_1\_36\_0.7z**
  * Also, download binary "bjam.exe" from the same place
    * I put it under folder "C:\Users\HPhan\Desktop\boost\_1\_36\_0\bjam"

  * Build the "bcp.exe" executable as follows:
    * C:\Users\HPhan\Desktop\boost\_1\_36\_0\tools\bcp>..\..\bjam\bjam.exe
    * The above command will create ".\dist\bin\bcp.exe"

  * Read: http://www.boost.org/doc/libs/1_36_0/tools/bcp/bcp.html
  * Under C:\Users\HPhan\Desktop\boost\_1\_36\_0>
    * .\dist\bin\bcp.exe shared\_ptr enable\_shared\_from\_this tokenizer algorithm c:\tmp
    * The above commands will copy all dependent files to c:\tmp\...

## MINIMAL Boost installation HOWTO ( for boost/shared\_ptr.hpp ) ##

  * Download the SOURCE of boosts library (I used **boost\_1\_34\_1**)

  * We need the "bcp" tool to extract all dependent files for <boost/shared\_ptr.hpp>
    * Need to download binary "bjam.exe" from sourceforge.net
    * I put it under folder "C:\Users\HPhan\Desktop\boost\_1\_34\_1\bjam"
  * Read instructions from from the local downloaded distribution:
    * C:\Users\HPhan\Desktop\boost\_1\_34\_1\tools\build\index.html

  * Build the "bcp.exe" executable as follows:
    * C:\Users\HPhan\Desktop\boost\_1\_34\_1\tools\bcp>..\..\bjam\bjam.exe
    * The above command will create ".\dist\bin\bcp.exe"

  * Under C:\Users\HPhan\Desktop\boost\_1\_34\_1>
    * .\dist\bin\bcp.exe boost\shared\_ptr.hpp c:\tmp
    * .\dist\bin\bcp.exe boost\enable\_shared\_from\_this.hpp c:\tmp
  * The above commands will copy all dependent files to c:\tmp\...

## References ##
  * http://www.boost.org/tools/bcp/bcp.html
  * http://boost.org/boost-build2/index.html
  * http://marc.info/?l=boost-users&m=118835450521043&w=2


---
