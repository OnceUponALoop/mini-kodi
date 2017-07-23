[NAME]
ircat - print strings when pressing buttons

[DESCRIPTION]
This program prints config strings to standard output. It can be used 
to provide remote control input to scripts and to debug your .lircrc 
file.

The argument to the program is the program name, as it
appears in the prog entries in .lircrc.

[EXAMPLES]
If .lircrc contains:

.nf 
begin
	prog = myprog
	button = tv_p+
	config = next_file
end
.fi

then
 
.nf
$ ircat myprog
.fi

will print "next_file" (followed by newline) every time the
button tv_p+ is pressed.
