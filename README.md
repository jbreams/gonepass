# Gonepass
Do you love 1Password? Do you also run Linux? Don't you hate that your 1Password data can't be used on Linux? Use Gonepassword!

## Building
Building gonepassword assumes you have the following things installed on your system
* Gtk+3
* Openssl
* Jansson (https://jansson.readthedocs.org/en/2.7/)
* pkg-config
* GNU make
* a working C compiler

Right now it's only been tested on Gnome 3, so milage may vary.

A real build system is coming, I promise! For now just run make, and it will produce an executable called gonepassapp.

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
