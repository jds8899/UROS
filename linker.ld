SECTIONS
{
	. = 0x10000;
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.debug_gdb_script : { *(.debug_gdb_script) }
	.eh_frame : { *(.eh_frame) }
	.data.rel.ro : { *(.data.rel.ro) }
	.comment : { *(.comment) }
	.debug_aranges : { *(.debug_arranges) }
	.debug_pubnames : { *(.debug_pubnames) }
	.debug_info : { *(.debug_info) }
	.debug_abbrev : { *(.debug_abbrev) }
	.debug_line : { *(.debug_line) }
	.debug_frame : { *(.debug_frame) }
	.debug_str : { *(.debug_str) }
	.debug_macinfo : { *(.debug_macinfo) }
	.debug_pubtypes : { *(.debug_pubtypes) }
	.debug_ranges : { *(.debug_ranges) }
	.shstrtab : { 
		*(.shstrtab)
		_end = .;
	}
	.symtab : { *(.symtab) }
	.strtab : { *(.strtab) }
	.data : { *(.data) }
	.bss : {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
}
