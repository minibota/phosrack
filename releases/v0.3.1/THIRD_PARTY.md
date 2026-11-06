# Third-party components in the Windows packages

The Windows packaging workflow uses the Qt 6.8.3 MSVC 2022 x64 SDK. The exact
application source revision and completed build records are recorded in this
version's README after the actual 0.3.1 workflows finish.

Phosrack source, build instructions and deployment scripts are available in this
repository under GPL-3.0-or-later. The portable package permits replacement of its
Qt DLLs. Qt modules and their dependencies retain their respective licenses.

Qt 6.8.3 corresponding source:
https://download.qt.io/archive/qt/6.8/6.8.3/single/qt-everywhere-src-6.8.3.tar.xz

Qt open-source licensing and source-build documentation:
https://doc.qt.io/qt-6.8/opensourcelicense.html
https://doc.qt.io/qt-6.8/windows-building.html

The Windows packages contain app-local Microsoft Visual C++ runtime DLLs from the
installed MSVC SDK. Microsoft retains their redistribution license.

Equalizer APO is a separate optional installation and is not bundled.
https://sourceforge.net/projects/equalizerapo/

Linux RPM/DEB packages use system libraries. The Flatpak depends on the KDE runtime;
it does not embed a private copy of the Spotify client or Spotify audio recordings.
