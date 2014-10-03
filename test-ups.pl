#!/usr/bin/perl -w

use strict;
use Net::UPS;
use Data::Dumper;

my $ups = Net::UPS->new("username", "password", "key");


# prepare package
my $pkg = Net::UPS::Package->new(length=>1, width=>1, height=>1, weight=>1);

    # calculate rate
my $from = '33015';
my $to   = '33141';
my $rate = $ups->rate($from, $to, $pkg);

    unless ( defined $rate ) {
        die "Couldn't calculate rates for your package: " . $ups->errstr;
    }
    printf("Sending this package will cost you \$%.2f\n", $rate->total_charges);

