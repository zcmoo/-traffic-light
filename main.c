#include<reg51.h>

// 引脚定义
sbit P10 = P1^0;
sbit P11 = P1^1;
sbit P12 = P1^2;
sbit P13 = P1^3;
sbit sn_red = P3^0;
sbit sn_yellow = P3^1;
sbit sn_green = P3^2;
sbit ew_green = P3^3;
sbit ew_yellow = P3^4;
sbit ew_red = P3^5;
sbit button_stop = P1^4;
sbit button_slow = P1^5;

// 枚举定义
enum Direction {EW, SN};
enum Location {LEFT, RIGHT};
enum SnState {SN_RED, SN_GREEN, SN_YELLOW};  // 南北向状态
enum EwState {EW_GREEN, EW_YELLOW, EW_RED};  // 东西向状态
enum ButtonState {IDLE, STOP, SLOW};

// 全局变量
bit stop_state = 0;
bit slow_state = 0;
int sn_units_place = 5;  // 南北向个位
int sn_tens_place = 2;   // 南北向十位
int ew_units_place = 0;  // 东西向个位
int ew_tens_place = 2;   // 东西向十位
unsigned char code disp[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88};  // 段码表（0-9）

// 全局状态变量
enum SnState current_sn_state;
enum EwState current_ew_state;
enum ButtonState current_button_state;

// 函数声明
void state_init();
void Timer0_init();
void delay(int x);
void handle_digit_carry(int count, int *units_place, int *tens_place);  
void update_segment_disp(enum Direction dir, enum Location location, unsigned char Num);
bit is_stop_botton_pressed();
bit is_slow_botton_pressed();
void update_sn_state();
void update_ew_state();
void update_botton_state();

// 状态初始化
void state_init() {
    current_sn_state = SN_RED;      // 初始：南北红灯
    current_ew_state = EW_GREEN;    // 初始：东西绿灯
    current_button_state = IDLE;    // 按钮初始状态
}

// 定时器0初始化（1ms中断）
void Timer0_init() {
    TMOD &= 0xf0;
    TMOD |= 0x01;  // 模式1：16位定时器
    TL0 = 0x18;    // 初值装载（1ms@12MHz）
    TH0 = 0xfc;
    TF0 = 0;       // 清除标志
    TR0 = 1;       // 启动定时器
    ET0 = 1;       // 允许中断
    EA = 1;        // 总中断允许
    PT0 = 0;       // 低优先级
}

// 延时函数
void delay(int x) {
    while(x--);
}

// 处理数字进位
void handle_digit_carry(int count, int *units_place, int *tens_place) {
    if (count >= 1000) {  
        *units_place -= 1;
        if (*units_place < 0) {
            *units_place = 9;  // 个位归9
            *tens_place -= 1;  // 十位减1
            if (*tens_place < 0) {
                *tens_place = 0;  // 防止十位为负
            }
        }
    }
}

// 更新数码管显示
void update_segment_disp(enum Direction dir, enum Location location, unsigned char Num) {
    // 消隐（先关闭所有段选）
    P0 = 0xff;
    P2 = 0xff;

    switch(dir) {
        case EW:  // 东西向数码管
            switch(location) {
                case LEFT:   // 十位
                    P10 = 1;  
                    P11 = 0;
                    break;
                case RIGHT:  // 个位
                    P10 = 0;
                    P11 = 1;  
                    break;
            }
            P2 = disp[Num];  // 段选输出
            break;
        case SN:  // 南北向数码管
            switch(location) {
                case LEFT:   // 十位
                    P12 = 1; 
                    P13 = 0;
                    break;
                case RIGHT:  // 个位
                    P12 = 0;
                    P13 = 1;  
                    break;
            }
            P0 = disp[Num];  // 段选输出
            break;
    }
    delay(100);  // 短暂延时，增强显示稳定性
}

// 检测停止按钮按下
bit is_button_stop_pressed() {
    if (button_stop == 0) {  // 检测到低电平
        delay(1000);    // 消抖（约10ms）
        if (button_stop == 0) {  // 确认按下
            while(button_stop == 0);  // 等待释放
            delay(1000);         // 释放消抖
            return 1;
        }
    }
    return 0;
}
// 检测减速按钮按下
bit is_button_slow_pressed() {
    if (button_slow == 0) {  // 检测到低电平
        delay(1000);    // 消抖（约10ms）
        if (button_slow == 0) {  // 确认按下
            while(button_slow == 0);  // 等待释放
            delay(1000);         // 释放消抖
            return 1;
        }
    }
    return 0;
}

// 更新南北向红绿灯状态
void update_sn_state() {
    // 状态切换逻辑：红灯→绿灯→黄灯→红灯...
    if (sn_tens_place == 0 && sn_units_place == 0) {  // 倒计时结束
        current_sn_state = (current_sn_state + 1) % 3;  // 循环切换
        // 根据新状态重置倒计时
        switch(current_sn_state) {
            case SN_RED:    // 红灯时长：20秒（20→19...0）
                sn_tens_place = 2;
                sn_units_place = 0;
                break;
            case SN_GREEN:  // 绿灯时长：25秒（25→24...0）
                sn_tens_place = 2;
                sn_units_place = 5;
                break;
            case SN_YELLOW: // 黄灯时长：5秒（5→4...0）
                sn_tens_place = 0;
                sn_units_place = 5;
                break;
        }
    }
    // 控制LED
    sn_red = (current_sn_state == SN_RED) ? 1 : 0;    
    sn_green = (current_sn_state == SN_GREEN) ? 1 : 0;
    sn_yellow = (current_sn_state == SN_YELLOW) ? 1 : 0;
}

// 更新东西向红绿灯状态
void update_ew_state() {
    // 状态切换逻辑：绿灯→黄灯→红灯→绿灯...
    if (ew_tens_place == 0 && ew_units_place == 0) {  // 倒计时结束
        current_ew_state = (current_ew_state + 1) % 3;
        // 根据新状态重置倒计时
        switch(current_ew_state) {
            case EW_GREEN:  // 绿灯时长：20秒
                ew_tens_place = 2;
                ew_units_place = 0;
                break;
            case EW_YELLOW: // 黄灯时长：5秒
                ew_tens_place = 0;
                ew_units_place = 5;
                break;
            case EW_RED:    // 红灯时长：25秒
                ew_tens_place = 2;
                ew_units_place = 5;
                break;
        }
    }
    // 控制LED
    ew_red = (current_ew_state == EW_RED) ? 1 : 0;
    ew_green = (current_ew_state == EW_GREEN) ? 1 : 0;
    ew_yellow = (current_ew_state == EW_YELLOW) ? 1 : 0;
}

// 更新按钮状态
void update_button_state() {
    switch(current_button_state) {
        case IDLE:
            if (is_button_stop_pressed()) {
                current_button_state = STOP;  // 按下停止键
            } else if (is_button_slow_pressed()) {
                current_button_state = SLOW;  // 按下慢速键
            }
            break;
        case STOP:
            // 停止状态：全红灯，数码管显示0
            sn_red = 1; sn_green = 0; sn_yellow = 0;
            ew_red = 1; ew_green = 0; ew_yellow = 0;
            update_segment_disp(EW, LEFT, 0);
            update_segment_disp(EW, RIGHT, 0);
            update_segment_disp(SN, LEFT, 0);
            update_segment_disp(SN, RIGHT, 0);
            if (is_button_stop_pressed()) {
                current_button_state = IDLE;  // 再次按下恢复
            }
            break;
        case SLOW:
            // 慢速状态：全黄灯，数码管显示0
            sn_red = 0; sn_green = 0; sn_yellow = 1;
            ew_red = 0; ew_green = 0; ew_yellow = 1;
			update_segment_disp(EW, LEFT, 0);
            update_segment_disp(EW, RIGHT, 0);
            update_segment_disp(SN, LEFT, 0);
            update_segment_disp(SN, RIGHT, 0);
            if (is_button_slow_pressed()) {
                current_button_state = IDLE;  // 再次按下恢复
            }
            break;
    }
}

// 主函数
void main() {
    state_init();
    Timer0_init();
    while(1) {
        update_button_state();
        if (current_button_state == IDLE) {  // 正常运行
            // 刷新数码管显示
            update_segment_disp(EW, LEFT, ew_tens_place);   // 东西向十位
            update_segment_disp(EW, RIGHT, ew_units_place); // 东西向个位
            update_segment_disp(SN, LEFT, sn_tens_place);   // 南北向十位
            update_segment_disp(SN, RIGHT, sn_units_place); // 南北向个位
            // 更新红绿灯状态
            update_sn_state();
            update_ew_state();
        }
    }
}

// 定时器0中断服务函数（1ms触发一次）
void Timer0_Routine() interrupt 1 {
    static int sn_count = 0, ew_count = 0;  
    TL0 = 0x18;  // 重装载初值
    TH0 = 0xfc;

    sn_count++;  // 南北向倒计时计数
    ew_count++;  // 东西向倒计时计数

    // 每1秒处理一次进位（1000ms）
    handle_digit_carry(sn_count, &sn_units_place, &sn_tens_place);
    handle_digit_carry(ew_count, &ew_units_place, &ew_tens_place);

    // 重置计数
    if (sn_count >= 1000) sn_count = 0;
    if (ew_count >= 1000) ew_count = 0;
}