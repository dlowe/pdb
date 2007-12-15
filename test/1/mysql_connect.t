#!/usr/bin/perl

use strict;
use warnings;

use Test;
use Socket;
use DBI;
use DBD::mysql;

BEGIN { plan tests => 1 }

require('test/PDBTest.pm');

## XXX: start mysql

PDBTest::startup();

my $dbh_pdb = DBI->connect("DBI:mysql:database=mysql;host=127.0.0.1;port=5032",
  'root', '');
ok(defined $dbh_pdb);
if (defined $dbh_pdb) {
    $dbh_pdb->disconnect();
}

PDBTest::shutdown();

## XXX: shutdown mysql
