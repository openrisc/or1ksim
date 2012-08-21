#ifndef __CRC32_H
#define __CRC32_H


/* Calculate CRC of buffer in one go */
unsigned long crc32( const void *buf, unsigned len );

/* Calculate incrementally */
void crc32_init( unsigned long *value );
void crc32_feed_bytes( unsigned long *value, const void *buf, unsigned len );
void crc32_close( unsigned long *value );

#endif /* ___CRC32_H */
