/* Bunch-to-bucket transfer */
/* Author: Jiaoni Bai */
/* Date: 04.11.2015 */

/* C Standard Includes*/
/*====================================================================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* GSI LM32 Includes */
/*====================================================================================*/
#include "mprintf.h"
#include "mini_sdb.h"
#include "ebm.h"
#include "aux.h"


/* Vender ID and device ID */
#define venID 0x651
#define devID_SCUBM 0x9602eb6f
#define devID_EBM   0x00000815
/* #define devID_ECA   0x9602eb6f for timestamp */

/* SCU Slave address */
/*====================================================================================
  E.g. the slave is at slot 5
  Base Address for the slave is 5 * 2^17 + 0x400000 = 0x4a0000
  The Phase Advance Prediction module is in slot 1
  The Phase Correction module is in slot 2
  The Phase Shift module is in slot 3
====================================================================================*/


/* SCU slave Phase Advance Prediction address offset: read */
#define OS_PAP 0x6
/* SCU slave Phase Correction address offset: write */
#define OS_PC 0x10
/* SCU slave Phase Shift address offset: write */
#define OS_PS 0x10

/* variable to the base address */
volatile unsigned short* pSCUbm = 0;
volatile unsigned short* pEBm = 0;

/* variables for the data from the SCU slaves */
volatile uint32_t v_PAP = 0;
volatile uint32_t v_PC = 0;
volatile uint32_t v_PS = 0;

/* send data to SCU slave */
void send_slave_param(int slave_nr, int offset, uint32_t value)
{
  pSCUbm[(slave_nr << 16) + offset] = value;
  mprintf("offset %x %x \n", pSCUbm[(slave_nr << 16) + offset],&pSCUbm[(slave_nr << 16) + offset]);
}

/* read data from SCU slave */
void read_slave_param(int slave_nr, int offset, uint32_t value)
{
  value = pSCUbm[(slave_nr << 16) + offset];
  mprintf("read %x \n", value);
  return;
}

/* Function main */
int main (void)
{
  /* Get uart unit address */
  discoverPeriphery();

  /* Initialize uart unit and other cores*/
  uart_init_hw();
  ebm_init();
  /* Display welcome message */
  mprintf("B2B main: Hello Word!\n");

  /* Base address of the SCU ip cores */
  pSCUbm =(unsigned int*)find_device_adr(venID,devID_SCUBM);
  pEBm = (unsigned int*)find_device_adr(venID,devID_EBM);
  mprintf("base address %x %x\n",pSCUbm,pEBm);

  /* Init EB Master */
  //config the source and destination MAC, ip, port
  ebm_config_if(LOCAL, "hw/00:26:7b:00:03:d7/udp/192.168.0.1/port/60368");
  ebm_config_if(REMOTE, "hw/ff:ff:ff:ff:ff:ff/udp/192.168.0.2/port/60369");
  ebm_config_meta(80, 0x11, 16, 0x00000000 );
  //creat WB cycle : WRITE/READ
  //Event ID 64 bit
  //atomic_on();
  ebm_op(0x55000333, 0xAAAAAAAA, 0x400000);
  ebm_op(0x55000333, 0xBBBBBBBB, 0x400000);
  ebm_op(0x55000333, 0xCCCCCCCC, 0x400000);
  //atomic_off();
  //Parameters 64 bit
  //Timestamp 64 bit
  //send
  ebm_flush();

  send_slave_param(2, OS_PC, 0xdeff);
  read_slave_param(2, OS_PC, v_PC);
  mprintf("result2 %x \n", v_PC);

  return (0);

}


