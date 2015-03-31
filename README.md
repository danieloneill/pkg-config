# pkg-config
pkg-config using C++/QtCore

##Build##

```
mkdir build
cd build
qmake ..
```

Or build it in-tree if you want, I'm not your mom.

##Requirements##

A C++ compiler and at least QtCore with development headers and qmake.

##Usage##

```
pkg-config [-A <package config directory>]
           [-D <package config directory>]
           [-L]
           [--atleast-pkgconfig-version <version>]
           [--version]
           [--exists <package name>]
           [--short-errors]
           [--print-warnings]
           [--print-errors]
           [--cflags <package name>]
           [--libs <package name>]
```

* **-A** - Add a .pc file search path for the current user.
* **-D** - Delete a .pc file search path for the current user.
* **-L** - List .pc file search paths for the current user.
* **-h** - Usage information.
* **--atleast-pkgconfig-version** - Returns 0 if we meet the version requirement, -1 otherwise.
* **--version** - The version of this pkg-config.
* **--exists** - Returns 0 if the specified package exists, -1 otherwise.
* **--short-errors** - Print errors in short form.
* **--print-warnings** - Print warnings encountered.
* **--print-errors** - Print errors encountered.
* **--cflags** - Output the cflags specified by the package.
* **--libs** - Output the ldflags (libs) as specified by the package.

##Other Stuff##

* **Is this XDG/Freedesktop compliant?**
* Almost certainly not.

* **Does it work?**
* Almost every time.

* **Why is it here?**
* Because I wrote it, and I wanted to share it.

* **What is it for?**
* A defunct project where Qt was the ubiquitous framework. Glib wasn't installed, and building it specifically for a single tool was silly.

* **Does pkg-config REALLY need to link against Qt?**
* No, in fact a saner solution would be pkg-config written in Python, Perl, LUA, or similar. A compiled tool for this task is overkill.

