#!/usr/bin/perl

package MySQLTest;

use strict;
use warnings;

our $basedir = '/usr/local/mysql';
our $socket_file = '/socket';
our $pid_file = '/pid';

our @servers = (
    { 'dir'  => '/tmp/test_db_1',
      'port' => 1234 },
    { 'dir'  => '/tmp/test_db_2',
      'port' => 1235 },
    { 'dir'  => '/tmp/test_db_3',
      'port' => 1236 },
);

1;
