# Cocoon Parser

Utility used for parsing tshark output. <br>

Dependencies:<br><br>

> gcc <br><br>

Compilation:<br><br>

> 1. cmake .<br>
> 2. make <br><br>

Usage:<br><br>

> ./parser -**i** [file] -**o** [file] -**[wz]**	(.csv file parsing) <br>
> ./parser -**l** -**o** [file] -**[wz]**		(live mode parsing, needs to be piped)<br><br>

Help:<br><br>

> **-l** - Live mode<br>
> **-io** - Input/Output<br>
> **-wz** - WiFi/ZigBee<br>
> **[file]** - Any existing/non-existing filename <br>

