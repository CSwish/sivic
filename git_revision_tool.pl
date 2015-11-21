#!/usr/bin/perl
use Cwd;
use Getopt::Long;
use Pod::Usage;
use strict;

sub get_last_revision();
sub check_repository_status($);
sub create_git_tag($);

my $tag_branch = "develop";
Getopt::Long::config("bundling");
GetOptions(
    "i=s"         =>\my $increment_index,
    "p=s"         =>\my $print_index,
    "b=s"         =>\$tag_branch,
    "o"           =>\my $override_repo_check,
    "v"           =>\my $verbose,
    "h"           =>\my $help
) or pod2usage(-verbose=>2, -exitval=>2);

if( defined($help) ) {
	pod2usage(-verbose=>2, -exitval=>2);
}

###############################################################################
#  Get last revision 
###############################################################################
my @revision = get_last_revision();

# last entry in array is the commit hash
my $last_revision_commit = pop(@revision);

my $revision_tag = join('.', @revision );
my $output_string = $revision_tag;

if (defined($increment_index) ){

    my $current_commit = `git rev-parse HEAD`;
    chomp($current_commit);
    if( $current_commit ne $last_revision_commit ) {

        ###############################################################################
        #  Make sure there are no local changes
        ###############################################################################
        if( !defined($override_repo_check) ) {
            my $repo_status = check_repository_status($tag_branch);
            if( $repo_status != 0 ) {
                print "ERROR: Your local repository is not suitable for tagging.\n";
                print "       You must use the $tag_branch branch AND have no local changes.\n";
                exit($repo_status);
            }
        }

        # Permit the adding of one new index. For example 0.9.4 can increment to 0.9.4.1
        my $max_index = $#revision + 1;

        if( $increment_index > $max_index || $increment_index < 0 ) {
            print "ERROR: Index ($increment_index) is out of range. Acceptable indices are 0-$max_index!\n";
            exit(1);
        } elsif( $increment_index == $max_index ) {
            # Set the new index to 0. It will be immediately incremented to 1 below.
            push(@revision, 0);
        }

        # increment the correct indices. All indices > the increment index are updated
        for( my $i = $increment_index; $i <= $#revision; $i++ ) {
            if( $i == $increment_index ) {
                $revision[$i]++;
            } else {
                $revision[$i] = 0;
            }
        }
        $revision_tag = join('.', @revision );
        if($verbose) {
            print "Creating tag: $revision_tag\n";
        }
        create_git_tag($revision_tag);
        $output_string = $revision_tag;
    } elsif ($verbose) {
        print "Current commit is already tagged. Will not be incremented.\n";
    }
} 

if( defined($print_index) ) { 
    if( $print_index >= 0 && $print_index <= $#revision ) {
        $output_string = $revision[$print_index];
    } else {
        print "ERROR: $print_index is outside of revision index range. Range is 0-$#revision.\n";
        exit(1);
    }
} else {
    $output_string .= "\n";
}

print $output_string;

exit(0);


###############################################################################
#
# SUBROUTINES
#
###############################################################################

###############################################################################
#  Gets the last tagged revision. Returns an array with each revision index
#  and appends the commit hash to the end of the list.
###############################################################################
sub get_last_revision() {

    ###############################################################################
    #  Parse to get the current revision
    ###############################################################################
    my @tags = `git ls-remote -t -q`;
    my $last_revision_commit;
    my @last_revision_number;
    foreach my $tag (@tags) {
        chomp($tag);
        if( $tag =~/(.*)\s+refs\/tags\/(.*)/ ) {
            my $revision_commit = $1;
            my @revision_numbers = ($2 =~ /(\d+)/g);
            for (my $index = 0; $index <= $#revision_numbers; $index++ ) {
                # Assume an extra index is always later. Example 2.2.0 comes after 2.2.
                if( $index > $#last_revision_number ){
                    @last_revision_number = @revision_numbers;
                    $last_revision_commit = $revision_commit;
                    last;
                } elsif( $revision_numbers[$index] > $last_revision_number[$index] ) {
                    @last_revision_number = @revision_numbers;
                    $last_revision_commit = $revision_commit;
                    last;
                }
            }
        }
    }

    if( !defined($last_revision_commit ) ) {
        print "ERROR: Could not find any previously tagged revisions.\n";
        exit(1);
    }
    push(@last_revision_number, $last_revision_commit);
    return @last_revision_number;
}


###############################################################################
# Checks to see if there are any local changes in your git repository.
# Also ensures that the user is on the correct branch.
###############################################################################
sub check_repository_status($) 
{
    my( $tag_branch ) = @_;
    my $return_status = 0;
    # First let's check to make sure the user is on the correct branch.
    my $branch =`git rev-parse --abbrev-ref HEAD`;
    chomp($branch);
    if( $branch ne $tag_branch ) {
        print "ERROR: Current branch=$branch. Please checkout the $tag_branch branch.\n";
        $return_status = 1;
    } else {
        my $local_changes = `git diff origin/$tag_branch`;
        if( $local_changes ne "" ) {
            print "ERROR: You have local changes. Please commit and push.\n";
            $return_status = 1;
        }
    }
    return $return_status;    
}


###############################################################################
#  Creates the new git tag.
###############################################################################
sub create_git_tag($)
{
    my( $new_tag ) = @_;
    if( system("git tag $new_tag") != 0 ) {
        print "ERROR: Could not create git tag: $new_tag.\n";
        exit(1);
    } 
    if( system("git push origin $new_tag") != 0 ) {
        print "ERROR: Could not push git tag: $new_tag.\n";
        exit(1);
    }
     
}

###############################################################################
#
#   POD usage docs
#
###############################################################################

=head1 NAME

git_revision_tool.pl

=head1 SYNOPSIS
    
    git_revision_tool.pl [-i increment_index -p print_index -b branch -n -v -h]

        -i  increment_index  Increment the revision by the given index.
        -p  print_index      Just print the given index of the current revision.
        -b  branch           The branch that tags should be made on.
        				     (default=develop)
        -o                   Override the repository check. If not set the
                             script will exit with an error if the user is not
                             on the specified branch OR has any local changes.
        -v                   Verbose output.
        -h                   Print this message.

=head1 DESCRIPTION

    The purpose of this script is to provide a tool for using git tags to
    manage revisions in a given git project. It is assumed that the tool is
    run from within a git project. There are a few different ways this script
    can behave depending on the arguments used: 
    
        *If no arguments are specified then current revision number is printed. 
    The current revision is determined by getting all tags for the project, 
    parsing out only tags that are an arbitrary number of integers delimited 
    by '.' characters. For example 0.9.4 would be identified as a revision tag.
    Only the highest revision number is printed, and it is assumed that any
    additional digits in a revision number belong to a LATER revision. For
    example 0.9.4.0 would be later then 0.9.4.

        *If the -i flag is used then the current revision will be incremented
    at the specified index. For example if '-i 0' is set the first index will be
    incremented by one and all later indices are will be set to 0. The index
    specified must be greater than or equal to zero and less then or equal to
    the number of revision indices. If the index is set to the number of
    revision indices then an extra revision index will be added. For example
    if the current revision is 0.9.2 and the user specifies '-i 3' then the
    new revision will be 0.9.2.1. The new index is set to one rather then
    zero for clarity. This flag will trigger a check to make sure the user
    is on the correct branch for tagging and that there are no local changes.
    Adding the '-o' flag overrides this check.

        *If the -p flag is used then only the revision number at a specific
    index is printed. For example if the current version was 1.2.0 and the
    user ran with '-p 1' then only '2' would be printed.
    

=cut
