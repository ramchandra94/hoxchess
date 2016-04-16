

# Introduction #

This page contains the instruction on how to build [HOXChess](http://hoxchess.googlecode.com/) based on the _latest_ version of the source code.
The page's content is based entirely on my experience working on the project.

# Requirements #
Starting from version _0.7.5.0_ (released on Jun 09, 2009), the project can be built under Windows, Linux, and Mac OS X thanks to the cross platform portability of [wxWidgets](http://www.wxwidgets.org).

  * [wxWidgets](http://www.wxwidgets.org/) 2.9.0 (the not-yet-released **branch** version)
    * I tested the 2.9.0 branch on Nov 30, 2009
    * For Ubuntu, we need the **wxGTK** port.
  * [Boost C++ 1\_41\_0 Libraries](http://www.boost.org) Headers only. Already included in the source code
  * [Asio 1.4.1 C++ Library](http://www.think-async.com/Asio/WebHome) Headers only. Already included in the source code
  * Operating Systems
    * Under Windows
      * Windows XP or Vista
      * Visual C++ 2008 Express Edition
    * Under Linux
      * Any Linux distribution that supports _g++_ and _make_ packages. I have tried on the following:
        * [CentOS](http://www.centos.org/) 4.7 and 5.4, or
        * [Ubuntu](http://www.ubuntu.com/) 9.10
    * Under Mac OS X using **Xcode**

# Detailed Instructions under Windows #
  * wxWidgets: Put the libraries and headers under _C:\wxMSW-2.9.0_
    * If your wxWidgets' location is different, then you need to adjust the appropriate settings of _hox\_Client.vcproj_.
  * Open the project **hox\_Client** under the solution _hox\_Project/hox\_Project.sln_ inside Visual Studio C++ and build from there.

# Detailed Instructions under Linux #
  * Follow [the instructions here](wxWidgetsInHOXChess.md) to build **wxGTK** libraries
  * You will need to install two packages: **libboost-dev** and **libasio-dev**
    * Or you can use the packages already included in HOXChess 's source code:
```
  $ CPLUS_INCLUDE_PATH=../lib/boost_1_41_0:../lib/asio-1.4.1/include  make
```
  * Build AI Engine Plugins and the main _hoxchess_ executable as follows:
```
  $ cd .../hox_Project/plugins/
  $ ./run.sh
  $ cd .../hox_Project/hox_Client/
  $ make
  $ ./hoxchess
```

# Detailed Instructions under Xcode in Mac OS X 10.6 #
  * Follow [the instructions here](wxWidgetsInHOXChess.md) to build **wxMac** libraries
```
  $ make build-release
  $ cd build-release
  $ ../configure –prefix=/opt/wxWidgets-2.9.0 –disable-compat28 –without-libtiff –disable-shared
  $ make
  $ sudo make install
```
  * Then, simply open the **Xcode** project under _.../hox\_Project/HOXChess.xcodeproj_ and build