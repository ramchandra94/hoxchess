| **HOXChess** is a _cross-platform_, _open source_ [Xiangqi](http://en.wikipedia.org/wiki/Xiangqi) (aka. Chinese Chess) client, capable of working with multiple online Xiangqi sites. It is a _traditional desktop_ application, developed mainly in C++ using [wxWidgets](http://www.wxwidgets.org/). Two servers are supported so far: **PlayXiangqi** and **Chesscape**. There are a number of Artificial Intelligent (AI) engines included. Recently, the App has also been ported to iPhone/iPod touch using Apple's native iPhone SDK. | ![![](http://hoxchess.googlecode.com/svn/trunk/CChess/screenshots/1.0/HOXChess-icon.png)](http://www.playxiangqi.com/cchess/screenshots/1.0/HOXChess-1.0.png) |
|:---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|:--------------------------------------------------------------------------------------------------------------------------------------------------------------|

### Latest News ###
#### Sep 23, 2014 ####
  * Moved the source code of the iOS client to [GitHub](https://github.com).
    * See: https://github.com/huygithub/hoxChess
  * **Note**: I will gradually migrate the entire project to GitHub.

#### Oct 27, 2010 ####
  * Released a new update iPhone/iPod touch HOXChess-1.1 to the AppStore.
    * Fixed the sound effects issue in iOS 4.0

#### Mar 25, 2010 ####
| The iPhone/iPod touch HOXChess-1.0 App is approved by the AppStore. | [![](http://www.playxiangqi.com/locale/en_US/App_Store_Badge_240x120.png)](http://itunes.apple.com/app/hoxchess/id363513274?mt=8) |
|:--------------------------------------------------------------------|:----------------------------------------------------------------------------------------------------------------------------------|

#### Mar 23, 2010 ####
  * Merged the iPhone/iPod touch source code from [NevoChess](http://nevochess.googlecode.com) to this project.
    * Submitted to the application to Apple's AppStore under the same name **HOXChess**.

#### Dec 23, 2009 ####
  * Released version **1.0.0** with binary packages for Windows, Mac OS X, and Linux
  * The _new features_ are:
    * Implement a new Click-N-Click move mode
    * Implement the 'highlight' of a next position when moving a Piece
    * Implement a Popup Private Chat window
    * Enable Copy/Paste option with the Chat window
    * Add a new 3D Piece set from [Eurasia-Chess](http://www.eurasia-chess.com)
  * Some fixes are:
    * Fix the King-facing-King bug in the Referee
    * Fix the crash if an AI Plugin's name is invalid
  * The Linux binary package has been tested on the following distributions:
    * **CentOS 5.4** (32-bit)
    * **Ubuntu/Kubuntu 9.10** (32-bit and 64-bit)
    * **Fedora 12** (32-bit)
    * **openSUSE 11.2** (32-bit)
    * **Debian 5.0.3** (32-bit)
    * **Mandriva 2010.0**
    * **Gentoo 2008.0** (32-bit)

#### Jul 25, 2009 ####
  * Released version **0.8.0.0** with binary packages for Windows, Mac OS X, and Linux
  * The _new features_ are:
    * Add **Play with yourself** (or _Human vs. Human_) feature
    * Support **Difficulty Level** when playing against AI
    * Add **Author**-name and Web site of the AI's author on the Practice table
    * Add the basic **Check for Updates** feature
    * AI **Folium**: synced up the source code with http://folium.googlecode.com
    * Add the **Clear** button in both the _Activities_ and _Messages_ dialogs
    * Improved localization in Chinese, French, and Vietnamese with help from native speakers
  * Some fixes are:
    * Mac OS X: Optimize the Move-handling code to speed up image drawing.
    * Allow to open the Practice table from everywhere
    * Fix the message-being-cut-off issue when _username_ contains a space
    * Fixed the _blank-Chat-window_ issue when  the window is full
    * Fix the crash when a _List-of-tables_ is displayed while the Site is closed
  * The Linux binary package has been tested on the following distributions:
    * **CentOS 5.3** (32-bit)
    * **Ubuntu/Kubuntu 9.04** (32-bit and 64-bit)
    * **Fedora 11**   (32-bit)
    * **openSUSE 11** (32-bit)
    * **Debian 5.0.2** (32-bit)
    * **Gentoo 2008.0** (32-bit)
    * **Mandriva 2009.1** (KDE, 32-bit)


[Old Release Notes can be found here...](Releases.md)