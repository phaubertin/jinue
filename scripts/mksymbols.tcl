#!/usr/bin/tclsh

if {$argc != 2} {
    puts "USAGE: $argv0 input_file output_file"
    exit 1
}

set input_file  [lindex $argv 0]
set output_file [lindex $argv 1]

if {[file exists $input_file] != 1} {
    puts "Input file $input_file does not exist."
    exit 2
}

set in  [open $input_file r]
set out [open $output_file w]

puts $out {#include <debug.h>}
puts $out {#include <jinue/types.h>}
puts $out {}
puts $out "debugging_symbol_t debugging_symbols_table\[\] = {"

while { [gets $in line] >= 0} {
    set fields [split $line]
    
    set addr [lindex $fields 0]
    set type [lindex $fields 1]
    set name [lindex $fields 2]
    
    if {$type != T && $type != t} continue
    
    puts $out "\t{(addr_t)0x$addr, \"$type\", \"$name\"},"
}

puts $out "\t{(addr_t)0, (char *)NULL, (char *)NULL}"
puts $out "};"

close $in
close $out
