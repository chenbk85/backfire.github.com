#!/usr/bin/perl -w
$| = 1;
use Net::INET6Glue::INET_is_INET6;

use LWP;
use Readonly;
use Encode;
use List::MoreUtils qw /any uniq/;
use IO::All;

#use re 'debugcolor';


Readonly::Scalar  my  $DIR => '/home/smyang/temp/bbs';

Readonly::Scalar  my  $HOST => "http://bbs2.ustc.edu.cn";

Readonly::Scalar  my  $PID_FILE => '/tmp/fetch_bbs_attach.pid';

my @board_list = qw/test PieBridge Portrait/;

local ($BOARD,$SEARCH_URL,$BBSCON_URL,$SEARCH_CONTENT_RE);


my ($DAY, $MONTH, $YEAR) = (localtime)[3,4,5];
Readonly::Scalar  my  $DATE => sprintf("%04d-%02d-%02d",$YEAR+1900,$MONTH+1,$DAY);
Readonly::Scalar  my  $FETCH_TXT => "./fetched_list.txt";

my $b = LWP::UserAgent->new;
$b->cookie_jar();

my @fetched_list;

sub search_at{
    my @result = ();
    my $response = $b->get($SEARCH_URL);
    if ($response->is_success){
        my $content = $response->content;
        $content = encode("utf8",decode("gbk",$content));
        while($content =~ /$SEARCH_CONTENT_RE/g){
            print "$2:$1\n";
            #push  (@result, $1) unless any {$_ eq $1} @fetched_list;
            push  (@result, $1) unless $1~~@fetched_list;
        }
    }else{
        warn $response->content;
    }
    @result;
}
sub fetch_at(@){
    my $board = shift(@_);
    my @list = @_;
    print "$_," for @list;
    print "\n";
    my @img = ();
    for my $fn (@list){
        my $url = $BBSCON_URL . $fn;
        my $response = $b->get( $url);

        if ($response->is_success){
            my $content = $response->content;
            $content = encode("utf8",decode("gbk",$content));
            print "GET $fn\n";
            my @img_tmp=();
            save_content($fn, $board, $content);
            while($content =~ /单击新窗口打开'><img src="(sf\?s=.+?)"\s+onload/gs){
                push @img_tmp,$1;
                push @img, $1;
                print "Parse $1\n";
            }
            #---download here-----
            down_at(@img_tmp);
        }else{
            warn $response->content;
        }
        push @fetched_list,$fn;
    }
    @img;

}
sub down_at(@){
    my @urls = @_;
    for my $url (@urls){
        $url =~ s/&amp;/&/g;
        print "download .. $url  ";
        use Digest::MD5  qw(md5 md5_hex);
        my $content_file = md5_hex($url);
        if($url =~/fn=(M.+?)&(?:amp;)?an=(.+)$/){
            $content_file = "$1__$2";
        }
        print  " > $content_file  ";
        if (-s "$DATE/$content_file" ){
            print " EXIST!!\n";
            next;
        }
        #- use gbk for download. (url encode)
        $url = encode('gbk',decode('utf8',$url));
        $b->get($HOST . "/cgi/" .$url, ':content_file' => "$DATE/$content_file" );
        print "Done.\n";
    }
}
sub init(){
    chdir $DIR;
    
    mkdir $BOARD unless -d $BOARD;
    chdir $DIR.$BOARD;

    mkdir $DATE;
    if (-s $FETCH_TXT){
        open my $f , "<", $FETCH_TXT or (warn "$FETCH_TXT can't open.\n" and return);
        while(<$f>){
            chomp;
            push (@fetched_list, $_);
        }
        @fetched_list = uniq @fetched_list;
        close($f);
    }
}
sub sync_list(){
    if(-d $DATE){
        open my $f , "+>", $FETCH_TXT or warn "$FETCH_TXT can't write.\n";
        for(@fetched_list){
            print {$f} "$_\n";
        }
        close($f);
    }
}

sub save_content($$$){
    my $fn = shift;
    my $board = shift;
    my $content = shift;
    my $title_start_index = index($content, "瀚海星云 文章阅读") + 27;
    my $title_end_index = index($content, "<\/title>");
    my $title = substr($content, $title_start_index, $title_end_index - $title_start_index, );
    $content =~s/"mycss\?css=bbs\.css"/"..\/..\/bbs.css"/g;
    $content =~s/单击新窗口打开'>
                    <img\s+src=".+?fn=(M.+?)&(?:amp;)?an=(.+?)"\s+
                /单击新窗口打开'><img src="..\/$DATE\/$1__$2"/gx;
    $content = encode('gbk',decode('utf8',$content));
    $content > io($board . "/${title}.html");
}

#---- ensure only one script is running.
sub check_pid(){
    if( -s $PID_FILE){
        my $pid = qx{ cat $PID_FILE};
        chomp $pid;
        if(qx{ps u}=~m/^\w+\h+${pid}\h+/m){
             die "the script is already running\n";
        }
        else{
            unlink $PID_FILE;
        }
    }
    open my $pid, ">$PID_FILE" or warn "Can't create PID file: $PID_FILE.";
    print $pid "$$\n";
    close $pid;
    print "[$$] started at ".(scalar(localtime))."\n";
}

#---- main ----
sub main{

    check_pid();

    for my $board(@board_list){
        local $BOARD = $board;
        local $SEARCH_URL = $HOST . "/cgi/bbsbfind?type=1&board=" .$BOARD . "&dt=7&at=on";
        local $BBSCON_URL = $HOST . '/cgi/bbscon?bn='. $BOARD .'&fn=';
        local $SEARCH_CONTENT_RE = qr/\@x?
                                    <td\s+ class='title'>
                                    <a\s+ href=bbscon\?bn=${BOARD}&fn=(M.+?)&num=\d+>
                                       (.+?)
                                    <\/a>
                            /ix;
        init();
        my @list = search_at();
        my @urls = fetch_at($BOARD, @list);
        sync_list();
    }
}

main();

