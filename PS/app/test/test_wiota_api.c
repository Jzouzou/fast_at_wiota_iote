
/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */
#include <rtthread.h>
#include "uc_wiota_api.h"
#include "test_wiota_api.h"
#ifdef _STATISTICS_DEBUG_
#include "ps_sys.h"
#endif
#include "uc_string_lib.h"


extern void cce_check_fetch();
extern void l1_sleep_test(void);
extern void l1_set_alarm_test(u32_t sec);


void* testTaskHandle = NULL;

const unsigned int USER_ID_LIST[7] = {0x4c00ccdb,0xc11cc34c,0x488c558a,0xabe44fcb,0x1c1138b8,0xdba09b6f,0x9768c6cc};
const unsigned int USER_DCXO_LIST[7] = {0x25000,0x29000,0x30000,0x27000,0x14000,0x30000,0x2E000};
#define USER_IDX 3
#define TEST_DATA_MAX_SIZE 1024

boolean is_scaning_freq = FALSE;
boolean is_need_reset = FALSE;
boolean is_need_scaning_freq = FALSE;
u8_t new_freq_idx = 0;

void test_send_callback(uc_send_back_p sendResult)
{
    rt_kprintf("app send result %d\n",sendResult->result);
}


void test_recv_callback(uc_recv_back_p recvData)
{
    u8_t freq_idx;
    u8_t freq_num;
    uc_freq_scan_result_t* freq_list;
            
    rt_kprintf("app recv result %d type %d len %d addr 0x%x\n",
            recvData->result,recvData->type,recvData->data_len,recvData->data);
    
    switch (recvData->type) {
        case UC_RECV_SCAN_FREQ:
            freq_idx = *(recvData->data);
            new_freq_idx = freq_idx;
            freq_num = recvData->data_len / sizeof(uc_freq_scan_result_t);
            freq_list = (uc_freq_scan_result_t*)(recvData->data);
            rt_kprintf("scan num %d\n",freq_num);
            for (u8_t i = 0; i < freq_num; i++) {
                rt_kprintf("freq %d is_sync %d rssi %d snr %d\n",freq_list[i].freq_idx,freq_list[i].is_synced,
                                                                 freq_list[i].rssi,freq_list[i].snr);
            }
            is_scaning_freq = FALSE;
            is_need_reset = TRUE;
            break;

        case UC_RECV_SYNC_LOST:
            rt_kprintf("lost sync, re scan\n");
            is_need_scaning_freq = TRUE;
            break;
            
        case UC_RECV_MSG:
        case UC_RECV_BC:
        case UC_RECV_OTA:
        
            break;
            
        default:
            rt_kprintf("Type ERROR!!!!!!!!!!!\n");
            break;
    }
            
    if (UC_OP_SUCC == recvData->result) { rt_free(recvData->data); }
}


void test_recv_req_callback(uc_recv_back_p recvData)
{
    rt_kprintf("app recv req result %d type %d len %d addr 0x%x\n",
            recvData->result,recvData->type,recvData->data_len,recvData->data);
    
    if (UC_OP_SUCC == recvData->result) { rt_free(recvData->data); }
}


#if 1
void app_test_main_task(void* pPara) 
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;
//    unsigned char testData[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
    unsigned char* testData = rt_malloc(TEST_DATA_MAX_SIZE);
    unsigned int i;
    unsigned int user_id[2] = {USER_ID_LIST[USER_IDX],0x0};
    
    for (i = 0; i < TEST_DATA_MAX_SIZE; i++) {
        testData[i] = i & 0xFF;
    }
    
    //unsigned char user_id_len = 0;
//    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};   
#ifdef _STATISTICS_DEBUG_
    StatisticalInfo_T local_statistics = {0};
#endif
//    {
//        .id_len = 1,
//        .pn_num = 1,
//        .symbol_length = 1,
//        .dlul_ratio = 0,
//        .btvalue = 1,
//        .group_number = 0,
//        .systemid = 0x11223344,
//        .subsystemid = 0x21456981,
//        .na = {0},
//    };
    
    // first of all, init wiota
    uc_wiota_init();

    // test! whole config 
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d %d %d 0x%x 0x%x\n",
                wiota_config.id_len,wiota_config.pn_num,wiota_config.symbol_length,wiota_config.dlul_ratio,
                wiota_config.btvalue,wiota_config.group_number,wiota_config.systemid,wiota_config.subsystemid); 
//    wiota_config.pn_num = 2;  // change config then set
//    wiota_config.spectrumIdx = 7;
//    uc_wiota_set_system_config(&wiota_config);
    
    // test! set dcxo
    uc_wiota_set_dcxo(USER_DCXO_LIST[USER_IDX]);

    // test! freq test
//    uc_wiota_set_freq_info(160);    // 470+0.2*100=490
    uc_wiota_set_freq_info(135);    // 470+0.2*100=490
//    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
//    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());
    
    // test! user id 
//    TRACE_PRINTF("set user id %s\n", user_id);
    uc_wiota_set_userid(user_id,4);
//    memset(user_id,0,4);
//    uc_wiota_get_userid(user_id, &user_id_len);
//    TRACE_PRINTF("get user id %s\n", user_id);
//    TRACE_PRINTF("len = %d\n",user_id_len);

//    uc_wiota_set_activetime(3);

    // after config set, run wiota !
    uc_wiota_run();

#ifdef _FPGA_    
    cce_check_fetch();
//    rt_thread_mdelay(100);
    rt_kprintf("app test main start loop\n");
#endif

    // sync connect
//    rt_kprintf("wiota state %d\n",uc_wiota_get_state());
    uc_wiota_connect();
    rt_thread_mdelay(10);
//    rt_kprintf("wiota state %d\n",uc_wiota_get_state());    

    // test! register recv data callback, afer upload, will continue recv ap's data or broadcast
    uc_wiota_register_recv_data(test_recv_callback);

//    unsigned char scan_data[10] = {100,100,100,100,100,100,100,100,100,100};
//    unsigned char scan_data[17] = {90,100,105,120,125,130,131,133,134,135,135,135,136,137,138,139,150};
//    unsigned char scan_data[20] = {0,50,70,115,129,132,133,111,136,138,140,105,142,106,147,150,152,107,135,104};
//    unsigned char scan_data[20] = {94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115};
    unsigned char scan_data[20] = {5,15,25,35,45,55,65,75,85,95,105,115,125,135,145,155,165,175,185,195};
//    unsigned char data[64] = {0};
    unsigned short scan_len = 20;

//    for (int i = 0; i < 16; i++) {
//        scan_data[i*4] = 125;
//        scan_data[i*4+1] = 130;
//        scan_data[i*4+2] = 135;
//        scan_data[i*4+3] = 140;
//    }

//    is_scaning_freq = TRUE;
//    uc_wiota_scan_freq(scan_data, scan_len, 0, test_recv_callback, NULL);
//    uc_wiota_scan_freq(scan_data, scan_len, 0, NULL, &recv_result_t);
//    uc_wiota_scan_freq(NULL, 0, 0, NULL, &recv_result_t);
//    if (UC_OP_SUCC == recv_result_t.result) { rt_free(recv_result_t.data); }
//    uc_wiota_scan_freq(NULL, 0, 0, test_recv_callback, NULL);

    while (1)
    {
//        rt_kprintf("%s line %d\n", __FUNCTION__, __LINE__);

        rt_memory_info(&total,&used,&max_used);
        rt_kprintf("total %d used %d maxused %d \n",total,used,max_used);
        
        if (is_need_reset) {
            uc_wiota_exit();
            uc_wiota_init();
            uc_wiota_set_dcxo(USER_DCXO_LIST[USER_IDX]);
            uc_wiota_set_freq_info(new_freq_idx);   
            uc_wiota_run();
            uc_wiota_register_recv_data(test_recv_callback);
            uc_wiota_connect();
            rt_thread_mdelay(1000);    
            is_need_reset = FALSE;
        }
        
        if (is_need_scaning_freq) {
            is_need_scaning_freq = FALSE;
            is_scaning_freq = TRUE;
//            uc_wiota_scan_freq(NULL, 0, 0, test_recv_callback, NULL);
            uc_wiota_scan_freq(scan_data, scan_len, 0, test_recv_callback, NULL);    
        }
        
        if (is_scaning_freq || UC_STATUS_SYNC != uc_wiota_get_state()) { 
            rt_thread_mdelay(10000);
            continue; 
        }
#ifdef _STATISTICS_DEBUG_
        state_get_statistics_info(&local_statistics);  
//        memcpy(testData,&local_statistics,sizeof(StatisticalInfo_T));
//        l1_print_data_u8(testData,20);
//        uc_wiota_send_data(testData, 20, 2000, test_send_callback);
//        uc_wiota_send_data(testData, 1024, 55000, NULL);
//        uc_wiota_send_data(testData, 30, 3000, test_send_callback);
        
//        uc_wiota_register_recv_data(test_recv_callback);
        // test! send data periodic
//        uc_wiota_send_data(testData, 20, 2000, test_send_callback);
#endif
        // test! recv data without callback
//        uc_wiota_recv_data(&recv_result_t, 10000, NULL);
//        rt_kprintf("data recv result %d type %d len %d data 0x%x\n",recv_result_t.result,recv_result_t.type,
//                                                                    recv_result_t.data_len,recv_result_t.data);
//        uc_free(recv_result_t.data);
        // test! recv data with callback
//        uc_wiota_recv_data(NULL, 9000, test_recv_req_callback);
        
//        uc_wiota_exit();
//        l1_set_alarm_test(121);
//        rt_thread_mdelay(100);
//        l1_sleep_test();
        
        // test! connect and disconnect
//        rt_thread_mdelay(10000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//        uc_wiota_disconnect();
//        rt_thread_mdelay(1000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//        rt_thread_mdelay(1000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
//        uc_wiota_connect();
//        rt_thread_mdelay(1000);
//        rt_kprintf("wiota state %d\n",uc_wiota_get_state());
        
        // test! wiota iote task exit
//        rt_thread_mdelay(3000);
//        uc_wiota_exit();
//        rt_thread_mdelay(3000);
//        uc_wiota_init();
//        // test! freq test
//        uc_wiota_set_dcxo(0xB000);
//        uc_wiota_set_freq_info(100);    // 470+0.2*100=490
//        rt_thread_mdelay(1000);
//        uc_wiota_run();
//        rt_thread_mdelay(1000);
//        uc_wiota_connect();
//        rt_thread_mdelay(1000);
      
        rt_thread_mdelay(10000);
    }
    
    rt_free(testData);
    
    return;
}

#else
void app_test_main_task(void* pPara) 
{
    s8_t temp = 0;
    u8_t subframe = 0;
    
    while (1)
    {
        if (UC_STATUS_SYNC == uc_wiota_get_state()) {
            temp = l1_adc_temperature_read();
            subframe = l1_get_subframeCounter();
            rt_kprintf("test temp read %d subf %d\n",temp,subframe);
        }
        rt_thread_mdelay(3);
    }
    return;
}   
#endif


int uc_thread_create_test(void ** thread, \
            char *name, void (*entry)(void *parameter), \
            void *parameter, unsigned int  stack_size, \
            unsigned char   priority, \
            unsigned int  tick)
{
    * thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack) { return 1; }
    if (RT_EOK != rt_thread_init(* thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}
 
int uc_thread_start_test(void * thread)
{
    return rt_thread_startup((rt_thread_t)thread);
}

void app_task_init(void) 
{
    uc_thread_create_test(&testTaskHandle,"test1", app_test_main_task, NULL, 256, 3, 3);

    uc_thread_start_test(testTaskHandle);
}


