#include "mbed.h"
#include "gw_fwu.h"
#include "gw_fwu_test.h"

InterruptIn button1(BTN0);
InterruptIn button2(BTN1);

bool is_btn1_pressed = false, is_btn2_pressed = false;
void btn1_press(void)
{
    if(!is_btn1_pressed)
    {
        is_btn1_pressed = true;

        switch(this_case)
        {
            case 12:
                m_FwuMgr->ready_checkout(FWU_READY_PREPARE_2);
                break;
            case 13:
                m_FwuMgr->ready_checkout(FWU_READY_PREPARE_3);
                break;
            default:
                m_FwuMgr->ready_checkout(FWU_READY_PREPARE_1);
                break;
        }
    }
}

void btn1_release(void)
{
    if(is_btn1_pressed)
    {
        is_btn1_pressed = false;
    }
}

void btn2_press(void)
{
    if(!is_btn2_pressed)
    {
        is_btn2_pressed = true;
        m_FwuMgr->evt_push(NULL, FW_UPDATE_EVENT_NEXT_TEST, NULL);
    }
}

void btn2_release(void)
{
    if(is_btn2_pressed)
    {
        is_btn2_pressed = false;
    }
}

void gw_test_button_init(void)
{
    button1.fall(&btn1_press);
    button1.rise(&btn1_release);
    button2.fall(&btn2_press);
    button2.rise(&btn2_release);
}
