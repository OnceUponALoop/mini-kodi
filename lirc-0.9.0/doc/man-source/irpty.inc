[NAME]
irpty - pseudo tty driver

[DESCRIPTION]

irpty connects to lircd to receive infrared codes and converts them to
key strokes. E.g. type irpty ~/.lircrc \-\- workbone to control the
CD-player program workbone. Of course you will have to create an
appropriate config file for this purpose first. The config string will
be passed to the desired application. Note that you can use escape
sequences to specify non printable characters. Have a look at the\
 .lircrc file format description for details.


