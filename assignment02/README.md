## Printing debug messages
* Make sure that the first line of Kbuild is not commented
* Use the dprintk() macro. Use the syntax `dprintk("[uart] Some Message\n");`.
* Check messages with the command `dmesg | "uart debug"`. Note that using `[]` here won't work.
* Clear debug messages with `sudo dmesg --clear`
