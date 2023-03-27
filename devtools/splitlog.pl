#!/usr/bin/perl -w

use strict;
use warnings FATAL => 'all';

sub append_to_file;
sub find_lowest_thread_no;
sub inc_out_file;
sub inc_out_stage;
sub output_line;
sub print_mutex_stats;
sub process_line;

my $inFile      = shift // undef;
my $prefix      = shift // "logsplit";
my %mutex_info  = ();
my %thread_info = ();
my $outName     = "";
my $outNo       = 0;
my $outStage    = 0; ## 0 = none, 1 = recording started, 2 = recording stopped.

defined( $inFile ) and ( length( $inFile ) > 0 ) and ( -f $inFile )
or print "Usage: ./splitlog.pl <logfile> [prefix]\n"
         . "\n<logfile> : Log file to split"
         . "\n[prefix]  : Prefix to name the splits. Defaults to: 'logsplit'\n\n"
         . "\tWill create/overwrite [prefix]_01[abc].txt .. [prefix]_nn[abc].txt\n"
         . "\ta splinters: Full log parts, only stripped of the time stamps and file names.\n"
         . "\tb splinters: Same as 'a', but without [MUTEX] lines.\n"
         . "\tc splinters: Log parts, [MUTEX] only, reformatted to <mutex>:<action>:<function>:<thread No>\n"
         . "Additionally file [prefix]_mutex_stats.csv will be written with some numbers to check.\n"
         . "\nNote: Threads are written as artificial numbers, not IDs, or diff'ing might end up being useless.\n"
   and exit 1;

print "Processing $inFile...";

if ( open( my $fIn, "<", $inFile ) ) {
	while ( my $line = <$fIn> ) {
		$line =~ s/[\r\n]+$//g;
		process_line( $line ) or close( $fIn ) and exit 1;
	}
	close( $fIn );
} else {
	print "ERROR: Can not open $inFile : $!\n";
	exit 1;
}

# Let's print mutex stats
print_mutex_stats() or print "ERROR writing mutex stats!\n" and exit 1;

print "\n$inFile processed.\n";
exit 0;

sub append_to_file {
	my ( $file, $line ) = @_;

	if ( open( my $fOut, ">>", $file ) ) {
		print( $fOut "$line\n" );
		close( $fOut );
	} else {
		print "ERROR: Can not create/overwrite $file : $!\n";
		return 0;
	}

	return 1
}

sub find_lowest_thread_no {
	my $thread_no = 0;

	for my $num ( sort keys %{ $thread_info{NOtoID} } ) {
		$thread_no = $num;
		defined( $thread_info{NOtoID} ) and ( "empty" ne $thread_info{NOtoID} ) or last;
		++$thread_no; ## If all are used, we end up with last num + 1;
	}

	return ( $thread_no > 0 ) ? $thread_no : 1;
}

sub inc_out_file {
	if ( 1 == $outStage ) {
		print "ERROR: Found recording start without stopping first!\n";
		return 0;
	}
	$outName = sprintf( "%s_%02d", $prefix, ++$outNo );
	for my $x ( qw{ a b c } ) {
		my $o = $outName . $x . ".txt";
		if ( -f $o ) {
			unlink( $o ) or print "ERROR: Can not unlink $o : $!" and return 0;
		}
		append_to_file( $o, "==== STARTING RECORDING ====" ) or return 0;
	}
	$outStage = 1;
	print "\n\tWriting into " . $outName . "[abc].txt : Start...";
	return 1;
} ## end sub inc_out_file

sub inc_out_stage {
	my $have_start = 1;
	if ( 1 != $outStage ) {
		print "Warning: Found recording stop without starting first!\n";
		$have_start = 0;
	}
	for my $x ( qw{ a b c } ) {
		( 0 == $have_start ) and
		append_to_file( $outName . $x . ".txt", "==== STARTING RECORDING ====\n(...did not happen ...)\n\n" );
		append_to_file( $outName . $x . ".txt", "==== STOPPING RECORDING ====" ) or return 0;
	}
	$outStage = 2;
	print " Stop...";
	return 1;
} ## end sub inc_out_stage

sub output_line {
	my ( $line ) = @_;

	# First trim the line of useless information
	$line =~ s/^\d+:\d+:\d+\.\d+:\s//;                                 # remove time stamp
	$line =~ s,/\d+-\d+-\d+[_ -]\d+-\d+-\d+\.(\w{3}),/<filename>.$1,g; ## remove explicit file name

	# Write to regular split file "a"
	if ( $outStage > 0 ) {
		append_to_file( $outName . "a.txt", $line ) or return 0;
	}

	# If this is a line without [MUTEX] info, write into b, and into c otherwise
	if ( $line =~ m/\[MUTEX\]/ ) {
		# Before we can write into c, we reformat the elements of the line and also create some stats
		# Source line example: config-file.c:673:config_get_string           : [debug] [MUTEX] Lock       '&config->mutex': 2916058460608
		# New source         : threading.h  :61 :pthread_mutex_init_recursive: [debug] [MUTEX] Initialize 'mutex'         : 140310026245056
		# Elements           :                  :   <function>               :                  <action>  <mutex>         :  <thread ID>
		# Wanted format      : <mutex>:<action>:<function>:<thread ID>
		if ( $line =~ m/[^:]+:\d+:([^:]+):.*\]\s+(.*)\s+[']&?([^']+)[']:?\s*(\d+)?\s*$/ ) {
			my ( $mutex, $action, $function, $thread_id ) = ( $3, $2, $1, $4 );
			defined( $thread_id ) or $thread_id           = "unknown";

			if ( !defined( $thread_info{IDtoNO}{$thread_id} ) ) {
				my $thread_no                    = find_lowest_thread_no();
				$thread_info{IDtoNO}{$thread_id} = $thread_no;
				$thread_info{NOtoID}{$thread_no} = $thread_id;
			}

			my $thread_name = sprintf( "thread_%02d", $thread_info{IDtoNO}{$thread_id} );

			if ( $outStage > 0 ) {
				# Note: We do not write output before the first start, but we need full mutex stats!
				append_to_file( $outName . "c.txt", "$mutex:$action:$function:$thread_name" ) or return 0;
			}

			defined( $mutex_info{mutex}{$mutex}{$action} ) or $mutex_info{mutex}{$mutex}{$action} = 0;
			$mutex_info{mutex}{$mutex}{$action}++;
			defined( $mutex_info{action}{$action} ) or print " (Adding $action)" and $mutex_info{action}{$action} = 1;

			defined( $mutex_info{thread}{$mutex}{$thread_name} ) or $mutex_info{thread}{$mutex}{$thread_name} = 0;
			$mutex_info{thread}{$mutex}{$thread_name}++;
			defined( $mutex_info{threadid}{$thread_name} ) or print " (Adding $thread_name)" and $mutex_info{threadid}{$thread_name} = 1;

			defined( $mutex_info{mutexaction}{$mutex}{$action}{$thread_name} ) or $mutex_info{mutexaction}{$mutex}{$action}{$thread_name} = 0;
			$mutex_info{mutexaction}{$mutex}{$action}{$thread_name}++;
		} else {
			print "ERROR: line does not fit MUTEX extract regex:\n$line\n";
			return 0;
		}
	} elsif ( $outStage > 0 ) {
		append_to_file( $outName . "b.txt", $line ) or return 0;
	}

	return 1;
} ## end sub output_line

sub print_mutex_stats {
	my $line = "mutex,";
	$outName = $prefix . "_mutex_stats.csv";

	if ( -f $outName ) {
		unlink( $outName ) or print "ERROR: Can not unlink $outName : $!" and return 0;
	}

	# Stats 1) Actions:
	for my $action ( sort keys %{ $mutex_info{action} } ) {
		$line .= "$action,";
	}

	append_to_file( $outName, $line ) or return 0;

	for my $mutex ( sort keys %{ $mutex_info{mutex} } ) {
		$line = "${mutex},";
		for my $action ( sort keys %{ $mutex_info{action} } ) {
			defined( $mutex_info{mutex}{$mutex}{$action} ) or $mutex_info{mutex}{$mutex}{$action} = 0;

			$line .= $mutex_info{mutex}{$mutex}{$action} . ",";
		}
		append_to_file( $outName, $line ) or return 0;
	}

	# Stats 2) Threads:
	$line = "mutex,";
	for my $threadid ( sort keys %{ $mutex_info{threadid} } ) {
		"unknown" eq $threadid and next;
		$line .= "$threadid,";
	}

	append_to_file( $outName, "\n$line" ) or return 0;

	for my $mutex ( sort keys %{ $mutex_info{thread} } ) {
		$line = "${mutex},";
		for my $threadid ( sort keys %{ $mutex_info{threadid} } ) {
			"unknown" eq $threadid and next;
			defined( $mutex_info{thread}{$mutex}{$threadid} ) or $mutex_info{thread}{$mutex}{$threadid} = 0;

			$line .= $mutex_info{thread}{$mutex}{$threadid} . ",";
		}
		append_to_file( $outName, $line ) or return 0;
	}

	# Stats 3) Full details over threads plus actions
	$line = "mutex,action,";
	for my $threadid ( sort keys %{ $mutex_info{threadid} } ) {
		"unknown" eq $threadid and next;
		$line .= "$threadid,";
	}

	append_to_file( $outName, "\n$line" ) or return 0;

	for my $mutex ( sort keys %{ $mutex_info{mutex} } ) {
		for my $action ( sort keys %{ $mutex_info{action} } ) {
			$line = "${mutex},${action},";
			for my $threadid ( sort keys %{ $mutex_info{threadid} } ) {
				"unknown" eq $threadid and next;
				defined( $mutex_info{mutexaction}{$mutex}{$action}{$threadid} ) or $mutex_info{mutexaction}{$mutex}{$action}{$threadid} = 0;

				$line .= $mutex_info{mutexaction}{$mutex}{$action}{$threadid} . ",";
			}
			append_to_file( $outName, $line ) or return 0;
		}
	}

	return 1;
}
sub process_line {
	my ( $line ) = @_;
	my $ret      = 1;

	( $line =~ m/Starting recording due to hotkey/ ) and ( $ret = inc_out_file() );
	( $line =~ m/Stopping recording due to hotkey/ ) and ( $ret = inc_out_stage() );
	( $line =~ m/d3d\d+ capture freed/ ) and $outStage          = 0; ## no start/stop beyond this point.
	( $line =~ m/\sShutting\sdown\s/ ) and $outStage            = 0; ## Non-Windows point of no return

	( $ret > 0 ) or return $ret;

	$ret = output_line( $line );

	return $ret;
} ## end sub process_line
