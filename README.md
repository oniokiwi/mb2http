# mb2http 

This is a modbus simulator, programmed to replicate the behaviour of an NEC "AEROS" battery controller. 
The unit we are interested in was at the Northern Power Grid's ("NPG") Rise Carr site, controlling a 2.5 MW battery.


To Build the simulator simply run the following comand in the current directory.
$ make 

To enable watchdog support on the executable run the following command to a cron timer job
$ make cronjobstart

To disable watchdog support
$ make cronjobstop

