#!/usr/local/bin/perl

##
#  mamer - Tournament Director for the Free Internet Chess Server
#  
#  Copyright (C) 1995 Fred Baumgarten
#  Copyright (C) 1996-2001 Michael A. Long
#  Copyright (C) 1996-2001 Matthew E. Moses
#  Copyright (C) 2002 Richard Archer
#  
#  $Id: hindex.pl,v 1.2 2002/08/08 02:53:35 rha Exp $
#  
#  mamer is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published 
#  by the Free Software Foundation; either version 2 of the License, 
#  or (at your option) any later version.
#  
#  mamer is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with mamer; if not, write to the Free Software Foundation, 
#  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##

$dirpath = "/home/mlong/mamer/help/";

opendir(FILELIST, "$dirpath");
@allfiles = readdir(FILELIST);
closedir(FILELIST);

open(OUT, ">$dirpath/index");
$i = 0;
$j = 0;
$offset = $#allfiles/4;
@allfiles = sort @allfiles;
foreach $a (@allfiles) {
    if($allfiles[$i] !~ /\./) {
	$temp[$j] = $allfiles[$i];
	$j++;
    }
    $i++;
}
@allfiles = @temp;
$i = 0;
#print OUT "$#allfiles files listed\n";
while ($i <= ($#allfiles/4)) {
    printf OUT 
	(" %-18s %-18s %-18s %-18s\n", 
	 $allfiles[$i], $allfiles[$i+($offset*1)],
	 $allfiles[$i+($offset*2)], $allfiles[$i+($offset*3)]);
    $i++;
}
#print OUT "\n";
close(OUT);
