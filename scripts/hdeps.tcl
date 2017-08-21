#!/usr/bin/env tclsh
#
# Copyright (C) 2017 Philippe Aubertin.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# 3. Neither the name of the author nor the names of other contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ------------------------------------------------------------------------------
# This utility script generates a Graphviz visualization of the relations between
# header files (i.e. which header is included by which other). Standard C header
# files are excluded (for this purpose, Standard C headers are any header file
# located in include/kstdc).
#
# This script is not part of the build process.
#
# To use this script, just change into its parent directory, then run it and
# redirect standard output to a file or pipe it into dot directly, e.g.
#   ./hdeps.tcl | dot -Tsvg > deps.svg

set INCLUDES        "../include"

set STD_INCLUDES    "kstdc"

set INDENT_STRING   [string repeat " " 4]

# Recursively find all header files starting from path. The prefix argument is
# set when this procedure calls itself recursively and should be set to the
# empty string initially.
#
# This procedure assumes the Unix file path separator (/). There is no point in
# trying to be portable since the the slash is also embedded in the #include
# preprocessor directives.
proc glob_recursive {path prefix} {
    global STD_INCLUDES
    global headers
    global std_headers
    
    # Find header files in the current dirctory (specified by the path argument)
    # by iterating over all regular files matching the *.h pattern.
    foreach h [glob -type f -tails -directory $path *.h] {
        # Add each header file to the headers list
        if {$prefix == ""} {
            lappend headers $h
        } elseif {$prefix == $STD_INCLUDES} {
            # Append standard C headers to a separate list to exclude them from
            # the graph.
            lappend std_headers $h
        } else {
            lappend headers "$prefix/$h"
        }
    }
    
    # Iterate over all subdirectories
    foreach dir [glob -nocomplain -type d -tails -directory $path *] {
        # Add subdirectory name to the prefix
        if {$prefix == ""} {
            set rprefix $dir
        } else {
            set rprefix "$prefix/$dir"
        }
        
        # Call this procedure recursively for each subdirectory
        glob_recursive "$path/$dir" $rprefix
    }
}

# Generate a node name from a header file name. We want to:
#   1) Ensure the generated name does not contain characters that are invalid
#      for a node name in the Graphviz syntax;
#   2) Always generate the same node name for a given header file name; and
#   3) Generate different node names for different header file names.
#
# The currently implemented mangling procedure is to replace each period (.),
# path name separator (/) and dash (-) with a double underscore (__).
proc mangle {file_name} {
    set buffer ""
    
    for {set idx 0} {$idx < [string length $file_name]} {incr idx} {
        set c [string index $file_name $idx]
        
        if {$c == "/" || $c == "."|| $c == "-"} {
            set s "__"
        } else {
            set s $c
        }
        
        set buffer "$buffer$s"
    }
    
    return $buffer
}

# Prepend indentation to specified string.
proc indent {level str}  {
    global INDENT_STRING
    
    return [format "%s%s" [string repeat $INDENT_STRING $level] $str]
}

# Emit a comment with the specified string content in Graphviz syntax.
proc comment {str} {
    return [format "/* %s */" $str]
}

# Emit a node declaration for the specified header file in Graphviz syntax
proc node_decl {file_name} {
    switch -glob $file_name {
        "jinue/*"           {set shape box}
        "jinue-common/*"    {set shape octagon}
        default             {set shape ellipse}
    }
    
    return [format {%s[label="%s", shape="%s"];} [mangle $file_name] $file_name $shape]
}

# Emit an edge declaration between the specified header files in Graphviz syntax
proc edge_decl {file_from file_to} {
    return [format {%s -> %s;} [mangle $file_from] [mangle $file_to]]
}

# Initialize header files list to an empty list
set headers     [list]
set std_headers [list]

# Find all header files
glob_recursive $INCLUDES ""

# Graphviz syntax output starts here
puts "digraph hdeps {"
puts [indent 1 "ranksep = 2.2;"]
puts ""
puts [indent 1 [comment "node declarations"]]

# Emit node declaration for each header file
foreach h $headers {
    puts [indent 1 [node_decl $h]]
}

puts ""
puts [indent 1 [comment "dependencies"]]

# Emit dependency edges
foreach h $headers {
    # Open header file and read whole content
    set fd [open [file join $INCLUDES $h] r]
    set content [read $fd]
    close $fd
    
    # Split file content into individual lines
    set lines [split $content "\n"]
    
    # find #include directives
    foreach line $lines {
        if {[regexp {#include[ ]*<(.*)>} $line dummy included]} {
            # exclude standard C headers
            if {[lsearch $std_headers $included] < 0} {
                puts [indent 1 [edge_decl $included $h]]
            }
        }
    }
}

puts "}"
