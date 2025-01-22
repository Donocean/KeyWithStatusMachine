# KeyWithStatusMachine

基于`状态机+FIFO`的按键检测，可以检测

- 短按
- 长按
- 双击

# 如何开始使用？

### 一、进入`key.c`添加按键检测函数

```c
// 请参考已有的模板添加按键检测函数
unsigned char ucKey0_Read(void)
{
    /* if条件检测GPIO按键的状态，用户需自行添加 */
    if (1) 
        return KEY_PRESS_DOWN;
    else
        return KEY_RELEASE_UP;
}

```

### 二、进入`key.h`添加enum类型

```c
/**
 * @brief 每次添加一个按键，先添加枚举类型，使用枚举类型进行方便管理
 */
enum ekey_num
{
    KEY0,
    /* 用户在此添加新的按键值 */
};
```

### 三、进入`key.c`添加按键设备

```c
// 请参考已有的模板添加按键检测函数，其中KEY0可以换成自己的按键
key_dev key_device[] = {
    {
        KEY_STATE_CHECK_PRESS_DOWN,
        KEY0,
        0,
        1500,
        20,
        300,
        uckey0_read
    },
    /* 用户在此添加新的按键设备 */

};
```

### 四、读取和检测

程序使用简单，只有两个API函数

- 检测按键: ` void key_scan(void)`
- 读按键fifo: `unsigned short key_read(void)`


- 用法实例：

```c
while (1)
{
    unsigned short key_value = key_read();

    if (key_value != KEY_NONE_IN_FIFO)
    {
        unsigned char key_num =  key_value && 0xFF;
        unsigned char key_status =  key_value >> 8;

        switch (key_num)
        {
            case KEY0:
                if (key_status == KEY_SHORT_PRESS)
                {
                    /* do something...  */
                }
                break;
            case KEY1:
                    /* do something...  */
        }
    }
}
```

更多信息，请参考注释说明进行使用

---

关于长按检测说明

> 当用户进行长按时，程序会检测到长按标志，用户可以利用此状态来进行一些处理，当用户松开时又会检测到一次状态，此状态为长按松开标志。

# 问题回馈

任何程序问题可以

- 提交 issues
- QQ: 1184427366
- B站: [Donocean](https://space.bilibili.com/7336549)
