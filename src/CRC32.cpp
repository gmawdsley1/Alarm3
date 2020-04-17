/*
  CRC32

  A CRC 32 (Cyclic redundancy check) implementation for Arduino
  Adapted from http://excamera.com/sphinx/article-crc.html
*/
#include <Arduino.h>

// Define lookup table and store in flash memory
static const PROGMEM uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

/**
* Updates the provided crc hash with the new byte
*
* crc: hash
* data: the new byte to add to the hash
*
* Sample: 
*    unsigned long hash = 0L;
*    hash = crc_update(hash, 212);
*    hash = crc_update(hash, 32);
*    hash = ~hash;
*
* Usage:
*   -Used to validate I2C data transfrers
*/
unsigned long crc_update(unsigned long crc, byte data){
    byte tbl_idx;
    tbl_idx = crc ^ (data >> (0 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    tbl_idx = crc ^ (data >> (1 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    return crc;
}

