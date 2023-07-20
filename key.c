/**
 * @file key.c
 * @author Donocean (1184427366@qq.com)
 * @brief 使用状态机+FIFO检测按键的短按、双击、长按
 * @version 1.0
 * @date 2023-07-19
 *
 * @copyright Copyright (c) 2023
 * 
 * @note 按键按下的三种情况
 * |--------------------------------------------------------------------|
 * |  short click                                                       |
 * |  ______                   ________                                 |
 * |     |  \                 /  |                                      |
 * |     |   \_______________/   |                                      |
 * |     |     |           |     |                                      |
 * |     |shake|  < long   |shake|                                      |
 * |                                                                    |
 *  --------------------------------------------------------------------|
 * |  double click                                                      |
 * |  ______                   _____________                   ____     |
 * |     |  \                 /  |       |  \                 /  |      |
 * |     |   \_______________/   |       |   \_______________/   |      |
 * |     |     |           |     | < max |     |           |     |      |
 * |     |shake|  < long   |shake|dclick |shake|  any time |shake|      |
 * |                                                                    |
 *  --------------------------------------------------------------------|
 * |  long click                                                        |
 * |  ______                                           ________         |
 * |     |  \                                         /  |              |
 * |     |   \_______________________________________/   |              |
 * |     |     |                                   |     |              |
 * |     |shake|             > long click          |shake|              |
 * |                                                                    |
 * |--------------------------------------------------------------------|
 */

#include "key.h"

/* ---------------------------宏声明区--------------------------- */

#define KEY_PRESS_DOWN       0X00
#define KEY_RELEASE_UP       0X01
/* 按键FIFO空间大小, 取值一定要为2的n次方 */
#define KEY_MAX_FIFO_SIZE    0x10
/* 当前按键FIFO内无按键值 */
#define KEY_NONE_IN_FIFO     0xFFFF

/* 状态机状态 */
enum ekey_state
{
    KEY_STATE_CHECK_PRESS_DOWN,               /* 是否按下*/
    KEY_STATE_PRESS_DOWN_FILTER,              /* 按下滤波 */
    KEY_STATE_CHEACK_LONG,                    /* 长按检测 */
    KEY_STATE_SHORT_RELEASE_UP_FILTER,        /* 短按松开滤波 */
    KEY_STATE_CHECK_DOUBLE_PRESS,             /* 双击检测 */
    KEY_STATE_DOUBLE_PREESS_FILTER,           /* 双击滤波 */
    KEY_STATE_CHECK_DOUBLE_RELEASE_UP_FILTER, /* 双击松开滤波 */
    KEY_STATE_CHECK_LONG_RELEASE_UP_FILTER,   /* 长按松开滤波*/
};

/* ---------------------------结构体声明区--------------------------- */

typedef struct _key_dev
{
    unsigned char ucStatus;                /* 状态机状态 (取值参考enum ekey_state)  */
    unsigned char ucKeyNum;                /* 按键值, 即当前是哪个按键 (取值参考enum ekey_num) */
    unsigned short usCountTick;            /* 计数器 */
    unsigned short usLongClickPeriod;      /* 长按检测时间 */
    unsigned short usFilterPeriod;         /* 按键滤波时间 */
    unsigned short usMaxDoubleClickPeriod; /* 双击检测最大间隔时间 */
    unsigned char (*read_Key)(void);       /* 读按键是否按下函数 */
} key_dev;

struct _key_fifo
{
    unsigned char ucwrite;                  /* 写指针，只会++ */
    unsigned char ucread;                   /* 读指针，只会++ */
    unsigned short buff[KEY_MAX_FIFO_SIZE]; /* 按键fifo缓冲区 */
};

/* ---------------------------本文件私有函数声明区--------------------------- */

static void prvkey_check_key_status(key_dev *pKey_Dev);
static void prvkey_wirte_keyval(unsigned short ucKeyVal);
static void prvkey_state_check_press_down(key_dev *pKey_Dev);
static void prvkey_state_press_down_filter(key_dev *pKey_Dev);
static void prvkey_state_check_long(key_dev *pKey_Dev);
static void prvkey_state_short_release_up_filter(key_dev *pkey_dev);
static void prvkey_state_check_double_press(key_dev *pKey_Dev);
static void prvkey_state_double_press_filter(key_dev *pKey_Dev);
static void prvkey_state_check_double_release_up_filter(key_dev *pkey_dev);
static void prvkey_state_check_long_release_up_filter(key_dev *pKey_Dev);

/* ---------------------------用户函数声明区--------------------------- */
unsigned char uckey0_read(void)
{
    /* if条件检测GPIO按键的状态，用户需自行添加 */
    if (1) 
        return KEY_PRESS_DOWN;
    else
        return KEY_RELEASE_UP;
}

// unsigned char uckey1_read(void)
// {
//     /* if条件检测GPIO按键的状态，用户需自行添加 */
//     if (1) 
//         return KEY_PRESS_DOWN;
//     else
//         return KEY_RELEASE_UP;
// }

/* ---------------------------全局变量声明区--------------------------- */
struct _key_fifo key_fifo;

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

/**
 * @brief 按键检测函数，此函数需要1ms执行一次
 */
void key_scan(void)
{
    unsigned char i;
    static unsigned char key_num = sizeof(key_device) / sizeof(key_dev);

    for (i = 0; i < key_num; i++)
    {
        prvkey_check_key_status(&key_device[i]);
    }
}

/**
 * @brief 从按键FIFO中读取按键状态
 *
 * @return 返回[两字节]按键值
 *         低字节: 存放按键值    (参考枚举值enum ekey_num)
 *         高字节: 存放按键值状态 (参考enum ekey_status)
 *         若返回0xFFFF(KEY_NONE_IN_FIFO)，则说明没有检测到按键按下
 * 
 * @note 请不要在按按键时阻塞此函数太久，如果fifo不够大将出现按键值覆盖情况
 */
unsigned short key_read(void)
{
    unsigned short uckey_val;
    unsigned char uclen = key_fifo.ucread & (KEY_MAX_FIFO_SIZE - 1);

    if (key_fifo.ucread == key_fifo.ucwrite)
    {
        uckey_val = KEY_NONE_IN_FIFO;
    }
    else
    {
        uckey_val = key_fifo.buff[uclen];
        key_fifo.ucread++;
    }

    return uckey_val;
}

/**
 *@brief : 检测按键是否按下
 *@param : key_dev - key device pointer
 */
static void prvkey_state_check_press_down(key_dev *pKey_Dev)
{
    unsigned char key_read;

    key_read = pKey_Dev->read_Key();

    if (key_read == KEY_PRESS_DOWN)
    {
        /* 进入消抖状态 */
        pKey_Dev->ucStatus = KEY_STATE_PRESS_DOWN_FILTER;
        pKey_Dev->usCountTick = 0;
    }
}

/* 消抖状态 */
static void prvkey_state_press_down_filter(key_dev *pKey_Dev)
{
    unsigned char key_read;

    pKey_Dev->usCountTick++;

    if (pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
    {
        return;
    }

    /* 消抖时间结束，检查按键是否还在按下 */
    key_read = pKey_Dev->read_Key();

    if (key_read == KEY_PRESS_DOWN)
    {
        /* 消抖时间到，按键进入检查长按阶段 */
        pKey_Dev->ucStatus = KEY_STATE_CHEACK_LONG;
        pKey_Dev->usCountTick = 0;
    }
    else
    {
        /* 消抖时间到，按键没被按下，重新回到检测按下状态 */
        pKey_Dev->ucStatus = KEY_STATE_CHECK_PRESS_DOWN;
    }
}

static void prvkey_state_check_long(key_dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;

    pKey_Dev->usCountTick++;

    key_read = pKey_Dev->read_Key();

    /* 若在长按检测时间内，松开按钮则进入短按松开检测状态 */
    if (pKey_Dev->usCountTick < pKey_Dev->usLongClickPeriod)
    {
        if (key_read == KEY_RELEASE_UP)
        {
            /* 进入短按松开检测状态 */
            pKey_Dev->ucStatus = KEY_STATE_SHORT_RELEASE_UP_FILTER;
        }
    }
    else
    {
        /* 超过长按检测时间，说明是长按  */
        if (key_read == KEY_PRESS_DOWN)
        {
            /* 记录长按标志 */
            key_val = KEY_LONG_PRESS | pKey_Dev->ucKeyNum;
            prvkey_wirte_keyval(key_val);
            /* 进入长按松开检测 */
            pKey_Dev->ucStatus = KEY_STATE_CHECK_LONG_RELEASE_UP_FILTER;
        }
    }
}

/**
 *@brief : 按钮短按松开，进行松开消抖，同时检测是否为误判
 *@param : key_dev - key device pointer
 */
static void prvkey_state_short_release_up_filter(key_dev *pkey_dev)
{
    unsigned char key_read;
    static unsigned short old = 0xffff;

    if (old == 0xffff)
    {
        /* 记录按钮之前被按下了多久 */
        old = pkey_dev->usCountTick;
        /* count清零，然后进行松开消抖 */
        pkey_dev->usCountTick = 0;
    }

    pkey_dev->usCountTick++;

    if (pkey_dev->usCountTick < pkey_dev->usFilterPeriod)
        return;

    /* 超过消抖处理时间 */
    key_read = pkey_dev->read_Key();

    if (key_read == KEY_RELEASE_UP)
    {
        /* 按钮松开，接下来进入双击检测状态 */
        pkey_dev->ucStatus = KEY_STATE_CHECK_DOUBLE_PRESS;
        pkey_dev->usCountTick = 0;
    }
    else
    {
        /* 如果在松开消抖时检测到按钮还在被按下，说明之前是误判，重新进入长按检测状态 */
        pkey_dev->ucStatus = KEY_STATE_CHEACK_LONG;
        pkey_dev->usCountTick += old;
    }

    old = 0xffff;
}

static void prvkey_state_check_double_press(key_dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;

    pKey_Dev->usCountTick++;

    key_read = pKey_Dev->read_Key();

    if (pKey_Dev->usCountTick < pKey_Dev->usMaxDoubleClickPeriod)
    {
        /* 若在双击检测的间隔内按下，表示此次是双击 */
        if (key_read == KEY_PRESS_DOWN)
        {
            /* 进入双击消抖状态 */
            pKey_Dev->ucStatus = KEY_STATE_DOUBLE_PREESS_FILTER;
            pKey_Dev->usCountTick = 0;
        }
    }
    else /* 大于双击检测的最大间隔 */
    {
        /* 按键松开，说明此次是短按 */
        if (key_read == KEY_RELEASE_UP)
        {
            /* 回到最初检测按下状态 */
            key_val = KEY_SHORT_PRESS | pKey_Dev->ucKeyNum;
            prvkey_wirte_keyval(key_val);
            pKey_Dev->ucStatus = KEY_STATE_CHECK_PRESS_DOWN;
        }
        else
        {
            /* 进入短按消抖 */
            pKey_Dev->ucStatus = KEY_STATE_PRESS_DOWN_FILTER;
            pKey_Dev->usCountTick = 0;
        }
    }
}

static void prvkey_state_double_press_filter(key_dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;
    static unsigned short old = 0xffff;

    if (old == 0xffff)
    {
        old = pKey_Dev->usCountTick;
        pKey_Dev->usCountTick = 0;
    }

    pKey_Dev->usCountTick++;

    if (pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
        return;

    key_read = pKey_Dev->read_Key();

    /* 超过消抖时间，按键被按下，说明是双击 */
    if (key_read == KEY_PRESS_DOWN)
    {
        /* 记录双击状态， */
        key_val = KEY_DOUBLE_PRESS | pKey_Dev->ucKeyNum;
        prvkey_wirte_keyval(key_val);
        pKey_Dev->ucStatus = KEY_STATE_CHECK_DOUBLE_RELEASE_UP_FILTER;
        pKey_Dev->usCountTick = 0;
    }
    else /* 超过消抖时间，按键是松开的，说明之前是误判，重新回到双击检测阶段 */
    {
        pKey_Dev->ucStatus = KEY_STATE_CHECK_DOUBLE_PRESS;
        pKey_Dev->usCountTick += old;
    }

    old = 0xffff;
}

static void prvkey_state_check_double_release_up_filter(key_dev *pKey_Dev)
{
    unsigned char ucKey_Read;

    ucKey_Read = pKey_Dev->read_Key();

    /* 若还在按着，则直接返回 */
    if (ucKey_Read == KEY_PRESS_DOWN)
        return;
    
    /* 进行松开消抖 */
    pKey_Dev->usCountTick++;

    if (pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
        return;
    
    /* 双击松开，回到最初检测 */
    if (ucKey_Read == KEY_RELEASE_UP)
        pKey_Dev->ucStatus = KEY_STATE_CHECK_PRESS_DOWN;
}

static void prvkey_state_check_long_release_up_filter(key_dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;
    key_read = pKey_Dev->read_Key();

    /* 若长按还在进行，则直接返回 */
    if (key_read == KEY_PRESS_DOWN)
        return;

    /* 若长按松开，进行松开消抖 */
    pKey_Dev->usCountTick++;

    if (pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
        return;

    if (key_read == KEY_RELEASE_UP)
    {
        /* 消抖时间到，回到最初检测按下状态 */
        key_val =  KEY_LONG_RELEASE_UP | pKey_Dev->ucKeyNum;
        prvkey_wirte_keyval(key_val);
        pKey_Dev->ucStatus = KEY_STATE_CHECK_PRESS_DOWN;
    }
}

static void prvkey_check_key_status(key_dev *pKey_Dev)
{
    switch (pKey_Dev->ucStatus)
    {
    case KEY_STATE_CHECK_PRESS_DOWN:
        prvkey_state_check_press_down(pKey_Dev);
        break;
    case KEY_STATE_PRESS_DOWN_FILTER:
        prvkey_state_press_down_filter(pKey_Dev);
        break;
    case KEY_STATE_CHEACK_LONG:
        prvkey_state_check_long(pKey_Dev);
        break;
    case KEY_STATE_SHORT_RELEASE_UP_FILTER:
        prvkey_state_short_release_up_filter(pKey_Dev);
        break;
    case KEY_STATE_CHECK_DOUBLE_PRESS:
        prvkey_state_check_double_press(pKey_Dev);
        break;
    case KEY_STATE_DOUBLE_PREESS_FILTER:
        prvkey_state_double_press_filter(pKey_Dev);
        break;
    case KEY_STATE_CHECK_DOUBLE_RELEASE_UP_FILTER:
        prvkey_state_check_double_release_up_filter(pKey_Dev);
        break;
    case KEY_STATE_CHECK_LONG_RELEASE_UP_FILTER:
        prvkey_state_check_long_release_up_filter(pKey_Dev);
        break;
    }
}

/**
 * @brief 将检测的按键值写入fifo(私有函数，内部调用)
 *
 * @param ucKeyVal 按键值按两字节存储
 *                | 高字节存放按键值状态 | 低字节存放存放按键值 |
 */
static void prvkey_wirte_keyval(unsigned short ucKeyVal)
{
    /* 计算出从buffer的初始位置到wirte这一段的长度len */
    unsigned char uclen = key_fifo.ucwrite & (KEY_MAX_FIFO_SIZE - 1);

    key_fifo.buff[uclen] = ucKeyVal;
    key_fifo.ucwrite++;
}
