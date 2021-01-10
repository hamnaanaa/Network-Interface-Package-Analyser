# Network-Interface-Package-Analyser
## Description
C program that mimicks a packet analyzer of all incoming network packets.
The program reads all network packets from the specified interface (see usage) and prints the total rate.
For additional information please refer to **IEEE 802**.

## Installation
Extremely straightforward under linux - clone it and run the Makefile in the directory.
```sh
$ make
```

## Usage
Example:
```sh
./ethstats -i wlp3s0 10000
```

Short usage:
```sh
$ ./ethstats --usage
Usage: ethstats [-?] [-i interface] [--interface=interface] [--help] [--usage]
            NUM
```

Complete help:
```sh
$ ./ethstats --help

Usage: ethstats [OPTION...] NUM
ethstats: Analyze and count raw Ethernet frames
-i         interface from which frames should be read
NUM        number of frames to read before printing summary

  -i, --interface=interface
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Default values:
```c
-> --interface= / -i : "eth0"
Consider running netstat to list all available interfaces:
$ netstat -i

-> NUM : 10
This value was chosen to be able to pinpoint some specific packets.
For a speedtest consider choosing values between 1,000 - 100,000 depending on the expected speed.
```

## Automation
This program can be executed by **cron** under linux as an automated internet speedtest:
```sh
$ crontab -e                            # edit crontab
$ * */1 * * * PROGRAM &>> OUTPUT_FILE   # run every hour
```
Please refer to the cron docs for more information:
```sh
$ man cron
```