[DAEMONS]
lircd and lircmd are daemons. You should start them in some init script
depending on your system. There are some example scripts for different
distributions in the contrib directory. lircmd has to be started after
lircd as it connects to the socket lircd provides.

If you start lircd or lircmd from your shell prompt you will usually get
back immediately to the prompt. Often people think that the program has
died. But this is not an error. lircd and lircmd are daemons. Daemons
always run in background.

