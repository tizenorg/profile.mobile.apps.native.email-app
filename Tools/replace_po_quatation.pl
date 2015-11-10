#!/usr/bin/perl

use strict;
use warnings;
use Tie::File;

# global
my @po_list = `ls *.po`;

# for replacing "
my $dest_char1 = "\\\\\"";
my $dest_char2 = "\\\"";
my $quatation_char = "\"";

# function
sub replace_quatation
{
    my ($po_file) = @_;
    my @po_file_line = $po_file;

    foreach (@po_file_line) {
        tie my @filearray, 'Tie::File', $_ or die "Couldn't open file $_, $!";
        foreach my $line(@filearray) {
            if (($line =~ /msgstr\ \"(.*)\"/)) {
                my $new_line = $1;

                # quatation check
                # there is no qutation, continue
                if (index($new_line, "\"") != -1) {
                    printf "replace: $new_line\n";
                    # equalizing to "
                    $new_line =~ s/$dest_char1/$quatation_char/g;

                    # replace " with \"
                    $new_line =~ s/$quatation_char/$dest_char2/g;

                    # write new line
                    $line = "msgstr\ \"$new_line\"";
                }

                # '\n' check
                if (index($new_line, "\\n") != -1) {
                    printf "replace: $new_line\n";

                    # replace \n with <br>
                    $new_line =~ s/\\n/<br>/g;

                    # write new line
                    $line = "msgstr\ \"$new_line\"";
                }

                # “” check
                if (index($new_line, "“") != -1) {
                    printf "replace: $new_line\n";

                    # replace \n with <br>
                    $new_line =~ s/\\“/\\"/g;

                    # write new line
                    $line = "msgstr\ \"$new_line\"";
                }
                if (index($new_line, "\\”") != -1) {
                    printf "replace: $new_line\n";

                    # replace \n with <br>
                    $new_line =~ s/\\”/\\"/g;

                    # write new line
                    $line = "msgstr\ \"$new_line\"";
                }

				if (index($new_line, "\xc2\xa0") != -1) {
					printf "replace: $new_line\n";

					# replace non-breaking space(\xc2\xa0 in UTF8) to space
					$new_line =~ s/\xc2\xa0/ /g;

					# write new line
					$line = "msgstr\ \"$new_line\"";
				}
            }
        }
        untie @filearray;
    }
}

# main
foreach (@po_list) {
    my $file = $_;
    chomp($file);
    replace_quatation($file);
}
