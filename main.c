#include<reg51.h>

// ���Ŷ���
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

// ö�ٶ���
enum Direction {EW, SN};
enum Location {LEFT, RIGHT};
enum SnState {SN_RED, SN_GREEN, SN_YELLOW};  // �ϱ���״̬
enum EwState {EW_GREEN, EW_YELLOW, EW_RED};  // ������״̬
enum ButtonState {IDLE, STOP, SLOW};

// ȫ�ֱ���
bit stop_state = 0;
bit slow_state = 0;
int sn_units_place = 5;  // �ϱ����λ
int sn_tens_place = 2;   // �ϱ���ʮλ
int ew_units_place = 0;  // �������λ
int ew_tens_place = 2;   // ������ʮλ
unsigned char code disp[] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88};  // �����0-9��

// ȫ��״̬����
enum SnState current_sn_state;
enum EwState current_ew_state;
enum ButtonState current_button_state;

// ��������
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

// ״̬��ʼ��
void state_init() {
    current_sn_state = SN_RED;      // ��ʼ���ϱ����
    current_ew_state = EW_GREEN;    // ��ʼ�������̵�
    current_button_state = IDLE;    // ��ť��ʼ״̬
}

// ��ʱ��0��ʼ����1ms�жϣ�
void Timer0_init() {
    TMOD &= 0xf0;
    TMOD |= 0x01;  // ģʽ1��16λ��ʱ��
    TL0 = 0x18;    // ��ֵװ�أ�1ms@12MHz��
    TH0 = 0xfc;
    TF0 = 0;       // �����־
    TR0 = 1;       // ������ʱ��
    ET0 = 1;       // �����ж�
    EA = 1;        // ���ж�����
    PT0 = 0;       // �����ȼ�
}

// ��ʱ����
void delay(int x) {
    while(x--);
}

// �������ֽ�λ
void handle_digit_carry(int count, int *units_place, int *tens_place) {
    if (count >= 1000) {  
        *units_place -= 1;
        if (*units_place < 0) {
            *units_place = 9;  // ��λ��9
            *tens_place -= 1;  // ʮλ��1
            if (*tens_place < 0) {
                *tens_place = 0;  // ��ֹʮλΪ��
            }
        }
    }
}

// �����������ʾ
void update_segment_disp(enum Direction dir, enum Location location, unsigned char Num) {
    // �������ȹر����ж�ѡ��
    P0 = 0xff;
    P2 = 0xff;

    switch(dir) {
        case EW:  // �����������
            switch(location) {
                case LEFT:   // ʮλ
                    P10 = 1;  
                    P11 = 0;
                    break;
                case RIGHT:  // ��λ
                    P10 = 0;
                    P11 = 1;  
                    break;
            }
            P2 = disp[Num];  // ��ѡ���
            break;
        case SN:  // �ϱ��������
            switch(location) {
                case LEFT:   // ʮλ
                    P12 = 1; 
                    P13 = 0;
                    break;
                case RIGHT:  // ��λ
                    P12 = 0;
                    P13 = 1;  
                    break;
            }
            P0 = disp[Num];  // ��ѡ���
            break;
    }
    delay(100);  // ������ʱ����ǿ��ʾ�ȶ���
}

// ���ֹͣ��ť����
bit is_button_stop_pressed() {
    if (button_stop == 0) {  // ��⵽�͵�ƽ
        delay(1000);    // ������Լ10ms��
        if (button_stop == 0) {  // ȷ�ϰ���
            while(button_stop == 0);  // �ȴ��ͷ�
            delay(1000);         // �ͷ�����
            return 1;
        }
    }
    return 0;
}
// �����ٰ�ť����
bit is_button_slow_pressed() {
    if (button_slow == 0) {  // ��⵽�͵�ƽ
        delay(1000);    // ������Լ10ms��
        if (button_slow == 0) {  // ȷ�ϰ���
            while(button_slow == 0);  // �ȴ��ͷ�
            delay(1000);         // �ͷ�����
            return 1;
        }
    }
    return 0;
}

// �����ϱ�����̵�״̬
void update_sn_state() {
    // ״̬�л��߼�����ơ��̵ơ��Ƶơ����...
    if (sn_tens_place == 0 && sn_units_place == 0) {  // ����ʱ����
        current_sn_state = (current_sn_state + 1) % 3;  // ѭ���л�
        // ������״̬���õ���ʱ
        switch(current_sn_state) {
            case SN_RED:    // ���ʱ����20�루20��19...0��
                sn_tens_place = 2;
                sn_units_place = 0;
                break;
            case SN_GREEN:  // �̵�ʱ����25�루25��24...0��
                sn_tens_place = 2;
                sn_units_place = 5;
                break;
            case SN_YELLOW: // �Ƶ�ʱ����5�루5��4...0��
                sn_tens_place = 0;
                sn_units_place = 5;
                break;
        }
    }
    // ����LED
    sn_red = (current_sn_state == SN_RED) ? 1 : 0;    
    sn_green = (current_sn_state == SN_GREEN) ? 1 : 0;
    sn_yellow = (current_sn_state == SN_YELLOW) ? 1 : 0;
}

// ���¶�������̵�״̬
void update_ew_state() {
    // ״̬�л��߼����̵ơ��Ƶơ���ơ��̵�...
    if (ew_tens_place == 0 && ew_units_place == 0) {  // ����ʱ����
        current_ew_state = (current_ew_state + 1) % 3;
        // ������״̬���õ���ʱ
        switch(current_ew_state) {
            case EW_GREEN:  // �̵�ʱ����20��
                ew_tens_place = 2;
                ew_units_place = 0;
                break;
            case EW_YELLOW: // �Ƶ�ʱ����5��
                ew_tens_place = 0;
                ew_units_place = 5;
                break;
            case EW_RED:    // ���ʱ����25��
                ew_tens_place = 2;
                ew_units_place = 5;
                break;
        }
    }
    // ����LED
    ew_red = (current_ew_state == EW_RED) ? 1 : 0;
    ew_green = (current_ew_state == EW_GREEN) ? 1 : 0;
    ew_yellow = (current_ew_state == EW_YELLOW) ? 1 : 0;
}

// ���°�ť״̬
void update_button_state() {
    switch(current_button_state) {
        case IDLE:
            if (is_button_stop_pressed()) {
                current_button_state = STOP;  // ����ֹͣ��
            } else if (is_button_slow_pressed()) {
                current_button_state = SLOW;  // �������ټ�
            }
            break;
        case STOP:
            // ֹͣ״̬��ȫ��ƣ��������ʾ0
            sn_red = 1; sn_green = 0; sn_yellow = 0;
            ew_red = 1; ew_green = 0; ew_yellow = 0;
            update_segment_disp(EW, LEFT, 0);
            update_segment_disp(EW, RIGHT, 0);
            update_segment_disp(SN, LEFT, 0);
            update_segment_disp(SN, RIGHT, 0);
            if (is_button_stop_pressed()) {
                current_button_state = IDLE;  // �ٴΰ��»ָ�
            }
            break;
        case SLOW:
            // ����״̬��ȫ�Ƶƣ��������ʾ0
            sn_red = 0; sn_green = 0; sn_yellow = 1;
            ew_red = 0; ew_green = 0; ew_yellow = 1;
			update_segment_disp(EW, LEFT, 0);
            update_segment_disp(EW, RIGHT, 0);
            update_segment_disp(SN, LEFT, 0);
            update_segment_disp(SN, RIGHT, 0);
            if (is_button_slow_pressed()) {
                current_button_state = IDLE;  // �ٴΰ��»ָ�
            }
            break;
    }
}

// ������
void main() {
    state_init();
    Timer0_init();
    while(1) {
        update_button_state();
        if (current_button_state == IDLE) {  // ��������
            // ˢ���������ʾ
            update_segment_disp(EW, LEFT, ew_tens_place);   // ������ʮλ
            update_segment_disp(EW, RIGHT, ew_units_place); // �������λ
            update_segment_disp(SN, LEFT, sn_tens_place);   // �ϱ���ʮλ
            update_segment_disp(SN, RIGHT, sn_units_place); // �ϱ����λ
            // ���º��̵�״̬
            update_sn_state();
            update_ew_state();
        }
    }
}

// ��ʱ��0�жϷ�������1ms����һ�Σ�
void Timer0_Routine() interrupt 1 {
    static int sn_count = 0, ew_count = 0;  
    TL0 = 0x18;  // ��װ�س�ֵ
    TH0 = 0xfc;

    sn_count++;  // �ϱ��򵹼�ʱ����
    ew_count++;  // �����򵹼�ʱ����

    // ÿ1�봦��һ�ν�λ��1000ms��
    handle_digit_carry(sn_count, &sn_units_place, &sn_tens_place);
    handle_digit_carry(ew_count, &ew_units_place, &ew_tens_place);

    // ���ü���
    if (sn_count >= 1000) sn_count = 0;
    if (ew_count >= 1000) ew_count = 0;
}