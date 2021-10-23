#include "key.h"
/*
 --------------------------------------------------------------------
|  short click                                                       |
|  ______                   ________                                 |
|     |  \                 /  |                                      |
|     |   \_______________/   |                                      |
|     |     |           |     |                                      |
|     |shake|  < long   |shake|                                      |
|                                                                    |
 -------------------------------------------------------------------
|  double click                                                      |
|  ______                   _____________                   ____     |
|     |  \                 /  |       |  \                 /  |      |
|     |   \_______________/   |       |   \_______________/   |      |
|     |     |           |     | < max |     |           |     |      |
|     |shake|  < long   |shake|dclick |shake|  any time |shake|      |
|                                                                    |
 --------------------------------------------------------------------
|  long click                                                        |
|  ______                                           ________         |
|     |  \                                         /  |              |
|     |   \_______________________________________/   |              |
|     |     |                                   |     |              |
|     |shake|             > long click          |shake|              |
|                                                                    |
 --------------------------------------------------------------------
*/

/* ---------------------------宏声明区--------------------------- */

/* 按键FIFO空间大小 */
#define KEY_MAX_FIFO_SIZE    0x10   
/* 当前按键FIFO内无按键值 */
#define KEY_NONE_IN_FIFO     0xFF

/* 按键是否按下状态 */
#define KEY_PRESS_DOWN   0X00
#define KEY_RELEASE_UP   0X01

/* 状态机状态值 */
#define KEY_STATUS_CHECK_PRESS_DOWN               0X00
#define KEY_STATUS_PRESS_DOWN_FILTER              0X01
#define KEY_STATUS_CHEACK_LONG                    0X02
#define KEY_STATUS_SHORT_RELEASE_UP_FILTER        0X03
#define KEY_STATUS_CHECK_DOUBLE_PRESS             0X04
#define KEY_STATUS_DOUBLE_PREESS_FILTER           0X05
#define KEY_STATUS_CHECK_DOUBLE_RELEASE_UP_FILTER 0X06
#define KEY_STATUS_CHECK_LONG_RELEASE_UP_FILTER   0X07


/* ---------------------------结构体声明区--------------------------- */
typedef struct _key_Dev
{
    unsigned char ucStatus;     /* 状态机状态  */
    unsigned char ucKeyNum;     /* 按键值，即当前是哪个按键 */
    unsigned short usCountTick; /* 计数器 */
    unsigned short usLongClickPeriod;   /* 长按检测时间 */
    unsigned short usFilterPeriod;   /* 按键滤波时间 */
    unsigned short usMaxDoubleClickPeriod; /* 双击检测最大间隔时间 */
    unsigned char (*Read_Key) (void); /* 读按键是否按下函数 */
} Key_Dev;

/* ---------------------------全局变量声明区--------------------------- */
struct
{
    unsigned short buff[KEY_MAX_FIFO_SIZE];
    unsigned char ucwrite; /* 写指针，只会++ */
    unsigned char ucread;  /* 读指针，只会++ */
}Key_FIFO;

Key_Dev Key_Device[] = {
    {
        KEY_STATUS_CHECK_PRESS_DOWN,
        key0,
        0,
        1500,
        20,
        300,
        ucKey0_Read
    },
	
	/* 用户在此添加新的按键设备 */
	
};

/* ---------------------------外部变量声明区--------------------------- */



/* ---------------------------本文件私有函数声明区--------------------------- */
static void prvKeyWirteKeyVal(unsigned short ucKeyVal);
static void prvkey_status_check_press_down(Key_Dev *pKey_Dev);
static void prvprvkey_status_press_down_filter(Key_Dev *pKey_Dev);
static void prvkey_status_check_long(Key_Dev *pKey_Dev);
static void prvkey_status_short_release_up_filter(Key_Dev *pkey_dev);
static void prvkey_status_check_double_press(Key_Dev *pKey_Dev);
static void prvkey_status_double_press_filter(Key_Dev *pKey_Dev);
static void prvkey_status_check_double_release_up_filter(Key_Dev *pkey_dev);
static void prvkey_status_check_long_release_up_filter(Key_Dev *pKey_Dev);
static void prvkey_check_key_status(Key_Dev *pKey_Dev);
/* -----------------------------------END---------------------------------------- */
unsigned char ucKey0_Read(void)
{
    if(1) /* if条件检测GPIO按键的状态 */
    {
        return KEY_PRESS_DOWN;
    }
    else
    {
        return KEY_RELEASE_UP;
    }
}

unsigned char ucKey1_Read(void)
{
    if(1) /* if条件检测GPIO按键的状态 */
    {
        return KEY_PRESS_DOWN;
    }
    else
    {
        return KEY_RELEASE_UP;
    }
}

/**
  *@brief : 检测按键是否按下
  *@param : key_dev - key device pointer
*/
static void prvkey_status_check_press_down(Key_Dev *pKey_Dev)
{
    unsigned char key_read;
    
    key_read = pKey_Dev->Read_Key();
    
    if(key_read == KEY_PRESS_DOWN)
    {
        /* 进入消抖状态 */
        pKey_Dev->ucStatus = KEY_STATUS_PRESS_DOWN_FILTER;
        pKey_Dev->usCountTick = 0;
    }
}

/* 消抖状态 */
static void prvprvkey_status_press_down_filter(Key_Dev *pKey_Dev)
{
    unsigned char key_read;

    pKey_Dev->usCountTick++;

    if(pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
    {
        return ;
    }

    /* 消抖时间结束，检查按键是否还在按下 */
    key_read = pKey_Dev->Read_Key();
    
    if(key_read == KEY_PRESS_DOWN)
    {
        /* 消抖时间到，按键进入检查长按阶段 */
        pKey_Dev->ucStatus = KEY_STATUS_CHEACK_LONG;
        pKey_Dev->usCountTick = 0;
    }
    else
    {
        /* 消抖时间到，按键没被按下，重新回到检测按下状态 */
         pKey_Dev->ucStatus = KEY_STATUS_CHECK_PRESS_DOWN;
    }
}

static void prvkey_status_check_long(Key_Dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;

    pKey_Dev->usCountTick++;
    
    key_read = pKey_Dev->Read_Key();

    /* 若在长按检测时间内，松开按钮则进入短按松开检测状态 */
    if(pKey_Dev->usCountTick < pKey_Dev->usLongClickPeriod )
    {
        if(key_read == KEY_RELEASE_UP)
        {
            /* 进入短按松开检测状态 */
            pKey_Dev->ucStatus = KEY_STATUS_SHORT_RELEASE_UP_FILTER;
        }
    }
    else
    {
        /* 超过长按检测时间，说明是长按  */
        if(key_read == KEY_PRESS_DOWN)
        {
            /* 记录长按标志 */
            key_val = (pKey_Dev->ucKeyNum<<8) | key_long_press;
            prvKeyWirteKeyVal(key_val);
            /* 进入长按松开检测 */
            pKey_Dev->ucStatus = KEY_STATUS_CHECK_LONG_RELEASE_UP_FILTER;
        }
    }
}

/**
  *@brief : 按钮短按松开，进行松开消抖，同时检测是否为误判
  *@param : key_dev - key device pointer
*/
static void prvkey_status_short_release_up_filter(Key_Dev *pkey_dev)
{
    unsigned char key_read;
    static unsigned short old = 0xffff;
 
    if(old == 0xffff)
    {
        /* 记录按钮之前被按下了多久 */
        old = pkey_dev->usCountTick;
        /* count清零，然后进行松开消抖 */ 
        pkey_dev->usCountTick = 0;  
    }
 
    pkey_dev->usCountTick++;
    
    if(pkey_dev->usCountTick < pkey_dev->usFilterPeriod)
    {
        return ;
    }
    
    /* 超过消抖处理时间 */
    key_read = pkey_dev->Read_Key();
 
    if(key_read == KEY_RELEASE_UP)
    {
        /* 按钮松开，接下来进入双击检测状态 */
        pkey_dev->ucStatus = KEY_STATUS_CHECK_DOUBLE_PRESS; 
        pkey_dev->usCountTick = 0;
    }
    else
    {
        /* 如果在松开消抖时检测到按钮还在被按下，说明之前是误判，重新进入长按检测状态 */
        pkey_dev->ucStatus = KEY_STATUS_CHEACK_LONG;
        pkey_dev->usCountTick += old;
    }
 
    old = 0xffff;
}

static void prvkey_status_check_double_press(Key_Dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;

     pKey_Dev->usCountTick++;

     key_read = pKey_Dev->Read_Key();

     if(pKey_Dev->usCountTick < pKey_Dev->usMaxDoubleClickPeriod)
     {
         /* 若在双击检测的间隔内按下，表示此次是双击 */
        if(key_read == KEY_PRESS_DOWN)
        {
            /* 进入双击消抖状态 */
            pKey_Dev->ucStatus = KEY_STATUS_DOUBLE_PREESS_FILTER;
            pKey_Dev->usCountTick  = 0;
        }
     }
     else /* 大于双击检测的最大间隔 */
     {
         /* 按键松开，说明此次是短按 */
         if(key_read == KEY_RELEASE_UP)
         {
            /* 回到最初检测按下状态 */
            key_val = (pKey_Dev->ucKeyNum<<8) | key_short_press;
            prvKeyWirteKeyVal(key_val);
            pKey_Dev->ucStatus = KEY_STATUS_CHECK_PRESS_DOWN;
         }
         else
         {
             /* 进入短按消抖 */
            pKey_Dev->ucStatus = KEY_STATUS_PRESS_DOWN_FILTER;
            pKey_Dev->usCountTick  = 0;  
         }
     }
}

static void prvkey_status_double_press_filter(Key_Dev *pKey_Dev)
{
    unsigned char key_read;
    unsigned short key_val;
    static unsigned short old =0xffff;

    if(old == 0xffff)
    {
        old = pKey_Dev->usCountTick;
        pKey_Dev->usCountTick = 0;
    }

    pKey_Dev->usCountTick++;

    if(pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
    {
        return ;
    }
    
    key_read = pKey_Dev->Read_Key();

    /* 超过消抖时间，按键被按下，说明是双击 */
    if(key_read == KEY_PRESS_DOWN)
    {
        /* 记录双击状态， */
        key_val = (pKey_Dev->ucKeyNum<<8) | key_double_press;
        prvKeyWirteKeyVal(key_val);
        pKey_Dev->ucStatus = KEY_STATUS_CHECK_DOUBLE_RELEASE_UP_FILTER;
        pKey_Dev->usCountTick = 0;
    }
    else /* 超过消抖时间，按键是松开的，说明之前是误判，重新回到双击检测阶段 */
    {
        pKey_Dev->ucStatus = KEY_STATUS_CHECK_DOUBLE_PRESS;
        pKey_Dev->usCountTick += old;
    }
    old =0xffff;
}

static void prvkey_status_check_double_release_up_filter(Key_Dev *pKey_Dev)
{
    unsigned char ucKey_Read;
	
    ucKey_Read = pKey_Dev->Read_Key();
    
    /* 若还在按着，则直接返回 */
    if(ucKey_Read == KEY_PRESS_DOWN)
    {
        return ;
    }
		
    /* 进行松开消抖 */
    pKey_Dev->usCountTick++;

    if(pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
    {
        return ;
    }
    
    /* 双击松开，回到最初检测 */
    if(ucKey_Read == KEY_RELEASE_UP)
    {
        pKey_Dev->ucStatus = KEY_STATUS_CHECK_PRESS_DOWN;
    }
}
static void prvkey_status_check_long_release_up_filter(Key_Dev *pKey_Dev)
{
    unsigned char key_read;
     unsigned short key_val;
    key_read = pKey_Dev->Read_Key();
    
    /* 若长按还在进行，则直接返回 */
    if(key_read == KEY_PRESS_DOWN)
    {
        return ;
    }
    
    /* 若长按松开，进行松开消抖 */
    pKey_Dev->usCountTick++;

    if(pKey_Dev->usCountTick < pKey_Dev->usFilterPeriod)
    {
        return ;
    }

    if(key_read == KEY_RELEASE_UP)
    {
        /* 消抖时间到，回到最初检测按下状态 */
        key_val = (pKey_Dev->ucKeyNum<<8) | key_long_release_up; 
        prvKeyWirteKeyVal(key_val);
        pKey_Dev->ucStatus = KEY_STATUS_CHECK_PRESS_DOWN;
    }
}

static void prvkey_check_key_status(Key_Dev *pKey_Dev)
{
    switch (pKey_Dev->ucStatus)
    {
        case KEY_STATUS_CHECK_PRESS_DOWN:
            prvkey_status_check_press_down(pKey_Dev);
            break;
        case KEY_STATUS_PRESS_DOWN_FILTER:
            prvprvkey_status_press_down_filter(pKey_Dev);
            break;
        case KEY_STATUS_CHEACK_LONG:
            prvkey_status_check_long(pKey_Dev);
            break;
        case KEY_STATUS_SHORT_RELEASE_UP_FILTER:
            prvkey_status_short_release_up_filter(pKey_Dev);
            break;
        case KEY_STATUS_CHECK_DOUBLE_PRESS:
            prvkey_status_check_double_press(pKey_Dev);
            break;
        case KEY_STATUS_DOUBLE_PREESS_FILTER:
            prvkey_status_double_press_filter(pKey_Dev);
            break;
        case KEY_STATUS_CHECK_DOUBLE_RELEASE_UP_FILTER:
            prvkey_status_check_double_release_up_filter(pKey_Dev);
            break;
        case KEY_STATUS_CHECK_LONG_RELEASE_UP_FILTER:
            prvkey_status_check_long_release_up_filter(pKey_Dev);
            break;
    }
}

/**
  *@brief 若想精确检测按键状态
  *	      此函数放入1ms定时器中断内进行检测 */
void vkey_check_all_key(void)
{
    unsigned char key_num,i;
    key_num = sizeof(Key_Device) / sizeof(Key_Dev);

    for(i=0;i<key_num;i++)
    {
        prvkey_check_key_status(&Key_Device[i]);
    }
}

/* 两字节 | 高字节存放按键值 | 低字节存放存放按键值状态  | */
static void prvKeyWirteKeyVal(unsigned short ucKeyVal)
{
    /* 计算出从buffer的初始位置到wirte这一段的长度len */
    unsigned char uclen = Key_FIFO.ucwrite & (KEY_MAX_FIFO_SIZE - 1);
    Key_FIFO.buff[uclen] = ucKeyVal;
    Key_FIFO.ucwrite++;
}

/**
  *@brief 从按键FIFO中读取按状态
  *@return 返回KeyVal，keyval高字节存放按键值,低字节存放存放按键值状态 */
unsigned short ucKeyReadKeyVal(void)
{
    unsigned short uckey_val;
    unsigned char uclen = Key_FIFO.ucread & (KEY_MAX_FIFO_SIZE - 1);

    if(Key_FIFO.ucread == Key_FIFO.ucwrite)
    {
        uckey_val = KEY_NONE_IN_FIFO;
    }
    else
    {
        uckey_val =  Key_FIFO.buff[uclen];
        Key_FIFO.ucread++;
    }
    
    return uckey_val;
}
