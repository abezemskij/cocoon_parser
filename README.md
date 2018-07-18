# Cocoon Parser

Utility used for parsing tshark output. <br>

Dependencies:<br><br>

> g++ <br><br>

Compilation:<br><br>
Temporary:<br>
> cd ./src
> g++ ./analysis/processor.c ./analysis/generic.c ./analysis/math_func.c ./analysis/windowing.c $(ls | grep .c) -o p -g


> 1. cmake .<br>
> 2. make <br><br>

Usage:<br><br>

> ./parser -**i** [file] -**o** [file] -**[wzs]**	(.csv file parsing) <br>
> ./parser -**l** -**o** [file] -**[wzs]**		(live mode parsing, needs to be piped)<br><br>


Help:<br><br>

> **-l** - Live mode<br>
> **-io** - Input/Output<br>
> **-t** - Direct output<br>
> **-wzs** - WiFi/ZigBee/IP Short Header<br>
> **[file]** - Any existing/non-existing filename <br>


For WiFi use [Input] | ./parser -l -w -t (will generate output)<br>
