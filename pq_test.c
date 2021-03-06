/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== uartecho.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

#include <ti/drivers/ADC.h>
#include <ti/drivers/ADCBuf.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/PWM.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Watchdog.h>
#define __MSP432P401R__
#include <ti/devices/msp432p4xx/driverlib/uart.h>
#include <ti/drivers/uart/UARTMSP432.h>

#include "INA226.h"
#include "PQ9_bus_engine.h"

/*
 *  ============ Test functions ============
 */

//To Do

// WDG timing

// Check that all tests are performed

// Test LTC id
// Test LTC meas
// Test WDG on all boards
// Test TMP on all boards
// Test INA meas on all boards

// TMP doesn't handle negative values


#define SUBS_TESTS -3
// EPS master -4
// RED sniffer -3
// OBC sniffer -2
// RED -1
// RED 0, MASTER
// OBC 1, MASTER
// ANT 2
// ADCS 3
// EPS 4
// EPS with BATT 5
// EPS with SOL  6
// EPS complete  7

#if (SUBS_TESTS == 0 || SUBS_TESTS == -1 || SUBS_TESTS == -3)
#include "RED_Board.h"
#elif (SUBS_TESTS == 1 || SUBS_TESTS == -2)
#include "OBC_Board.h"
#elif (SUBS_TESTS == 2)
#include "ANT_Board.h"
#elif (SUBS_TESTS == 3)
#include "ADCS_Board.h"
#elif (SUBS_TESTS >= 4 || SUBS_TESTS == -4)
#include "EPS_Board.h"
#endif

/*
 *  ------------ UART functions ------------
 *  Each subsystem has the same uart and rs-485 peripherals, so no need for change.
 */

UART_Handle uart_dbg_bus;
UART_Handle uart_pq9_bus;

void temp(UART_Handle handle, void *buf, size_t count) {

}

void rs_test() {


  UART_Params uartParams;

  GPIO_write(PQ9_EN, 0);

  /* Create a UART with data processing off. */
  UART_Params_init(&uartParams);
  uartParams.writeMode = UART_MODE_BLOCKING;
  uartParams.writeDataMode = UART_DATA_BINARY;
  uartParams.readMode = UART_MODE_CALLBACK;
  uartParams.readDataMode = UART_DATA_BINARY;
  uartParams.readReturnMode = UART_RETURN_FULL;
  uartParams.readEcho = UART_ECHO_OFF;
  uartParams.baudRate = 500000;
  uartParams.readCallback = &temp;

  uart_pq9_bus = UART_open(PQ9, &uartParams);
  sleep(1);

  UARTMSP432_HWAttrsV1 const *hwAttrs = uart_pq9_bus->hwAttrs;
  UART_setDormant(hwAttrs->baseAddr);

}

uint8_t get_subs_addr() {
  #if (SUBS_TESTS == 1 || SUBS_TESTS == 0)
      return 0x01; //OBC
  #else
      return 0x07; //DBG
  #endif
}

extern uint8_t pq_rx_buf[300];
extern uint16_t pq_rx_count, pq_size;
extern bool pq_rx_flag;

char msg_uart[500] = "Testing RS tx\n";

char buf_rs[100];
uint16_t buf_cnt = 0;

void rs_tx_addr_test() {

  UART_Params uartParams;

  GPIO_write(PQ9_EN, 0);

  sprintf(msg_uart, "Hello DBG, PQ9 master tester\n");
  UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

  char resp[10];
  int32_t res = 0;
  uint8_t dest_addr = 1;
  do {
    sleep(1);
    char data[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    pq9_pkt test_pkt;
    test_pkt.src_id = 1;
    test_pkt.size = 4;
    test_pkt.msg = data;
    char pq_tx_buf[20];
    uint16_t pq_tx_size;

    res = UART_read(uart_dbg_bus, resp, 1);
    if(res > 0 && resp[0] == 'a') {
        sprintf(msg_uart, "Sending packet addr 0x55\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        test_pkt.dest_id = 0x55;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 'z') {
        sprintf(msg_uart, "Sending packet Wr addr 0x55\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        test_pkt.dest_id = 0x55;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        GPIO_write(PQ9_EN, 1);
        UART_write(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 's') {
        sprintf(msg_uart, "Sending packet addr 0x75\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        test_pkt.dest_id = 0x75;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 'w') {
        sprintf(msg_uart, "Error packet addr 0x55\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        test_pkt.dest_id = 0x55;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        //giving wrong size
        pq_tx_buf[1] = 2;
        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 'e') {
        sprintf(msg_uart, "Error packet addr 0x55\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        test_pkt.dest_id = 0x55;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        //giving wrong size
        pq_tx_buf[1] = 6;
        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 'p') {
        sprintf(msg_uart, "Sending query addr %d\n", dest_addr);
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        uint8_t req[] = { 17, 1};

        test_pkt.dest_id = dest_addr;
        test_pkt.src_id = 0x01;
        test_pkt.size = 2;
        test_pkt.msg = req;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        test_pkt.msg = data;

        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 'h') {
        sprintf(msg_uart, "Sending query addr %d\n", dest_addr);
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        uint8_t req[] = { 3, 1};

        test_pkt.dest_id = dest_addr;
        test_pkt.src_id = 0x01;
        test_pkt.size = 2;
        test_pkt.msg = req;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        test_pkt.msg = data;

        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);
    } else if(res > 0 && resp[0] == 'i') {
        sprintf(msg_uart, "Sending query addr %d\n", dest_addr);
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        uint8_t req[] = { 1, 1, 4, 1};

        test_pkt.dest_id = dest_addr;
        test_pkt.src_id = 0x01;
        test_pkt.size = 4;
        test_pkt.msg = req;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        test_pkt.msg = data;

        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);
    } else if(res > 0 && resp[0] == 'o') {
        sprintf(msg_uart, "Sending query addr %d\n", dest_addr);
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        uint8_t req[] = { 1, 1, 4, 0};

        test_pkt.dest_id = dest_addr;
        test_pkt.src_id = 0x01;
        test_pkt.size = 4;
        test_pkt.msg = req;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        test_pkt.msg = data;

        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);
    } else if(res > 0 && resp[0] == 'm') {
        sprintf(msg_uart, "Change address OBC: 1 EPS: 2 DBG: 7\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        res = 0;
        while(res == 0) {
            res = UART_read(uart_dbg_bus, resp, 1);

            if(res > 0 && resp[0] == '1') {
                dest_addr = 1;
                sprintf(msg_uart, "OBC selected\n");
                UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));
            } else if(res > 0 && resp[0] == '2') {
                            dest_addr = 2;
                            sprintf(msg_uart, "EPS selected\n");
                            UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));
            } else if(res > 0 && resp[0] == '7') {
                dest_addr = 7;
                sprintf(msg_uart, "DBG selected\n");
                UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));
            }
        }
        sprintf(msg_uart, "Change address finished\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

    } else if(res > 0 && resp[0] == 'g') {
        sprintf(msg_uart, "Sending query addr GET %d\n", dest_addr);
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        uint8_t req[] = { 1, 2, 42};

        test_pkt.dest_id = dest_addr;
        test_pkt.src_id = 0x01;
        test_pkt.size = 3;
        test_pkt.msg = req;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        test_pkt.msg = data;

        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);

    } else if(res > 0 && resp[0] == 't') {
        sprintf(msg_uart, "Sending query addr SET %d\n", dest_addr);
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

        uint8_t req[] = { 1, 1, 42, 0xBA, 0xAD,0xF0,0x0D};

        test_pkt.dest_id = dest_addr;
        test_pkt.src_id = 0x01;
        test_pkt.size = 7;
        test_pkt.msg = req;
        pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);
        test_pkt.msg = data;

        GPIO_write(PQ9_EN, 1);
        UART_writePolling(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);
    }
    resp[0] = 'o';

     UARTMSP432_Object *object = uart_pq9_bus->object;

     while(RingBuf_get(&object->ringBuffer, &buf_rs[buf_cnt]) != -1) {
       buf_cnt++;
     }
     while(buf_cnt > 0) {
       sprintf(msg_uart,"%02x ", buf_rs[buf_cnt]);
       UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));
       buf_cnt--;
     }

    if(pq_rx_flag) {

      uint8_t data[100];
      pq9_pkt rx_pq_pkt;
      rx_pq_pkt.msg = data;

      pq_rx_flag = 0;

      char *pos = msg_uart;

      sprintf(msg_uart, "msg: ");
                                pos += strlen(msg_uart);
                                for(uint8_t i = 2; i < pq_rx_count; i++) {
                                    sprintf(pos, ",%02x ",pq_rx_buf[i]);
                                    pos += 4;
                                }
                                sprintf(pos, "\n");
                                UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

      bool unpack_res = unpack_PQ9_BUS(pq_rx_buf, pq_rx_count, &rx_pq_pkt);

      *pos = msg_uart;
                          sprintf(msg_uart, "PQ msg: %d,%d %d, %d: %02x,%02x : ", rx_pq_pkt.src_id, rx_pq_pkt.dest_id, rx_pq_pkt.size, res, rx_pq_pkt.msg[0], rx_pq_pkt.msg[1]);
                          pos += strlen(msg_uart);
                          for(uint8_t i = 2; i < rx_pq_pkt.size; i++) {
                              sprintf(pos, ",%02x ",rx_pq_pkt.msg[i]);
                              pos += 4;
                          }
                          sprintf(pos, "\n");
                          UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

      if(unpack_res == false && data[0] == 17 && data[1] == 2) {

        sprintf(msg_uart, "Got RESPONSE\n");
        UART_write(uart_dbg_bus, msg_uart, strlen(msg_uart));

      }
      sleep(1);
    }



  } while(1);
}

uint32_t test_tx_pkt_cnt = 0;
uint32_t test_rx_pkt_cnt = 0;

void rs_tx_stress_test() {

  UART_Params uartParams;

  GPIO_write(PQ9_EN, 0);

  char msg[50] = "Testing RS stress tx\n";
  UART_write(uart_dbg_bus, msg, strlen(msg));

  char resp[10];
  int32_t res = 0;

    //UART_write(uart_dbg_bus, "Hello DBG\n", 10);
  sleep(1);
  uint8_t req[] = { 17, 1};

  pq9_pkt test_pkt;
  test_pkt.src_id = 1;
  test_pkt.size = 2;
  test_pkt.msg = req;
  char pq_tx_buf[20];
  uint16_t pq_tx_size;

  sprintf(msg, "Sending query addr 0x55\n");
  UART_write(uart_dbg_bus, msg, strlen(msg));


  test_pkt.dest_id = 0x55;
  test_pkt.size = 2;
  test_pkt.msg = req;
  pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);

  uint8_t data[100];
  pq9_pkt rx_pq_pkt;
  rx_pq_pkt.msg = data;

  uint16_t msg_cnt = 0;

  do {
        GPIO_write(PQ9_EN, 1);
        UART_write(uart_pq9_bus, pq_tx_buf, pq_tx_size);
        GPIO_write(PQ9_EN, 0);
        test_tx_pkt_cnt++;


          sleep(5);

        if(pq_rx_flag) {
          pq_rx_flag = 0;
          //sprintf(msg, "Rx msg: %d,%d: %x %x %x %x\n",pq_rx_count, pq_size, pq_rx_buf[0], pq_rx_buf[1], pq_rx_buf[2], pq_rx_buf[3]);
          //UART_write(uart_dbg_bus, msg, strlen(msg));

          bool unpack_res = unpack_PQ9_BUS(pq_rx_buf, pq_rx_count, &rx_pq_pkt);

          //sprintf(msg, "PQ Rx msg: %d,%d: %d %d %x\n",res, rx_pq_pkt.size, rx_pq_pkt.src_id, rx_pq_pkt.dest_id, rx_pq_pkt.msg[0]);
          //UART_write(uart_dbg_bus, msg, strlen(msg));

          if(unpack_res == false && data[0] == 17 && data[1] == 2) {

            //sprintf(msg, "Got RESPONSE\n");
            //UART_write(uart_dbg_bus, msg, strlen(msg));
            test_rx_pkt_cnt++;
          }
        }

        if(msg_cnt == 1000) {
            msg_cnt = 0;
            sprintf(msg, "Str %d, %d\n", test_tx_pkt_cnt, test_rx_pkt_cnt);
            UART_write(uart_dbg_bus, msg, strlen(msg));
        }
        msg_cnt++;

  } while(1);
}

void rs_rx_addr_test() {

      UART_Params uartParams;

      GPIO_write(PQ9_EN, 0);

      char msg[30] = "Testing RS rx\n";
      UART_write(uart_dbg_bus, msg, strlen(msg));

      int32_t res = 0;

      uint8_t data[100];
      pq9_pkt rx_pq_pkt;
      rx_pq_pkt.msg = data;

      pq9_pkt test_pkt;
      char resp[] = { 17, 2};

      test_pkt.src_id = 1;
      test_pkt.size = 2;
      test_pkt.msg = resp;
      char pq_tx_buf[50];
      uint16_t pq_tx_size;

      test_pkt.dest_id = 0x75;
      pack_PQ9_BUS(&test_pkt, pq_tx_buf, &pq_tx_size);

      do {

          if(pq_rx_flag) {
              pq_rx_flag = 0;
              char *pos = msg;
              sprintf(msg, "Rx msg: ");
              pos += strlen(msg);
              for(uint8_t i = 0; i < pq_rx_count; i++) {
                  sprintf(pos, ",%d ",pq_rx_buf[i]);
                  pos += strlen(msg);
              }
              sprintf(pos, "\n");
              UART_write(uart_dbg_bus, msg, strlen(msg));

              bool unpack_res = unpack_PQ9_BUS(pq_rx_buf, pq_rx_count, &rx_pq_pkt);

              //sprintf(msg, "PQ Rx msg: %d,%d: %d %d %x\n",res, rx_pq_pkt.size, rx_pq_pkt.src_id, rx_pq_pkt.dest_id, rx_pq_pkt.msg[0]);
              //UART_write(uart_dbg_bus, msg, strlen(msg));

              if(unpack_res == false && data[0] == 17 && data[1] == 1) {

                test_rx_pkt_cnt++;

                //sprintf(msg, "Got request, sending resp\n");
                //UART_write(uart_dbg_bus, msg, strlen(msg));

                GPIO_write(PQ9_EN, 1);
                UART_write(uart_pq9_bus, pq_tx_buf, pq_tx_size);
                GPIO_write(PQ9_EN, 0);
              }

          }

          usleep(100);
      } while(1);
}

#define HLDLC_START_FLAG        0x7E
#define HLDLC_CONTROL_FLAG      0x7D
#define HLDLC_STOP_FLAG         0x7C

void HLDLC_deframe(uint8_t *buf_in,
                              uint8_t *buf_out,
                              const uint16_t size_in,
                              uint16_t *size_out) {


    uint16_t cnt = 0;

    for(uint16_t i = 0; i < size_in; i++) {

        uint8_t c = buf_in[i];

        if(c == HLDLC_START_FLAG) {
            cnt = 0;
        } else if(c == HLDLC_STOP_FLAG) {
            *size_out = cnt;
            return ;
        } else if(c == HLDLC_CONTROL_FLAG) {
            i++;
            c = buf_in[i];

            if(c == 0x5E) {
              buf_out[cnt++] = 0x7E;
            } else if(c == 0x5D) {
              buf_out[cnt++] = 0x7D;
            } else if(c== 0x5C) {
              buf_out[cnt++] = 0x7C;
            } else {
              return ;
            }
        } else {
            buf_out[cnt++] = c;
        }

    }
    return ;
}



uint8_t buf_uart43[100];
uint8_t buf_uart_hldlc[100];
uint16_t buf_uart_cnt = 0;
uint16_t buf_uart_hldlc_cnt = 0;
uint8_t resp54[3];
uint8_t res;

bool ctrl_flag, strt_flag = false;

void pc_interface() {

  UARTMSP432_Object *object = uart_pq9_bus->object;

  char temp[10];

  uint8_t tx_count, tx_size, tx_buf[255];

  while(1) {

    // PQ9 -> PC
    while(RingBuf_get(&object->ringBuffer, &buf_rs[buf_cnt]) != -1) {
      buf_cnt++;
    }
    if(buf_cnt > 0) {
      //sprintf(temp,"%02x ", buf_rs[buf_cnt]);
      //UART_write(uart_dbg_bus, temp, strlen(temp));
      UART_write(uart_dbg_bus, buf_rs, buf_cnt);
      buf_cnt = 0;
    }

    // PC -> PQ9
    do {
      res = UART_read(uart_dbg_bus, resp54, 1);
      if(res > 0) {
        char msg[20];
        //sprintf(msg, "New byte %02x\n", resp54[0]);
        //UART_write(uart_dbg_bus, msg, strlen(msg));
        if(resp54[0] == HLDLC_START_FLAG) {
          strt_flag = true;

        } else if(resp54[0] == HLDLC_CONTROL_FLAG) {
          ctrl_flag = true;
        } else if(strt_flag) {
           strt_flag = false;
           tx_buf[0] = resp54[0];
           tx_count = 1;
        } else if(ctrl_flag) {
           ctrl_flag = false;
           if(resp54[0] == 0x5D) {
             tx_buf[tx_count] = 0x7D;
             tx_count++;
           } else if(resp54[0] == 0x5E) {
             tx_buf[tx_count] = 0x7E;
             tx_count++;
           }
        } else if(tx_count == 1) {
          tx_buf[tx_count] = resp54[0];
          tx_size = resp54[0] + 5;
          tx_count++;
        } else if(tx_count > 0 && tx_count < tx_size - 1) {
          tx_buf[tx_count] = resp54[0];
          tx_count++;
        } else if(tx_count > 0 && tx_count == tx_size - 1) {
          tx_buf[tx_count] = resp54[0];
          tx_count++;
          GPIO_write(PQ9_EN, 1);
          UART_writePolling(uart_pq9_bus, tx_buf, tx_count);
          GPIO_write(PQ9_EN, 0);
        }
      }
    } while(res > 0);

  }
}



void uart_test() {


      UART_Params uartParams;

      UART_Params_init(&uartParams);
      uartParams.writeMode = UART_MODE_BLOCKING;
      uartParams.writeDataMode = UART_DATA_BINARY;
      uartParams.readMode = UART_MODE_BLOCKING;
      uartParams.readDataMode = UART_DATA_BINARY;
      uartParams.readTimeout = 1000;
      uartParams.readReturnMode = UART_RETURN_FULL;
      uartParams.readEcho = UART_ECHO_ON;
      uartParams.baudRate = 9600;
      uart_dbg_bus = UART_open(DBG, &uartParams);

      //UART_write(uart_dbg_bus, "Hello DBG\n", 10);

      //char msg[] = "press key to continue\n";
      //UART_write(uart_dbg_bus, msg, strlen(msg));

      char resp[10];
      int32_t res = 0;
      //do {
          //UART_write(uart_dbg_bus, "Hello DBG\n", 10);
          sleep(1);
        res = UART_read(uart_dbg_bus, resp, 1);
     //} while(res <= 0);

}

#if (SUBS_TESTS != 2 && SUBS_TESTS > 0)

/*
 *  ------------ FRAM functions ------------
 *  each subsys has only on FRAM, so no need for changes.
 */

// Status register
#define FRAM_WPEN       0x80
#define FRAM_BP1        0x08
#define FRAM_BP0        0x04
#define FRAM_WEL        0x02

// OP-CODE
#define FRAM_WREN       0x06
#define FRAM_WRDI       0x04
#define FRAM_RDSR       0x05
#define FRAM_WRSR       0x01
#define FRAM_READ       0x03
#define FRAM_WRITE      0x02

#define FRAM_MAX_TRANSACTION 20 //mem size
#define FRAM_MEM_SIZE   0x8000  //Memory size

/*Dummy definitions for compilation reasons, comment when necessary*/
//#define FRAM 1
//#define FRAM_CS 1

SPI_Handle spi_fram;

void FRAM_init()
{
    FRAM_write_Disable();
}

void SPI_readWrite(void *writeBuf,
                   size_t count,
                   void *readBuf) {

  SPI_Transaction spiTransaction;

  spiTransaction.count = count;
  spiTransaction.txBuf = (void *)writeBuf;
  spiTransaction.rxBuf = (void *)readBuf;

  GPIO_write(FRAM_CS, 0);
  SPI_transfer(spi_fram, &spiTransaction);
  GPIO_write(FRAM_CS, 1);


}

/*
 *  Enable data writing in FRAM memory space
 *
 */
void FRAM_write_Enable()
{
  uint8_t tx = FRAM_WREN;
  uint8_t rx = 0;
  SPI_readWrite(&tx, 1, &rx);
}

/**  Disable data writing in FRAM memory space
 *
 */
void FRAM_write_Disable()
{
  uint8_t tx = FRAM_WRDI;
  uint8_t rx = 0;
  SPI_readWrite(&tx, 1, &rx);
}

/**  Returns content of FRAM status register
 *
 *   Returns
 *   unsigned char         status register value
 */
void FRAM_read_Status(uint8_t *res)
{
  uint8_t tx[2] = { FRAM_RDSR, 0x00 };
  uint8_t rx[2] = {0};
  SPI_readWrite(&tx, 2, &rx);
  *res = rx[1];
}

/**  reads sequential memory locations to buffer
 *
 *   Parameters
 *   unsigned int address       start address
 *   void *Ptr                  variable pointer
 *   unsigned int size          total number of bytes
 *
 */
void FRAM_read(uint16_t address, uint8_t *buf, size_t count)
{

  uint8_t tx[FRAM_MAX_TRANSACTION] = { FRAM_READ,
                                       (uint8_t)address >> 8,
                                       (uint8_t)address};
  uint8_t rx[FRAM_MAX_TRANSACTION] = {0};

  SPI_readWrite(&tx, count+3, &rx);
  memcpy(buf, &rx[3], count);

}


/**  writes to sequential memory locations from buffer
 *
 *   Parameters
 *   unsigned int address       start address
 *   void *Ptr                  variable pointer
 *   unsigned int size          total number of bytes
 *
 */
void FRAM_write(uint16_t address, uint8_t *buf, size_t count)
{
  uint8_t tx[FRAM_MAX_TRANSACTION] = { FRAM_WRITE,
                                       (uint8_t)address >> 8,
                                       (uint8_t)address};
  uint8_t rx[FRAM_MAX_TRANSACTION] = {0};
  memcpy(&tx[3], buf, count);

  FRAM_write_Enable();
  SPI_readWrite( &tx, count+3, &rx);
  FRAM_write_Disable();
}

void fram_test() {

         SPI_Params spiParams;

         GPIO_write(FRAM_CS, 1);

         SPI_Params_init(&spiParams);
         spiParams.frameFormat = SPI_POL0_PHA0;
         spiParams.bitRate = 100000;
         spi_fram = SPI_open(FRAM, &spiParams);

        uint8_t rx[15] = {0};
        uint8_t tx[15];

        FRAM_init();

        uint8_t res = 0;
        //FRAM_read_Status( &res);

        for(uint16_t i=0; i <= 15; i++) {
            tx[i] = i+10;
        }
        FRAM_write(0x16, tx, 15);
        FRAM_read( 0x16, rx, 15);

        bool res_fram = true;
        for(uint16_t i=0; i < 15; i++) {
                    if(!(rx[i] == i+10)) {
                        res = false;
                    }
        }

        char msg[20];
        if(res_fram) { sprintf(msg, "FRAM works\n"); }
        else { sprintf(msg, "FRAM failed\n"); }

        UART_write(uart_dbg_bus, msg, strlen(msg));

        // for(uint16_t i=0; i < 200; i++) {
        //  FRAM_write(EPS_FRAM_DEV_ID, 0x16, tx, 15);
        //  FRAM_read(EPS_FRAM_DEV_ID, 0x16, rx, 15);
        //   for(uint32_t d = 0; d < 100000; d++) {}
        // }

}

#endif

/*
 *  ------------ I2C functions ------------
 */


I2C_Handle      i2c;
uint8_t i2c_rx_temp_buff[30];
uint8_t i2c_tx_temp_buff[30];

void HAL_I2C_readWrite(int i2c_add,
                                  void *writeBuf,
                                  size_t tx_count,
                                  void *readBuf,
                                  size_t rx_count) {

  I2C_Transaction i2cTransaction;
  //uint8_t i2c_add = 0;

 //memcpy(i2c_tx_temp_buff, (uint8_t*)writeBuf, tx_count);

  i2cTransaction.slaveAddress = i2c_add;
  //i2cTransaction.writeBuf = i2c_tx_temp_buff;
  i2cTransaction.writeBuf = writeBuf;
  i2cTransaction.writeCount = tx_count;
  i2cTransaction.readBuf = i2c_rx_temp_buff;
  i2cTransaction.readCount = rx_count;
  I2C_transfer((i2c), &i2cTransaction);

    memcpy(readBuf, i2c_rx_temp_buff, rx_count);

}

#if (SUBS_TESTS == 4 || SUBS_TESTS == 5 || SUBS_TESTS == 7)

void init_i2c_EPS_BRD() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(EPS_BRD, &i2cParams);

}
#endif

#if (SUBS_TESTS == 6 || SUBS_TESTS == 7)

void init_i2c_EPS_SOL() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(EPS_SOL, &i2cParams);

}

#endif

#if (SUBS_TESTS == 5 || SUBS_TESTS == 7)

void init_i2c_EPS_BATT() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(EPS_BATT, &i2cParams);

}

#endif


#if (SUBS_TESTS == 3)

void init_i2c_ADCS_MON() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(I2C_MON, &i2cParams);

}

void init_i2c_ADCS_SEN1() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(I2C_SEN1, &i2cParams);

}

void init_i2c_ADCS_SEN2() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(I2C_SEN2, &i2cParams);

}

#endif

#if (SUBS_TESTS == 1)

void init_i2c_OBC() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(I2C_MON, &i2cParams);

}
#endif

#if (SUBS_TESTS == 2)

void init_i2c_ANT() {

  I2C_Params      i2cParams;

  I2C_Params_init(&i2cParams);
  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_100kHz;

  i2c = I2C_open(I2C_MON, &i2cParams);

}

#endif

/*
 *  ------------ INA functions ------------
 */

void ina_test(uint8_t i2c_add) {

    /* Buffers used in this code example */
    uint8_t             txBuffer[10];
    uint8_t             rxBuffer[10];

    /* I2C ina226 read manufacture id test*/
    /* Initialize all buffers */
    for (uint8_t i = 0; i < 10; i++) {
        rxBuffer[i] = 0x00;
        txBuffer[i] = 0x00;
    }

    // response
    I2C_Transaction i2cTransaction;

        /*
         * FEh Manufacturer ID Register
         */
        txBuffer[0] = 0xFE;

        /*
         * Response from slave 0x5449
         */
        i2cTransaction.slaveAddress = i2c_add;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 2;
        I2C_transfer(i2c, &i2cTransaction);

        bool res = false;

        if(rxBuffer[0] == 0x54 && rxBuffer[1] == 0x49) {
            res = true;
        }

        txBuffer[0] = 0xFF;
        rxBuffer[0] = 0;
        rxBuffer[1] = 0;

        i2cTransaction.slaveAddress = i2c_add;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 2;
        I2C_transfer(i2c, &i2cTransaction);

        char msg[30];
        if(res) { sprintf(msg, "INA %x, ID %x%x works\n", i2c_add, rxBuffer[1], rxBuffer[0]); }
        else { sprintf(msg, "INA %x failed\n", i2c_add); }
        UART_write(uart_dbg_bus, msg, strlen(msg));

}

void ina_meas(uint8_t i2c_add) {

    // Configure INA226
      INA226_configure2(i2c_add, INA226_AVERAGES_1, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);

      // Calibrate INA226. Rshunt = 0.01 ohm, Max excepted current = 4A
      INA226_calibrate2(i2c_add, 0.04, 0.5);

  //ina_setShuntResistor(i2c_add, 0.040);

               char msg[50];
               char resp[10];
               int32_t res = 0;
               do {
                   float v,c,p,v2;

                  p = INA226_readBusPower2(i2c_add);
                   usleep(10);
                  c = INA226_readShuntCurrent2(i2c_add);
                   usleep(10);
                  v = INA226_readBusVoltage2(i2c_add);
                   usleep(10);
                   v2 = INA226_readShuntVoltage2(i2c_add);
                                      usleep(10);

                   uint16_t con = INA226_getCalibration(i2c_add);
//                 ina_setShuntResistor(i2c_add, 0.40);
//                 usleep(1);
//                 uint16_t curr = ina_getCurrent(i2c_add);
//                 usleep(1);
//                 curr = ina_getCurrent(i2c_add);
//                 usleep(1);
//
//                 usleep(10);
//                 ina_setShuntResistor(i2c_add, 0.40);
//                                  usleep(1);
//                                  uint16_t volt = ina_getVoltage(i2c_add);
//                                  usleep(1);
//                                  volt = ina_getVoltage(i2c_add);
//                                  usleep(1);
//
//                                  usleep(10);
//                                  ina_setShuntResistor(i2c_add, 0.40);
//                                                   usleep(1);
//                                                   uint16_t watt = ina_getPower(i2c_add);
//                                                   usleep(1);
//                                                   watt = ina_getPower(i2c_add);
//                                                   usleep(1);
//
//                 //int16_t volt = ina_getVoltage(i2c_add);
//                 //usleep(1);
//
//                 //uint16_t watt = ina_getPower(i2c_add);
//                 //usleep(1);

                   sprintf(msg, "INA: %d, C %d, V %d, W %d, V2: %d, press key to continue\n", i2c_add, (int)(c * 1000), (int)v, (int)p, (int)v2);

                 //sprintf(msg, "INA: %d, C %d, V %d, W %d, press key to continue\n", i2c_add, curr, volt, watt);
                 UART_write(uart_dbg_bus, msg, strlen(msg));

                 res = UART_read(uart_dbg_bus, resp, 1);

              } while(res <= 0);


}

/*
 *  ------------ LTC functions ------------
 */

void er() {
  sleep(1);
}

float ltc_to_c(uint8_t raw_m, uint8_t raw_l) {
    uint16_t temp = (raw_m << 8) | raw_l;
    float st1 = (float) temp;

    float st2 = st1/65535;
    float st3 = st2*600;
    float st4 = st3-(float)(272.15);
    return st4;
}

float ltc_to_v(uint8_t raw_m, uint8_t raw_l) {
    uint16_t temp = (raw_m << 8) | raw_l;
    float st1 = (float) temp;

    float st2 = st1/65535;
    float st3 = st2*6;
    return st3;
}

float ltc_to_a(uint8_t raw_m, uint8_t raw_l) {
    uint16_t temp = (raw_m << 8) | raw_l;
    float st1 = (float) temp;

    //float st2 = st1*(float)0.46;
    //float st2 = (float)(st1 * (float)7.0);
    float st2 = (float)(st1 * (float)0.085);
    return st2;
}

struct ltc_device {
  dev_id id;
  uint16_t R_sense;
  uint8_t M;
  uint16_t Q;
  uint16_t R;
  uint16_t I;
  int16_t temp;
}ltc_dev;

void ltc_test() {

    uint8_t t_l, t_m, v_lsb, v_msb , err_flag, status_reg, acc_l, acc_m = 0;

    ltc_dev.R_sense = 0;
      ltc_dev.M = 0;
      ltc_dev.Q = 750;
      ltc_dev.R = 33;
      ltc_dev.I = 1500;
      ltc_init(ltc_dev.id,
               &(ltc_dev.R_sense),
               &(ltc_dev.M),
               ltc_dev.Q,
               ltc_dev.R,
               ltc_dev.I);

    /* Buffers used in this code example */
        uint8_t             txBuffer[10];
        uint8_t             rxBuffer[10];
        bool result = false;

        char u_msg[100];
        char resp[10];
        int32_t u_res = 0;

        I2C_Transaction i2cTransaction;

                /* I2C LTC2942 test*/
                /* Initialize all buffers */
                for (uint8_t i = 0; i < 10; i++) {
                    rxBuffer[i] = 0x00;
                    txBuffer[i] = 0x00;
                }

                /*
                 * READ LTC2942 configuration Register
                 */
                txBuffer[0] = 0x01;
                txBuffer[1] = 0xC8;
                /*
                 * Response from slave for GETSTATUS Cmd
                 * rxBuffer[0] = status
                 */
                i2cTransaction.slaveAddress = 0x64;
                i2cTransaction.writeBuf = txBuffer;
                i2cTransaction.writeCount = 1;
                i2cTransaction.readBuf = rxBuffer;
                i2cTransaction.readCount = 1;
while(result == false) {
                result = I2C_transfer(i2c, &i2cTransaction);
                if(result == false) {
                    UART_write(uart_dbg_bus, "e\n", 2);
                }
}

                /*
                 * WRITE LTC2942 configuration Register
                 */
                txBuffer[0] = 0x01;
                txBuffer[1] = 0xC8;
                /*
                 * Response from slave for GETSTATUS Cmd
                 * rxBuffer[0] = status
                 */
                i2cTransaction.slaveAddress = 0x64;
                i2cTransaction.writeBuf = txBuffer;
                i2cTransaction.writeCount = 2;
                i2cTransaction.readBuf = rxBuffer;
                i2cTransaction.readCount = 0;

               // result = I2C_transfer(i2c, &i2cTransaction);
                if(result == false) {
                                    UART_write(uart_dbg_bus, "e\n", 2);
                                }



                while(1) {
 t_l = 0;
 t_m = 0;
err_flag = 0;
 status_reg = 0;
  acc_l = 0;
  acc_m = 0;
    /*
                     * WRITE LTC2942 configuration Register
                     */
                    txBuffer[0] = 0x0D;
                    txBuffer[1] = 0xF8;

                    rxBuffer[0] = 0x00;
                    result = false;
                    /*
                     * Response from slave for GETSTATUS Cmd
                     * rxBuffer[0] = status
                     */
                    i2cTransaction.slaveAddress = 0x64;
                    i2cTransaction.writeBuf = txBuffer;
                    i2cTransaction.writeCount = 1;
                    i2cTransaction.readBuf = rxBuffer;
                    i2cTransaction.readCount = 1;

                    result = I2C_transfer(i2c, &i2cTransaction);
                    if(result == false) {
                        err_flag = true;
                    }
                    t_l = rxBuffer[0];
                    usleep(10);

                    /*
                     * WRITE LTC2942 configuration Register
                     */
                    txBuffer[0] = 0x0C;
                    txBuffer[1] = 0xF8;

                    rxBuffer[0] = 0x00;
                    result = false;
                    /*
                     * Response from slave for GETSTATUS Cmd
                     * rxBuffer[0] = status
                     */
                    i2cTransaction.slaveAddress = 0x64;
                    i2cTransaction.writeBuf = txBuffer;
                    i2cTransaction.writeCount = 1;
                    i2cTransaction.readBuf = rxBuffer;
                    i2cTransaction.readCount = 1;

                    result = I2C_transfer(i2c, &i2cTransaction);
                    if(result == false) {
                        err_flag = true;
                    }
                    t_m = rxBuffer[0];
                    usleep(10);
                                        /*
                     * WRITE LTC2942 configuration Register
                     */
                    txBuffer[0] = 0x09;
                    txBuffer[1] = 0xF8;

                    rxBuffer[0] = 0x00;
                                        result = false;
                    /*
                     * Response from slave for GETSTATUS Cmd
                     * rxBuffer[0] = status
                     */
                    i2cTransaction.slaveAddress = 0x64;
                    i2cTransaction.writeBuf = txBuffer;
                    i2cTransaction.writeCount = 1;
                    i2cTransaction.readBuf = rxBuffer;
                    i2cTransaction.readCount = 1;

                    result = I2C_transfer(i2c, &i2cTransaction);
                    if(result == false) {
                        err_flag = true;
                    }
                    v_lsb = rxBuffer[0];
                    usleep(10);
                                                            /*
                     * WRITE LTC2942 configuration Register
                     */
                    txBuffer[0] = 0x08;
                    txBuffer[1] = 0xF8;

                    rxBuffer[0] = 0x00;
                    result = false;
                    /*
                     * Response from slave for GETSTATUS Cmd
                     * rxBuffer[0] = status
                     */
                    i2cTransaction.slaveAddress = 0x64;
                    i2cTransaction.writeBuf = txBuffer;
                    i2cTransaction.writeCount = 1;
                    i2cTransaction.readBuf = rxBuffer;
                    i2cTransaction.readCount = 1;

                    result = I2C_transfer(i2c, &i2cTransaction);
                    if(result == false) {
                        err_flag = true;
                    }
                    v_msb = rxBuffer[0];
                    usleep(10);

        /*
           * start temp LTC2942 configuration Register
           */
          txBuffer[0] = 0x02;

          rxBuffer[0] = 0x00;
          result = false;
          /*
           * Response from slave for GETSTATUS Cmd
           * rxBuffer[0] = status
           */
          i2cTransaction.slaveAddress = 0x64;
          i2cTransaction.writeBuf = txBuffer;
          i2cTransaction.writeCount = 1;
          i2cTransaction.readBuf = rxBuffer;
          i2cTransaction.readCount = 1;

          result = I2C_transfer(i2c, &i2cTransaction);
          if(result == false) {
              err_flag = true;
          }
          acc_m = rxBuffer[0];
          usleep(10);

          /*
             * start temp LTC2942 configuration Register
             */
            txBuffer[0] = 0x03;

            rxBuffer[0] = 0x00;
            result = false;
            /*
             * Response from slave for GETSTATUS Cmd
             * rxBuffer[0] = status
             */
            i2cTransaction.slaveAddress = 0x64;
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 1;
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 1;

            result = I2C_transfer(i2c, &i2cTransaction);
            if(result == false) {
                err_flag = true;
            }
            acc_l = rxBuffer[0];
            usleep(10);


      /*
         * start temp LTC2942 configuration Register
         */
        txBuffer[0] = 0x00;

        rxBuffer[0] = 0x00;
        result = false;
        /*
         * Response from slave for GETSTATUS Cmd
         * rxBuffer[0] = status
         */
        i2cTransaction.slaveAddress = 0x64;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 1;

        result = I2C_transfer(i2c, &i2cTransaction);
        if(result == false) {
            err_flag = true;
        }
        status_reg = rxBuffer[0];
        usleep(10);

        float t = ltc_to_c(t_m, t_l);
        float v = ltc_to_v(v_msb, v_lsb);
        float a = ltc_to_a(acc_m,acc_l);

        sprintf(u_msg, "LTS: %02x, %02x%02x, %02x%02x, %02x%02x, %d, %d, %d\n", status_reg, acc_m,acc_l,t_m, t_l, v_msb,v_lsb, (int)t, (int)v, (int)a);
        UART_write(uart_dbg_bus, u_msg, strlen(u_msg));



        u_res = UART_read(uart_dbg_bus, resp, 1);

        if(u_res > 0) {
        /*
           * start temp LTC2942 configuration Register
           */
          txBuffer[0] = 0x01;
          txBuffer[1] = 0xC9;

          rxBuffer[0] = 0x00;
          result = false;
          /*
           * Response from slave for GETSTATUS Cmd
           * rxBuffer[0] = status
           */
          i2cTransaction.slaveAddress = 0x64;
          i2cTransaction.writeBuf = txBuffer;
          i2cTransaction.writeCount = 2;
          i2cTransaction.readBuf = rxBuffer;
          i2cTransaction.readCount = 0;

          result = I2C_transfer(i2c, &i2cTransaction);
          if(result == false) {
              err_flag = true;
          }

          usleep(100);

          if(resp[0] == '1') {
            txBuffer[1] = 0x00;
          }  else if(resp[0] == '0') {
              txBuffer[1] = 0xff;
          } else {
            txBuffer[1] = 0x80;
          }

          /*
             * start temp LTC2942 configuration Register
             */
            txBuffer[0] = 0x02;

            rxBuffer[0] = 0x00;
            result = false;
            /*
             * Response from slave for GETSTATUS Cmd
             * rxBuffer[0] = status
             */
            i2cTransaction.slaveAddress = 0x64;
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 2;
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 0;

            result = I2C_transfer(i2c, &i2cTransaction);
            if(result == false) {
                err_flag = true;
            }

            usleep(100);
            /*
               * start temp LTC2942 configuration Register
               */
              txBuffer[0] = 0x03;

              rxBuffer[0] = 0x00;
              result = false;
              /*
               * Response from slave for GETSTATUS Cmd
               * rxBuffer[0] = status
               */
              i2cTransaction.slaveAddress = 0x64;
              i2cTransaction.writeBuf = txBuffer;
              i2cTransaction.writeCount = 2;
              i2cTransaction.readBuf = rxBuffer;
              i2cTransaction.readCount = 0;

              result = I2C_transfer(i2c, &i2cTransaction);
              if(result == false) {
                  err_flag = true;
              }

          /*
             * start temp LTC2942 configuration Register
             */
            txBuffer[0] = 0x01;
            txBuffer[1] = 0xC8;

            rxBuffer[0] = 0x00;
            result = false;
            /*
             * Response from slave for GETSTATUS Cmd
             * rxBuffer[0] = status
             */
            i2cTransaction.slaveAddress = 0x64;
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 2;
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 0;

            result = I2C_transfer(i2c, &i2cTransaction);
            if(result == false) {
                err_flag = true;
            }
          }
    }
}

void ltc_meas() {

    uint16_t R_sense;
    uint8_t  M;

    //Initialise Control Register
    ltc_init(0x64, &M, &R_sense, 750, 5, 1500);       //Battery capacity: 750mAh, Rsense: 5mOhm,Imax: 1500mA

    char msg[100];
                   char resp[10];
                   int32_t res = 0;
                   do {
                       int16_t temp;
                       uint16_t voltage;

                       uint32_t charge1;
                       uint32_t charge2;

                     ltc_code_to_celcius_temperature(0x64, &temp);
                     ltc_code_to_voltage(0x64, &voltage);

                     ltc_code_to_microAh(0x64, &R_sense, &M, &charge1);
                     ltc_code_to_millicoulombs(0x64, &R_sense, &M, &charge2);

                     sprintf(msg, "LTC, Temp %d, Volt %d, Charge mAh %d, Charge mC %d, press key to continue\n", temp, voltage, charge1, charge2);
                     UART_write(uart_dbg_bus, msg, strlen(msg));

                     res = UART_read(uart_dbg_bus, resp, 1);

                  } while(res <= 0);

}

/*
 *  ------------ TMP functions ------------
 */

void tmp_test(uint8_t i2c_add) {

        /* Buffers used in this code example */
         uint8_t             txBuffer[10];
         uint8_t             rxBuffer[10];

         I2C_Transaction i2cTransaction;

           /* I2C TP100 read manufacture id test*/
           /* Initialize all buffers */
           for (uint8_t i = 0; i < 10; i++) {
                rxBuffer[i] = 0x00;
                txBuffer[i] = 0x00;
            }

           char msg[30];
           char resp[10];
           int32_t res = 0;
           do {

            /*
             * TMP100 configuration Register
             */
            txBuffer[0] = 0x01;
            txBuffer[1] = 0x18;
            /*
             * Response from slave for GETSTATUS Cmd
             * rxBuffer[0] = status
             */
            i2cTransaction.slaveAddress = i2c_add;
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 2;
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 0;

            I2C_transfer(i2c, &i2cTransaction);

            /*
             * Read temp
             */
            txBuffer[0] = 0x00;

            rxBuffer[0] = 0x00;
            rxBuffer[1] = 0x00;

            /*
             * Response from slave for GETSTATUS Cmd
             * rxBuffer[0] = status
             */
            i2cTransaction.slaveAddress = i2c_add;
            i2cTransaction.writeBuf = txBuffer;
            i2cTransaction.writeCount = 1;
            i2cTransaction.readBuf = rxBuffer;
            i2cTransaction.readCount = 2;

            int16_t res_raw2 = 0;
            float res_temp2 = 0.0;
            /* Re-try writing to slave till I2C_transfer returns true */



                sleep(1);
                I2C_transfer(i2c, &i2cTransaction);
                res_raw2 = (int16_t)((int16_t)((int16_t)(rxBuffer[0] << 8) | (int16_t)(rxBuffer[1])) >> 4);
                res_temp2 = (float)res_raw2 * 0.0625;

                sprintf(msg, "Temp: %d, press key to continue\n", (int)res_temp2);
                UART_write(uart_dbg_bus, msg, strlen(msg));

                res = UART_read(uart_dbg_bus, resp, 1);

          } while(res <= 0);
}


/*
 *  ------------ BMX functions ------------
 */

#define BMG160_CHIP_ID_ADDR 0x00

void bmx_test() {

    /* Buffers used in this code example */
             uint8_t             txBuffer[10];
             uint8_t             rxBuffer[10];

             I2C_Transaction i2cTransaction;

             /*
                     * BMX055 Manufacturer ID Register
                     */
                    txBuffer[0] = BMG160_CHIP_ID_ADDR;
                    rxBuffer[0] = 0;

                    /*
                     * Response from slave for GETSTATUS Cmd
                     * rxBuffer[0] = status
                     * response should be 0x0F
                     */

                    i2cTransaction.slaveAddress = 0x68;
                    i2cTransaction.writeBuf = txBuffer;
                    i2cTransaction.writeCount = 1;
                    i2cTransaction.readBuf = rxBuffer;
                    i2cTransaction.readCount = 1;

                    I2C_transfer(i2c, &i2cTransaction);

                    char msg[30];
                            if(rxBuffer[0] == 0x0F) {
                                sprintf(msg, "BMX is working\n");
                            } else {
                                sprintf(msg, "BMX Failed\n");
                            }

                            UART_write(uart_dbg_bus, msg, strlen(msg));

}

/*
 *  ------------ MAG functions ------------
 */

#define MAG3110_I2C_ADDRESS 0x0E
#define MAG3110_WHO_AM_I    0x07

void mag_test() {

    /* Buffers used in this code example */
             uint8_t             txBuffer[10];
             uint8_t             rxBuffer[10];

             I2C_Transaction i2cTransaction;

        /*
         * MAG3110 Manufacturer ID Register
         * C4 response
         */
        txBuffer[0] = MAG3110_WHO_AM_I;

        /*
         * Response from slave for GETSTATUS Cmd
         * rxBuffer[0] = status
         */

        i2cTransaction.slaveAddress = MAG3110_I2C_ADDRESS;
        i2cTransaction.writeBuf = txBuffer;
        i2cTransaction.writeCount = 1;
        i2cTransaction.readBuf = rxBuffer;
        i2cTransaction.readCount = 1;

        I2C_transfer(i2c, &i2cTransaction);

        char msg[30];
        if(rxBuffer[0] == 0xC4) {
            sprintf(msg, "MAG is working\n");
        } else {
            sprintf(msg, "MAG Failed\n");
        }

        UART_write(uart_dbg_bus, msg, strlen(msg));

}

/*
 *  ------------ GPIO functions ------------
 */

/*
 *  ------------ EPS SW functions ------------
 */

#if (SUBS_TESTS >= 4 || SUBS_TESTS == -4)

void sw_test() {

    char msg[] = "press key to continue\n";
    char resp[10];
    int32_t res = 0;
    do {

        /* Turn on subsystem en switches*/
        GPIO_write(SBSYS_EN_SW0, 1);
        GPIO_write(SBSYS_EN_SW1, 1);
        GPIO_write(SBSYS_EN_SW2, 1);
        GPIO_write(SBSYS_EN_SW3, 1);

        UART_write(uart_dbg_bus, "SW on\n", 6);

        sleep(1);

        /* Turn on subsystem en switches*/
        GPIO_write(SBSYS_EN_SW0, 0);
        GPIO_write(SBSYS_EN_SW1, 0);
        GPIO_write(SBSYS_EN_SW2, 0);
        GPIO_write(SBSYS_EN_SW3, 0);

        UART_write(uart_dbg_bus, "SW off\n", 7);
        sleep(1);


      UART_write(uart_dbg_bus, msg, strlen(msg));
      res = UART_read(uart_dbg_bus, resp, 1);

    } while(res <= 0);

    /* Turn on subsystem en switches*/
    GPIO_write(SBSYS_EN_SW0, 1);
    GPIO_write(SBSYS_EN_SW1, 1);
    GPIO_write(SBSYS_EN_SW2, 1);
    GPIO_write(SBSYS_EN_SW3, 1);

}

void sw_open() {


        /* Turn on subsystem en switches*/
        GPIO_write(SBSYS_EN_SW0, 1);
        GPIO_write(SBSYS_EN_SW1, 1);
        GPIO_write(SBSYS_EN_SW2, 1);
        GPIO_write(SBSYS_EN_SW3, 1);

}

#endif


/*
 *  ------------ ANT SW functions ------------
 */

#if (SUBS_TESTS == 2 )

void dep_sw_test() {

    char msg[] = "press key to continue\n";
    char msg_res[30];
        char resp[10];
        int32_t res = 0;
        uint8_t sw[4];
        do {

            /* Turn on subsystem en switches*/
            sw[0] = GPIO_read(DEB_STATUS0);
            sw[1] = GPIO_read(DEB_STATUS1);
            sw[2] = GPIO_read(DEB_STATUS2);
            sw[3] = GPIO_read(DEB_STATUS3);

            sprintf(msg_res, "SW: %d,%d,%d,%d\n", sw[0], sw[1], sw[2], sw[3]);
            UART_write(uart_dbg_bus, msg_res, strlen(msg_res));
            sleep(1);


          //UART_write(uart_dbg_bus, msg, strlen(msg));
          res = UART_read(uart_dbg_bus, resp, 1);

        } while(res <= 0);

}

void burn_test() {

    char msg[] = "press key to continue\n";
    char resp[10];
    int32_t res = 0;
    do {

        /* Turn on subsystem en switches*/
        GPIO_write(BURN0, 1);
        GPIO_write(BURN1, 1);
        GPIO_write(BURN2, 1);
        GPIO_write(BURN3, 1);

        UART_write(uart_dbg_bus, "BURN on\n", 8);

        sleep(10);

        /* Turn on subsystem en switches*/
        GPIO_write(BURN0, 0);
        GPIO_write(BURN1, 0);
        GPIO_write(BURN2, 0);
        GPIO_write(BURN3, 0);

        UART_write(uart_dbg_bus, "BURN off\n", 9);
        sleep(5);




      UART_write(uart_dbg_bus, msg, strlen(msg));
      res = UART_read(uart_dbg_bus, resp, 1);

    } while(res <= 0);

}

#endif

/*
 *  ------------ ADCS TOR functions ------------
 */

// OBC 1
// ANT 2
// ADCS 3
// EPS 4
// EPS with BATT 5
// EPS with SOL  6
// EPS complete  7
#if (SUBS_TESTS == 3 )

void tor_test() {


    char msg[] = "press key to continue\n";
    char resp[10];
    int32_t res = 0;
    do {

        /* Turn on TOR en*/
        GPIO_write(TOR_EN0, 1);
        GPIO_write(TOR_EN1, 1);
        GPIO_write(TOR_EN1, 1);

        GPIO_write(TOR_POL0, 0);
        GPIO_write(TOR_POL0, 0);
        GPIO_write(TOR_POL0, 0);

        UART_write(uart_dbg_bus, "TOR on\n", 7);

        sleep(1);

        /* change pol*/

        GPIO_write(TOR_POL0, 1);
        GPIO_write(TOR_POL0, 1);
        GPIO_write(TOR_POL0, 1);

        UART_write(uart_dbg_bus, "TOR on, Changed POL\n", 20);

        sleep(1);

        /* Turn off TOR*/
        GPIO_write(TOR_EN0, 0);
        GPIO_write(TOR_EN1, 0);
        GPIO_write(TOR_EN1, 0);

        sleep(1);

        UART_write(uart_dbg_bus, "TOR off\n", 8);
        sleep(1);


      UART_write(uart_dbg_bus, msg, strlen(msg));
      res = UART_read(uart_dbg_bus, resp, 1);

    } while(res <= 0);

}

#endif

/*
 *  ------------ WDG functions ------------
 */
#if (SUBS_TESTS > 0)

bool wdg_reset_flag = true;

void wdg_test() {

        char msg[] = "WDG: press key to continue, r for reset\n";
        char resp[10];
        int32_t res = 0;
        do {

          sleep(1);

          UART_write(uart_dbg_bus, msg, strlen(msg));
          res = UART_read(uart_dbg_bus, resp, 1);

          if(res > 0 && resp[0] == 'r') {
              wdg_reset_flag = false;
          }

        } while(res <= 0);
}

void wdg_reset() {

          if(wdg_reset_flag) {
              GPIO_write(EXT_WDG, 1);
              usleep(100);
              GPIO_write(EXT_WDG, 0);
          }

          usleep(100);
}
#endif

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{

    /* Call driver init functions */
    GPIO_init();
    UART_init();
    #if (SUBS_TESTS != 2 && SUBS_TESTS > 0)
       SPI_init();
    #endif
    I2C_init();

    #if (SUBS_TESTS >= 4 || SUBS_TESTS == -4)
      sw_open();
    #endif

    uart_test();

    rs_test();


    //LTC board specific
    #if (SUBS_TESTS == 5)
    init_i2c_EPS_BRD();

    init_i2c_EPS_BATT();



    ltc_test();
    #endif


#if (SUBS_TESTS == 1 || SUBS_TESTS == 0 || SUBS_TESTS == -4)
    rs_tx_addr_test();
    //rs_tx_stress_test();
#elif (SUBS_TESTS == -2 || SUBS_TESTS == -3)
    pc_interface();
#else
    rs_rx_addr_test();
#endif

    while(1) {
        sleep(1);
    }

    //wdg_test();

    {
        // OBC 1
        // ANT 2
        // ADCS 3
        // EPS 4
        // EPS with BATT 5
        // EPS with SOL  6
        // EPS complete  7
        char msg[20];

        #if (SUBS_TESTS == 1)
        sprintf(msg, "Testing OBC\n");
        #elif (SUBS_TESTS == 2)
        sprintf(msg, "Testing ANT\n");
        #elif (SUBS_TESTS == 3)
        sprintf(msg, "Testing ADCS\n");
        #elif (SUBS_TESTS >= 4)
        sprintf(msg, "Testing EPS\n");
        #else
        sprintf(msg, "Testing null\n");
        #endif

        UART_write(uart_dbg_bus, msg, strlen(msg));
    }



    //ANT board specific
    #if (SUBS_TESTS == 2)
    init_i2c_ANT();

    ina_test(0x40);
    ina_meas(0x40);

    tmp_test(0x48);

    burn_test();
    dep_sw_test();
    #endif

    //EPS board specific
    #if (SUBS_TESTS >= 4)
    //fram_test();

   // sw_test();

    init_i2c_EPS_BRD();

    ina_test(0x40);
    ina_test(0x41);
    ina_test(0x42);
    ina_test(0x43);
    ina_test(0x48);
    ina_test(0x4A);

    ina_meas(0x40);
  //  ina_meas(0x41);
  //  ina_meas(0x42);
  //  ina_meas(0x43);
  //  ina_meas(0x48);
  //  ina_meas(0x4A);
    #endif


    //EPS solar specific
    #if (SUBS_TESTS == 6 || SUBS_TESTS == 7)
    init_i2c_EPS_SOL();

    ina_test(0x40);
    ina_test(0x41);
    ina_test(0x42);
    ina_test(0x43);

    ina_meas(0x40);
    ina_meas(0x41);
    ina_meas(0x42);
    ina_meas(0x43);

    tmp_test(0x4F);
    tmp_test(0x4D);
    tmp_test(0x49);
    #endif


    #if (SUBS_TESTS == 5 || SUBS_TESTS == 7)
    init_i2c_EPS_BATT();

    ltc_test();
    #endif

    //OBC specific
    #if (SUBS_TESTS == 1)
    fram_test();

    init_i2c_OBC();

    ina_test(0x40);

    ina_meas(0x40);

    tmp_test(0x48);
    #endif


    //ADCS specific
    #if (SUBS_TESTS == 3)
        fram_test();

        init_i2c_ADCS_MON();

        ina_test(0x40);
        ina_test(0x41);
        ina_test(0x42);
        ina_test(0x43);

        ina_meas(0x40);
        ina_meas(0x41);
        ina_meas(0x42);
        ina_meas(0x43);

        tmp_test(0x48);

        init_i2c_ADCS_SEN1();

        bmx_test();
        mag_test();

        init_i2c_ADCS_SEN2();

        bmx_test();
        mag_test();

        tor_test();
    #endif

    /* Loop forever echoing */
    while (1) {

    }
}

/*  ======== wdgThread ========
 *  This thread runs on a higher priority, since wdg pin
 *  has to be ready for master.
 */
#if (SUBS_TESTS > 0)
void *wdgThread(void *arg0)
{

    /* Loop forever */
    while (1) {
        wdg_reset();
    }

    return (NULL);
}
#endif
