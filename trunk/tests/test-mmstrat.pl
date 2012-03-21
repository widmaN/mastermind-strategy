#!/usr/bin/perl
#
# Run tests for the mmstrat utility.
#

use strict;
use warnings;

# Path to executable.
my $exec = 'mmstrat';

# Counter for test number.
my $number = 0;
my $failed = 0;

# Test strategies. Each test is specified by its command line arguments
# and the expected output. The "-S -q" switches are automatically appended
# to each entry. Individual options for specific strategies are also tested.
my @test_cases = (

	# Test heuristic strategies for standard Mastermind rules.
	"-r mm -s minmax",          "5778:5:663",
	"-r mm -s minmax -no",      "5778:5:663",
	"-r mm -s minmax -nc",      "5780:5:663",
	"-r mm -s minmax -po",      "5797:6:44",
	"-r mm -s minavg",          "5696:6:3",
	"-r mm -s minavg -no",      "5696:6:3",
	"-r mm -s minavg -nc",      "5702:6:3",
	"-r mm -s minavg -po",      "5722:6:43",
	"-r mm -s entropy",         "5719:6:18",
	"-r mm -s entropy -no",     "5719:6:18",
	"-r mm -s entropy -nc",     "5726:6:12",
	"-r mm -s entropy -po",     "5787:6:58",
	"-r mm -s parts",           "5668:6:7",
	"-r mm -s parts -no",       "5668:6:7",
	"-r mm -s parts -nc",       "x",
	"-r mm -s parts -po",       "5714:7:2",

	# Test optimal strategies.
	"-r mm -s optimal",         "5625:6:7",
	"-r mm -s optimal -O 1",    "5625:6:7",
	"-r mm -s optimal -po",     "x",
	"-r mm -s optimal -md 10",  "x",
	"-r mm -s optimal -md 6",   "x",
	"-r mm -s optimal -md 5",   "x",
	"-r mm -s optimal -md 4",   "x",

	# Test different equivalence filters.
	"-r mm -s minavg -e default",    "5696:6:3",
	"-r mm -s minavg -e constraint", "5696:6:3",
	"-r mm -s minavg -e color",      "5696:6:3",
	"-r mm -s minavg -e none",       "5696:6:3",

	# Build strategy using 2 threads.
	"-r mm -mt 2 -s minmax",    "5778:5:663",
	"-r mm -mt 2 -s minavg",    "5696:6:3",
	"-r mm -mt 2 -s entropy",   "5719:6:18",
	"-r mm -mt 2 -s parts",     "5668:6:7",
	"-r mm -mt 2 -s optimal",   "5625:6:7",
);

for (my $i = 0; $i < $#test_cases; $i += 2)
{
	my $opt = $test_cases[$i];
	my $expect = $test_cases[$i+1];

	print "Running test ", sprintf("%2d", ++$number), " ... ";

	my $cmd = "$exec -S -q $opt";
	my $actual = `$cmd`;
	chomp($actual);

	if ($actual eq $expect)
	{
		print "OK\n";
	}
	else
	{
		print "FAILED\n";
		print "    Command: $cmd\n";
		print "    Expect:  $expect\n";
		print "    Actual:  $actual\n";
		++$failed;
	}
}

# Display summary.
if ($failed == 0)
{
	print "All tests passed.\n";
}
else
{
	print "$failed tests failed out of $number.\n";
}
