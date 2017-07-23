[NAME]
lircrcd - daemon to handle consistent .lircrc state among client applications

[DESCRIPTION]

.B lircrcd
reads the given .lircrc config file and synchronises the mode
that the LIRC clients using this config file (\fBirexec\fR(1),
\fBirxevent\fR(1) , etc.) are in. Using 
.B lircrcd
has to be explicitly
enabled in the config file by adding the following line at the
beginning of the file:
.br
#! lircrcd

[OPTIONS]
.TP
.BI \-p,\ \-\-premission= mode
The \-\-permission option gives the file permission of the Unix domain
socket lircrcd creates on startup in octal representation. Read the
documentation for chmod for further details. If no \-\-permission option
is given when the socket is created the default is to give only the
user owning the file read and write permissions (0600 in octal
representation).
.TP
.BI \-o,\ \-\-output= socket
With the \-\-output option you can select the Unix domain socket, which
lircrcd will create. The default is to append a "d" character to the
config filename given.
