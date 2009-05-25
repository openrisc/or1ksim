MEMORY
        {
        flash  : ORIGIN = 0xf0000000, LENGTH = 0x00200000
        ram    : ORIGIN = 0x00000500, LENGTH = 0x001fb000
        }
 
SECTIONS
{
      .reset :
        {
        *(.reset)
         _src_beg = .;
        } > flash
      .text : 
        AT ( ADDR (.reset) + SIZEOF (.reset) )
        {
        _dst_beg = .;
        *(.text)
        } > ram
      .data :
        AT ( ADDR (.reset) + SIZEOF (.reset) + SIZEOF (.text) )
        {
        *(.data)
        *(.rodata)
        _dst_end = .;
        } > ram
      .bss :
        {
        *(.bss)
        } > ram
}
