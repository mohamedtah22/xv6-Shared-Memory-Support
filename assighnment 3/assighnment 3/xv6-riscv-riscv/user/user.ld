OUTPUT_ARCH("riscv")
ENTRY(_main)
PHDRS {
  text PT_LOAD FLAGS(5); /* R-X */
  data PT_LOAD FLAGS(6); /* RW- */
}
SECTIONS {
  . = 0;
  .text : { *(.text .text.*) } :text
  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) *(.rodata .rodata.*)
  } :text
  . = ALIGN(0x1000);
  .data : { *(.sdata .sdata.*) *(.data .data.*) } :data
  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) *(.bss .bss.*) *(COMMON)
  } :data
  PROVIDE(end = .);
}
