[NAME]
irrecord - application for recording IR-codes for usage with LIRC

[DESCRIPTION]
This program will record the signals from your remote control and create
a config file for lircd. A proper config file for lircd is maybe the
most vital part of this package, so you should invest some time to
create a working config file. Although I put a good deal of effort in
this program it is often not possible to automatically recognize all
features of a remote control.

If the program fails to recognize the protocol of the remote control you
should use the \-\-force option to at least create a config file in raw mode.

If \fIfile\fR already exists and contains a valid config irrecord will use the
protocol description found there and will only try to record the
buttons. This is very useful if you want to learn a remote where config
files of the same brand are already available. Of course this will only
work if the remotes use the same protocol but it's worth a try. You will
find template files for the most common protocols in the
remotes/generic/ directory of this package. The name of the new file is
created by appending .conf to the given filename in this case.

[SEE ALSO]
Further information on this topic is available in section "Adding new
remote controls" in html/help.html

