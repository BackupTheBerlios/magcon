#ifndef __SER_H_
#define __SER_H_

extern UInt16 serlib;
extern Boolean ser_open(void);
extern void ser_close(void);
extern Boolean get_string(char* string,UInt16 size);
extern Boolean send_string(char* str,Boolean ack);

#endif
