#!/usr/bin/perl

package MySQLTest;

use strict;
use warnings;

our $basedir = '/usr/local/mysql';
our $socket_file = '/socket';
our $pid_file = '/pid';

our @servers = (
    { 'dir'  => '/tmp/test_master',
      'port' => 1234,
      'init' => 'test/mysql_master.sql' },
    { 'dir'  => '/tmp/test_partition_1',
      'port' => 1235,
      'init' => 'test/mysql_partition_1.sql' },
    { 'dir'  => '/tmp/test_partition_2',
      'port' => 1236,
      'init' => 'test/mysql_partition_2.sql' },
);

1;
