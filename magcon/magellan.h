#ifndef __MAGELLAN_H_
#define __MAGELLAN_H_

extern void magchksum (char *Data, char* chksum,UInt32 size);
extern void get_checksum(char* data,char* field,UInt32 fieldsize);
extern void get_field(char* data,char* field,UInt32 fieldsize, UInt16 fieldno);

#endif
