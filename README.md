# KeyWithStatusMachine

基于`状态机+FIFO`的按键检测，可以检测

- 短按
- 长按
- 双击

# 基本原理

按键按下最常见的三种情况如下所示，本库按键检测基于此进行设计:

```
 |--------------------------------------------------------------------|
 |  short click                                                       |
 |  ______                   ________                                 |
 |     |  \                 /  |                                      |
 |     |   \_______________/   |                                      |
 |     |     |           |     |                                      |
 |     |shake|  < long   |shake|                                      |
 |                                                                    |
  --------------------------------------------------------------------|
 |  double click                                                      |
 |  ______                   _____________                   ____     |
 |     |  \                 /  |       |  \                 /  |      |
 |     |   \_______________/   |       |   \_______________/   |      |
 |     |     |           |     | < max |     |           |     |      |
 |     |shake|  < long   |shake|dclick |shake|  any time |shake|      |
 |                                                                    |
  --------------------------------------------------------------------|
 |  long click                                                        |
 |  ______                                           ________         |
 |     |  \                                         /  |              |
 |     |   \_______________________________________/   |              |
 |     |     |                                   |     |              |
 |     |shake|             > long click          |shake|              |
 |                                                                    |
 |--------------------------------------------------------------------|
```

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
    /* 用户在此添加新的按键值, 例如MY_KEY */
    MY_KEY,
};
```

### 三、进入`key.c`添加按键设备

假设`key_scan()`函数执行周期为`1ms`，则添加一个按键设备，要求:

1. 设置按键为`MY_KEY`，后续添加新的按键需要参考上一步（步骤二）操作
2. 按键按下和松开的**滤波时间**为`20ms`
3. 按键的长按检测时间为`1500ms`，即:
    - 按下小于`1500ms`时，松开按键便可触发**短按状态**
    - 按下超过`1500ms`时，便可触发**长按状态**
4. 按键在`1500ms`内松开(短按松开)，并且在小于`300ms`的时间间隔内再次按下，则检测到**双击状态**

```c
key_dev key_device[] = {
    {
        /* 按键初始状态 */
        KEY_STATE_CHECK_PRESS_DOWN,
        /* 当前是哪个按键 (取值参考enum ekey_num) */
        MY_KEY,
       /* 计数器默认为0 */
        0,
        /* 长按检测时间，若1ms执行一次key_scan()，则检测时间单位就是1ms */
        1500,
        /* 按键滤波时间 */
        20,
        /* 双击检测最大间隔时间 */
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


用法实例：
```c
while (1)
{
    unsigned short key_value = key_read();

    if (key_value != KEY_NONE_IN_FIFO)
    {
        unsigned char key_num =  key_value & 0xFF;
        unsigned char key_status =  key_value & 0xFF00;

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
