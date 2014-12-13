#!/usr/bin/perl

#    Copyright 2014 Andrew Basterfield
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.

use warnings;
use strict;
use Time::HiRes;

# Set up the serial port for Windows
#use Win32::SerialPort;
#my $port = Win32::SerialPort->new("COM2");

# setup serial port for Linux
use Device::SerialPort;
my $port = Device::SerialPort->new("/dev/ttyACM0");

#port configuration  115200/8/N/1
$port->baudrate(115200);
$port->databits(8);
$port->parity("none");
$port->stopbits(1);
#$port->buffers(1, 1); #1 byte or it buffers everything forever
$port->dtr_active(0); # doesn't work
$port->stty_echo(0);
$port->write_settings || undef $port; #set
unless ($port) {
  die "couldn't write_settings";
}

my $buffer = "";
while (1) {

  my ($count,$chunk) = $port->read(255);

  if (defined $count && $count > 0) {
    $buffer .= $chunk;
  }
  
  while ($buffer =~ m/\n/) {
  
    my $nlpos = index($buffer,"\n");
    
    #
    # Extract the line from the start of buffer until before \n
    #
    my $line = substr($buffer,0,$nlpos);
    
    #
    # Bring forwards the startof the buffer after \n
    #
    $buffer = substr($buffer,$nlpos+1);
  
    logger('RECV',"[".$line."]\n");

    if ($line eq "Request Sync") {
      my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(getTime());
      
      $year += 1900;
      $mon += 1;
      
      my $time = sprintf("H%02dM%02dS%02dd%02dm%02dY%04dT",$hour,$min,$sec,$mday,$mon,$year);
      logger("SEND","[$time]\n");
      
      
      $port->write($time);
    }
  }
}

sub getTime {

  my ($time,$usec) = Time::HiRes::gettimeofday();
  
  #
  # Sleep until the start of the next second
  #
  Time::HiRes::usleep(1000000-$usec);

  #
  # we have slept so increment the unix timestamp
  #
  $time++;
  
  return $time;
}

sub logger {
  my $level = shift;
  my $info = shift;

  foreach my $line (split (/\n/, $info)) {
    #
    my ($time,$usec) = Time::HiRes::gettimeofday();

    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=localtime($time);
    #
    printf "%4d-%02d-%02d %02d:%02d:%02d.%06d [%d] %s: %s\n",$year+1900,$mon+1,$mday,$hour,$min,$sec,$usec,$$,$level,$line;
  }
}
