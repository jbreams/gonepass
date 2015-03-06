# Gonepass
Do you love 1Password? Do you also run Linux? Don't you hate that your 1Password is trapped on on-linux? Use Gonepassword!

## Building
Building gonepassword assumes you have the following things installed on your system
* Gtk+3
* Openssl
* Jansson (https://jansson.readthedocs.org/en/2.7/)
* pkg-config

## Great, now what?
When you start one password point it at your password vault in Dropbox. You should select the folder that ends with `agilekeychain` and type in your master password.

The notes section of each item is a regular text box and will automatically expand if there are any notes present. You can copy and paste from notes like any other text box.

To copy any passwords, just right click on them. Whatever the value of that row will be copied to the clipboard.

If you want to load up multiple password vaults, just go to the Application menu and click `Load`, it will pop up a new window for selecting another password vault.

The window for selecting password vaults will remember the last password vault it successfully loaded.

## That's pretty useful, can I update my items?
Not yet, sorry.

## Something isn't working?
Sorry about that, open an issue.
