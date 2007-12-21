#!/usr/bin/perl

use strict;
use warnings;

use Test;
use Socket;
use DBI;
use DBD::mysql;

BEGIN { plan tests => 6 }

require('test/PDBTest.pm');

## XXX: start mysql

PDBTest::startup();

my $dbh_pdb = DBI->connect("DBI:mysql:database=mysql;host=127.0.0.1;port=5032",
  'root', '');
ok(defined $dbh_pdb);

my $row = $dbh_pdb->selectall_arrayref('SELECT DATABASE(),USER()')->[0];
ok($row);
ok($row->[0] eq 'mysql');
ok($row->[1] =~ /^root/);

## twice in a row; the second one may hang if LISTFIELDS isn't correctly
## implemented
$row = $dbh_pdb->selectall_arrayref("LISTFIELDS user");
ok($row);
$row = $dbh_pdb->selectall_arrayref("LISTFIELDS user");
ok($row);

if (defined $dbh_pdb) {
    $dbh_pdb->disconnect();
}

PDBTest::shutdown();

## XXX: shutdown mysql
