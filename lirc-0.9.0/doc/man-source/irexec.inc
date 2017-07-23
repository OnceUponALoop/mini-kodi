[NAME]
irexec - run programs with one button press

[DESCRIPTION]
This program lets you execute arbitrary commands on an IR signal decoded
by lircd, the LIRC daemon. You can give irexec a command line parameter
which has to be a name of a valid config file. If no command line
parameters are given irexec reads the default config file which is
usually ~/.lircrc.

If irexec executes a program it will wait until this program terminates.
So append a '&' to the command string if you don't want that.

The config string consists of the command to be run.

[OPTIONS]
If you add the \-\-daemon option irexec will fork to
background. That way you can easily start irexec from an init script. In
this case you should specify a config file on the command line as irexec
won't be able to find your home directory. Potential uses are shutting
down the computer, starting a dial-up connection etc.

[NOTE]
If you start irexec, it reads your ~/.lircrc and reacts only on
prog= entries that point to irexec. If you have included more than one
program in your .lircrc, then start all these programs, they react
itself only to their according entries in .lircrc.

