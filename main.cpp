/*****************************************************************//**
 * @file main.cpp
 *
 * @brief display temperature on sseg
 *
 * @author aidan highsmith
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"


void float_to_8ptn(float value, uint8_t *arr)
{
    char buf[16];

    // Format with required precision (3 decimals here)
    snprintf(buf, sizeof(buf), "%.3f", value);

    int count = 0;
    for (int i = 0; buf[i] != '\0'; i++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            arr[count++] = buf[i] - '0';
        }
    }
}

void sseg_display(SsegCore *sseg_p, float tmpC, char unit) {
   int i, n;
   uint8_t dp;
   uint8_t data[8];
   float tmpF;

   //turn off led
   for (i = 0; i < 8; i++) {
      sseg_p->write_1ptn(0xff, i);
   }
   //turn off all decimal points
   sseg_p->set_dp(0x00);
   
   
   if(unit == 'F') {
       //fahrenheit
       tmpF = (9.0/5.0)*tmpC + 32.0;
       sseg_p->write_1ptn(0x8e, 0);
       float_to_8ptn(tmpF,data);
   }
   else if (unit == 'C') {
       //celsius
       sseg_p->write_1ptn(0xc6, 0);
       float_to_8ptn(tmpC,data);
   }
   
   for(int i=1; i<5; i++) {
       sseg_p->write_1ptn(sseg_p->h2s(data[4-i]), i);
       
       /*
       uart.disp("digit: ");
       uart.disp((data[i]));
       uart.disp("\n\r");*/
   }
   
   //sseg_p->write_8ptn(data);
   sseg_p->set_dp(0x08);
   sleep_ms(300);

   /*
   // display 0x0 to 0xf in 4 epochs
   // upper 4  digits mirror the lower 4
   for (n = 0; n < 4; n++) {
      for (i = 0; i < 4; i++) {
         sseg_p->write_1ptn(sseg_p->h2s(i + n * 4), 3 - i);
         sseg_p->write_1ptn(sseg_p->h2s(i + n * 4), 7 - i);
         sleep_ms(300);
      } // for i
   }  // for n
      // shift a decimal point 4 times
   for (i = 0; i < 4; i++) {
      bit_set(dp, 3 - i);
      sseg_p->set_dp(1 << (3 - i));
      sleep_ms(300);
   }*/
   /*
   //turn off led
   for (i = 0; i < 8; i++) {
      sseg_p->write_1ptn(0xff, i);
   }
   //turn off all decimal points
   sseg_p->set_dp(0x00);
   */
}


float adt7420_check(I2cCore *adt7420_p, GpoCore *led_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   //int ack;
   uint16_t tmp;
   float tmpC;

   // read adt7420 id register to verify device existence
   // ack = adt7420_p->read_dev_reg_byte(DEV_ADDR, 0x0b, &id);

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);
   uart.disp("read ADT7420 id (should be 0xcb): ");
   uart.disp(bytes[0], 16);
   uart.disp("\n\r");
   //debug("ADT check ack/id: ", ack, bytes[0]);
   // read 2 bytes
   //ack = adt7420_p->read_dev_reg_bytes(DEV_ADDR, 0x0, bytes, 2);
   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
   }
   /*
   uart.disp("temperature (C): ");
   uart.disp(tmpC);
   uart.disp("\n\r");
   led_p->write(tmp);
   sleep_ms(1000);
   led_p->write(0);
   */

   return tmpC;
}

GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));


int main() {
   float tmpC;
   char unit='C';
   while (1) {
      tmpC = adt7420_check(&adt7420, &led);
      sseg_display(&sseg, tmpC, unit);
   } //while
} //main