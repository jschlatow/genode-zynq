# vim: ts=4 sw=4 et

proc parse_reg { out } {
    set value [expr 0x[lindex [split $out {: }] 2]]
    return $value
}

proc bitfield { value start len } {
    return [expr $value >> $start & [expr (1 << $len) - 1]]
}

proc bitfield2 { value start len start2 len2 } {
    return [expr [bitfield $value $start $len] | [bitfield $value $start2 $len2] << $len ]
}

proc masked { value start len } {
    return [expr $value & (((1 << $len) - 1) << $start)]
}

proc read_word { addr } {
    return [mrd -address-space PA -value 0x[format %x $addr]]
}

proc parse_page_lvl1 { addr desc } {
    set base [masked $desc 10 22]
    return [expr $base | [bitfield $addr 12 8] << 2]
}

proc parse_small_page { addr desc verbose} {
    set base [masked $desc 12 20]
    set properties [masked $desc 0 12]
    if { $verbose } {
        puts "XN:  [bitfield  $properties 0 1]"
        puts "B:   [bitfield  $properties 2 1]"
        puts "C:   [bitfield  $properties 3 1]"
        puts "TEX: 0x[format %x [bitfield  $properties 6 2]]"
        puts "AP:  0x[format %x [bitfield2 $properties 4 2 9 1]]"
        puts "S:   [bitfield  $properties 10 1]"
        puts "nG:  [bitfield  $properties 11 1]"
    }
    return [expr $base | [bitfield $addr 0 12]]
}

proc parse_section { addr desc verbose } {
    set base [masked $desc 20 12]
    set properties [masked $desc 0 20]
    set supersection [bitfield $properties 18 1]

    if { $supersection } {
        puts "Supersections not implemented"
        exit 1
    }

    if { $verbose } {
        puts "XN:  [bitfield  $properties 4 1]"
        puts "B:   [bitfield  $properties 2 1]"
        puts "C:   [bitfield  $properties 3 1]"
        puts "TEX: 0x[format %x [bitfield  $properties 12 2]]"
        puts "AP:  0x[format %x [bitfield2 $properties 10 2 15 1]]"
        puts "S:   [bitfield  $properties 16 1]"
        puts "nG:  [bitfield  $properties 17 1]"
    }
    return [expr $base | [bitfield $addr 0 20]]
}

proc walk {addr verbose} {
    if { $verbose } {
        puts "======================================================================"
        puts "==  Performing page table walk for 0x[format %08x $addr]"
    }

    set ttbr0 [parse_reg [rrd cp15 c2 ttbr0]]
    set ttbr1 [parse_reg [rrd cp15 c2 ttbr1]]
    set ttbcr [parse_reg [rrd cp15 c2 ttbcr]]
    set prrr  [parse_reg [rrd cp15 c10 prrr]]
    set nmrr  [parse_reg [rrd cp15 c10 nmrr]]
    set sctlr [parse_reg [rrd cp15 c1 sctlr]]
    set TRE [bitfield $sctlr 28 1]
    set ttbr $ttbr0
    set N   [bitfield $ttbcr 0 3]

    if { $N != 0 } {
        if { [bitfield $addr [expr 32-$N] $N] } {
            set ttbr $ttbr1
            set N 0
        }
    }

    #########################
    # Get translation table

    if { $verbose } {
        if { $TRE } {
            puts "PRRR: 0x[format %08x $prrr]"
            puts "NMRR: 0x[format %08x $nmrr]"
        }
        puts "TTBR0: 0x[format %08x $ttbr]"
        puts "  - RGN:  0x[format %x [bitfield  $ttbr 3 2]]"
        puts "  - IRGN: 0x[format %x [bitfield2 $ttbr 6 1 0 1]]"
        puts "  - S:    [bitfield  $ttbr 1 1]"
        puts "  - NOS:  [bitfield  $ttbr 5 1]"
    }

    set base [masked $ttbr [expr 14 - $N] [expr 18 + $N]]

    set tabidx [bitfield $addr 20 [expr 12 - $N]]

    set lvl1_addr [expr $base | ($tabidx << 2)]

    if { $verbose } {
        puts "Translation base: 0x[format %08x $base]"
        puts "1st level descriptor @ 0x[format %08x $lvl1_addr]"
    }

    #############################
    # Parse 1st level descriptor

    set lvl1_desc [read_word $lvl1_addr]
    set type [bitfield $lvl1_desc 0 2]
    if { $type == 0x1 } {
        set lvl2_addr [parse_page_lvl1 $addr $lvl1_desc]
        if { $verbose } {
            puts "2nd level descriptor @ 0x[format %08x $lvl2_addr]"
        }

        set lvl2_desc [read_word $lvl2_addr]

        set type2 [bitfield $lvl2_desc 0 2]
        if { $type2 == 0x1 } {
            puts "Large pages not implemented"
            exit 1
        } elseif { $type2 & 0x2 } {
            return [parse_small_page $addr $lvl2_desc $verbose]
        } else {
            puts "page fault on lvl 2"
            return 0
        }

    } elseif { $type & 0x2 } {
        return [parse_section $addr $lvl1_desc $verbose]
    } else {
        puts "page fault on lvl 1"
        return 0
    }
}

proc va_to_pa {addr verbose} {
    set paddr [walk $addr $verbose]
    puts "0x[format %08x $addr] -> 0x[format %08x $paddr]"
}

proc region_to_pa {addr size} {
    while { $size > 0 } {
        set paddr [walk $addr 0]
        puts "0x[format %08x $addr] -> 0x[format %08x $paddr]"
        set addr [expr $addr + 0x1000]
        set size [expr $size - 0x1000]
    }
}

if { $argc != 1 } {
    puts "The script requires an address as argument."
    puts "If called from interactive shell, you can run 'va_to_pa <address> <verbose>' manually."
} else {
    connect
    targets 2
    va_to_pa [expr [lindex $argv 0]] 1
}


