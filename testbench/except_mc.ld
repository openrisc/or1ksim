MEMORY
        {
        except : ORIGIN = 0x00000000, LENGTH = 0x00002000
        flash  : ORIGIN = 0x00002000, LENGTH = 0x001fe000
        ram    : ORIGIN = 0x40000000, LENGTH = 0x00200000
        }
 
SECTIONS
{
      .except :
        {
        *(.except)
         _src_beg = .;
        } > except
      .text :
        AT ( ADDR (.except) + SIZEOF (.except) )
        {
        _dst_beg = .;
        *(.text)
        *(.rodata)
        } > ram
      .data :
        AT ( ADDR (.except) + SIZEOF (.except) + SIZEOF (.text))
        {
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
