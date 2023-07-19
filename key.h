/**
 * @file key.h
 * @author Donocean (1184427366@qq.com)
 * @brief 使用状态机+FIFO检测按键的短按、双击、长按
 * @version 1.0
 * @date 2023-07-19
 *
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __KEY_H_
#define __KEY_H_

/**
 * @brief 程序可检测的四种状态。短按，长按，长按松开，双击
 */
enum ekey_status
{
    KEY_SHORT_PRESS = 1,
    KEY_LONG_PRESS,
    KEY_DOUBLE_PRESS,
    KEY_LONG_RELEASE_UP,
};

/**
 * @brief 每次添加一个按键，先添加枚举类型，使用枚举类型进行方便管理
 */
enum ekey_num
{
    KEY0,
    /* 用户在此添加新的按键值 */
};

/**
 * @brief 按键检测函数，此函数需要1ms执行一次
 */
void key_scan(void);

/**
 * @brief 从按键FIFO中读取按状态
 *
 * @return 返回[两字节]按键值
 *         高字节: 存放按键值(参考枚举值enum Key_Num)
 *         低字节: 存放存放按键值状态(参考enum ekey_status)
 *
 * @note 请不要在按按键时阻塞此函数太久，如果fifo不够大将出现按键值覆盖情况
 */
unsigned short key_read(void);

#endif /* __KEY_H_ */
