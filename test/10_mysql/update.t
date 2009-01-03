#!/usr/bin/perl

use strict;
use warnings;

use Test::More qw(no_plan);
use Socket;
use DBI;
use DBD::mysql;

use lib qw(test);
use MySQLTest;
use PDBTest;

my $port = 2139;

PDBTest::startup_with_inline_configuration(<<"ENDCFG");
log_file = test/pdb.log
log_level = DEBUG

listen_port = $port

$MySQLTest::database_configuration
ENDCFG

my $dbh_pdb = DBI->connect("DBI:mysql:database=irrelevant;host=127.0.0.1;port=$port", 'root', '');
die unless defined $dbh_pdb;

my $rv;

## update existing data on master
# $rv = $dbh_pdb->do('update whatsit set description = \'poot\' where whatsit_id = 1');
# ok($rv == 1);

## update nonexistent data on master
# $rv = $dbh_pdb->do('update whatsit set description = \'poot\' where whatsit_id = 0');
# ok($rv == 0);

## update existing partitioned data
# $rv = $dbh_pdb->do('update widget set widget_information = \'poot\' where widget_id = 2');
# ok($rv == 1);
# $rv = $dbh_pdb->do('update widget set widget_information = \'poot\' where widget_id = 3');
# ok($rv == 1);

## update nonexistent partitioned data
# $rv = $dbh_pdb->do('update widget set widget_information = \'poot\' where widget_id = 0');
# ok($rv == 0);

## update partitions without specifying key (parallel)
# $rv = $dbh_pdb->do('update widget set widget_information = \'boot\' where widget_information = \'poot\'');
# ok($rv == 2);

$dbh_pdb->disconnect();
PDBTest::shutdown();
