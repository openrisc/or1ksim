MEMORY
        {
        except : ORIGIN = 0x00000000, LENGTH = 0x00002000
        flash  : ORIGIN = 0xf0000000, LENGTH = 0x00200000
        ram    : ORIGIN = 0x00002000, LENGTH = 0x001fe000
        }
 
SECTIONS
{
      .text :
        {
        *(.text)
        *(.rodata)
         _src_beg = .;
        } > flash
      .dummy ALIGN(0x4):
        {
         _src_beg = .;
        } > flash
      .except :
        AT ( ADDR (.dummy))
        {
        _except_beg = .;
        *(.except)
        _except_end = .;
        } > except
      .data :
        AT ( ADDR (.dummy) + SIZEOF (.except))
        {
        _dst_beg = .;
        *(.data)
        _dst_end = .;
        } > ram
      .bss :
        {
        *(.bss)
        } > ram
      .stack  ALIGN(0x10) (NOLOAD):
        {
        *(.stack)
        _ram_end = .;
        } > ram
}
