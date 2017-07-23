[NAME]
irw - sends data from Unix domain socket to stdout


[DESCRIPTION]

irw will connect to any Unix domain socket and will print the data that it
receives to stdout. If you don't give it a socket name argument it will watch
\//var/run/lirc/lircd. Useful for debugging.

