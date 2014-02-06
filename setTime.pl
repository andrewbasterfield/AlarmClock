#!/usr/bin/perl

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
  
    print "RECV [".$line."]\n";

    if ($line eq "Request Sync") {
      my $time = getTime();
      $time = time();
      
      my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($time);
      
      $year += 1900;
      $mon += 1;
      
      $time = sprintf("H%02dM%02dS%02dd%02dm%02dY%04dT",$hour,$min,$sec,$mday,$mon,$year);
      print "SEND [$time]\n";
      
      
      $port->write($time);
    }
  }
}

sub getTime {

  my $time = Time::HiRes::time();
  
  #
  # take the integer component of the time +1
  #
  my $itime = int $time + 1;
  my $ftime = $time - $itime;
  #
  # calculate the time required to sleep till $time
  #
  my $frac = 1 - $ftime;
  
  Time::HiRes::sleep($frac);
  
  #$time += 3600;
  
  return $time;
#  return sprintf("T%10.0f",$time);
}