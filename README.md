# Gonepass
Do you love 1Password? Do you also run Linux? Don't you hate that your 1Password data can't be used on Linux? Use Gonepassword!

## Building
Building gonepassword assumes you have the following things installed on your system
* Gtk+3
* Openssl
* Jansson (https://jansson.readthedocs.org/en/2.7/)
* pkg-config
* cmake >= 3.0
* a working C compiler

Right now it's only been tested on Gnome 3, so mileage may vary.

Gonepass uses cmake 3.0! To build, make a build directory and run cmake and then make/make install.

```
$ mkdir build
$ cd build/
$ cmake ..
-- The C compiler identification is GNU 4.9.2
-- The CXX compiler identification is GNU 4.9.2
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.28")
-- checking for module 'gtk+-3.0'
--   found gtk+-3.0, version 3.14.9
-- checking for module 'jansson'
--   found jansson, version 2.7
-- checking for module 'openssl'
--   found openssl, version 1.0.2
-- GSettings schemas will be compiled.
-- GSettings schemas will be compiled in-place.
-- GSettings schemas will be installed into /usr/share/glib-2.0/schemas/
-- Configuring done
-- Generating done
-- Build files have been written to: gonepass/build
$ make
[ 14%] Generating Resources.c
Scanning dependencies of target gonepass
[ 28%] Building C object CMakeFiles/gonepass.dir/appwindow.c.o
[ 42%] Building C object CMakeFiles/gonepass.dir/gonepassapp.c.o
[ 57%] Building C object CMakeFiles/gonepass.dir/item_builder.c.o
[ 71%] Building C object CMakeFiles/gonepass.dir/main.c.o
[ 85%] Building C object CMakeFiles/gonepass.dir/unlockdialog.c.o
[100%] Building C object CMakeFiles/gonepass.dir/Resources.c.o
Linking C executable gonepass
Copying schema gonepass/gonepassapp.gschema.xml to gonepass/build/gsettings
Compiling schemas in folder: gonepass/build/gsettings
[100%] Built target gonepass
$ sudo make install
[100%] Built target gonepass
Install the project...
-- Install configuration: ""
-- Up-to-date: /usr/share/glib-2.0/schemas/gonepassapp.gschema.xml
-- Compiling GSettings schemas
-- Installing: /usr/local/bin/gonepass
```

## Great, now what?
When you start one password point it at your password vault in Dropbox. You should select the folder that ends with `agilekeychain` and type in your master password.

![alt tag](https://raw.github.com/jbreams/gonepass/gh-pages/images/gonepass_unlock.png)

If you want to load up multiple password vaults, just go to the Application menu and click `Load`, it will pop up a new window for selecting another password vault.

The window for selecting password vaults will remember the last password vault it successfully loaded.

Then just browse through your passwords!

![alt tag](https://raw.github.com/jbreams/gonepass/gh-pages/images/gonepass_main.png)

## That's pretty useful, can I update my items?
Not yet, sorry.

## Something isn't working?
Sorry about that, open an issue.
