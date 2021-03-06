#ifndef __MAGELLAN_H_
#define __MAGELLAN_H_

typedef enum _msgtype {track=1,waypt,route} msgtype;

extern void magchksum (char *data, char* chksum,UInt32 size);
extern void get_checksum(char* data,char* field,UInt32 fieldsize);
extern void get_field(char* data,char* field,UInt32 fieldsize, UInt16 fieldno);
extern void mag_get_data(msgtype msgt);
extern void get_mag_id(void);
extern void mag_upload(void);

#define GETID 0
#define DOWNTRACK 1
#define DOWNWAYPTS 2
#define DOWNROUTE 3
#define UPLOAD 4 
#endif
