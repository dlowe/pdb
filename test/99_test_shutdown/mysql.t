#!/usr/bin/perl

use strict;
use warnings;

use Test::More qw(no_plan);

use lib qw(test);
use MySQLTest ();

foreach my $server (@MySQLTest::servers) {
    my $pidf = $server->{'dir'} . $MySQLTest::pid_file;
    if (-e $pidf) {
        system('kill `cat ' . $pidf . '`');
    }
}

WAIT_FOR_MYSQLD: while (1) {
    sleep(1);
    foreach my $server (@MySQLTest::servers) {
        if (-e $server->{'dir'} . $MySQLTest::pid_file) {
            next WAIT_FOR_MYSQLD;
        }
    }
    last WAIT_FOR_MYSQLD;
}

foreach my $server (@MySQLTest::servers) {
    if (-d $server->{'dir'}) {
        system('rm -rf ' . $server->{'dir'});
    }
}

ok(1);
