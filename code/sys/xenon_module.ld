/* Bracket writable module state so Sys_LoadGameDll can restore it on reload. */
SECTIONS
{
  .data : {
    __module_data_start = .;
    *(.data .data.*)
    __module_data_end = .;
  }
  .bss : {
    __module_bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    __module_bss_end = .;
  }
}