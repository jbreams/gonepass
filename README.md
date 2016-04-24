# Gonepass
Do you love 1Password? Do you also run Linux? Don't you hate that your 1Password data can't be used on Linux? Use Gonepassword!

## Building
Building gonepassword assumes you have the following things installed on your system
* Gtk+3
* Gtkmm 3.0
* Openssl
* pkg-config
* cmake >= 3.0
* a working C compiler

Gonepass uses cmake 3.0! To build, make a build directory and run cmake and then make/make install.

```
 $ cmake ..
-- The C compiler identification is GNU 5.3.0
-- The CXX compiler identification is GNU 5.3.0
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
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.29.1")
-- Checking for module 'gtkmm-3.0'
--   Found gtkmm-3.0, version 3.20.0
-- Checking for module 'openssl'
--   Found openssl, version 1.0.2g
-- Configuring done
-- Generating done
-- Build files have been written to: /home/jreams/Documents/git/gonepass/build

 $ make
[ 20%] Generating Resources.c
Scanning dependencies of target gonepass
[ 40%] Building CXX object CMakeFiles/gonepass.dir/main.cpp.o
[ 60%] Building CXX object CMakeFiles/gonepass.dir/keychain.cpp.o
[ 80%] Building C object CMakeFiles/gonepass.dir/Resources.c.o
[100%] Linking CXX executable gonepass
[100%] Built target gonepass
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
