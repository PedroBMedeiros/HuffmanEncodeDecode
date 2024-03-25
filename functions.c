#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/* utility function to test for empty file */
int file_is_empty(int file){
  int result;
  unsigned char buffer[1];
  off_t offset;
  result = read(file, &buffer, 1);
  if (result > 0) {
    offset = lseek(file, 0, SEEK_SET); /* go back to beginning of file */
    if (offset == -1) {
        perror("lseek");
        return -1;
    }
  }
  return result;
}
/* utility function to convert unisgned char to strings of 0s and 1s */
int char_to_8_bit_string(unsigned char buf, char* eight_bits) {
    int i;
    unsigned char bitmask;
    bitmask = 1 << 7; /* intial mask = '10000000'  to pick up first bit*/
    for (i = 0; i < 8; i++) {
        eight_bits[i] = (buf & bitmask) ? '1' : '0';  /* masks all bits 
                                                    except position i*/
        bitmask = bitmask >> 1; /* move mask to next bit*/
        }
    return 0;
}