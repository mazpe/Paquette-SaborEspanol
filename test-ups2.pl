#!/usr/local/bin/perl
use Business::UPS;

my ($shipping,$ups_zone,$error) = getUPS(qw/GNDCOM 33015 33141 2/);
$error and die "ERROR: $error\n";
print "Shipping is \$$shipping\n";
print "UPS Zone is $ups_zone\n";
