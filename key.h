#ifndef __KEY_H_
#define __KEY_H_

typedef void *KeyHandle_t;


enum eKey_status
{
    key_short_press=1,
    key_long_press,
    key_double_press,
    key_long_release_up,
};

enum Key_Num
{
    key0,
    key1,
    key2,
    key3,
};

unsigned char ucKey0_Read(void);

void vkey_check_all_key(void);
unsigned short ucKeyReadKeyVal(void);
#endif /* __KEY_H_ */
