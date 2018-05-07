# Cocoon Parser

Utility used for parsing tshark output. <br>

Dependencies:<br><br>

> g++ <br><br>

Compilation:<br><br>

> 1. cmake .<br>
> 2. make <br><br>

Usage:<br><br>

> ./parser -**i** [file] -**o** [file] -**[wzs]**	(.csv file parsing) <br>
> ./parser -**l** -**o** [file] -**[wzs]**		(live mode parsing, needs to be piped)<br><br>


Help:<br><br>

> **-l** - Live mode<br>
> **-io** - Input/Output<br>
> **-wzs** - WiFi/ZigBee/IP Short Header<br>
> **[file]** - Any existing/non-existing filename <br>

