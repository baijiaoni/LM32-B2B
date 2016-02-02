/* Bunch-to-bucket transfer */
/* Author: Jiaoni Bai */
/* Date: 04.11.2015 */

/* Code for source B2B SCU */
/* ===================================================================================
Soure B2B SCU
      Slot 1-2>> Phase advance prediction module
      Slot 3-4>> Phase correction module
      Slot 5-6>> Phase shift module
      Slot 7-8>> Reference Group DDS
===================================================================================*/

/* C Standard Includes*/
/*====================================================================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

/* GSI LM32 Includes */
/*====================================================================================*/
#include "mprintf.h"
#include "mini_sdb.h"
#include "ebm.h"
#include "aux.h"
#include "hw-tlu.h"

/* Vender ID and device ID */
#define venID 0x651
#define devID_SCUBM 0x9602eb6f
#define devID_EBM   0x00000815
#define devID_ECA   0x8752bf44
#define devID_TLU   0x10051981
#define LM32_CB_CLUSTER       0x10041000
#define LM32_IRQ_EP           0x10050083

/* #define devID_ECA   0x9602eb6f for timestamp */

/* SCU Slave address */
/*====================================================================================
  E.g. the slave is at slot 5
  Base Address for the slave is 5 * 2^17 + 0x400000 = 0x4a0000
  The Phase Advance Prediction module is in slot 1
  The Phase Correction module is in slot 2
  The Phase Shift module is in slot 3
====================================================================================
  EVT_B2B_START timestamp must be aligned with a T0 edge, tm + 100us corresponds to
  the PAP predicted phase.

====================================================================================*/
/* SCU slave Phase Advance Prediction address offset: read */
#define OS_PAP 0x6
/* SCU slave Phase Correction address offset: write */
#define OS_PC 0x10
/* SCU slave Phase Shift address offset: write */
#define OS_PS 0x10

/* Timing message*/
#define Ebm_WRITE 0x400000
#define WB_Addr   0x7ffffff0
#define FID_GID   0x33333333
#define Param     0x4444444455555555
#define Timestamp 0x1234567812345678

#define Source_B2B_SCU 1
#define Target_B2B_SCU 0

#define SCALE 1000000000000 //ps

#define EVT_START_B2B_tag   0x0B2B0001
#define TGM_PHASE_TIME_tag  0x0B2B0001
#define TGM_SYNCH_WIN       0xB001000000000C00
#define TGM_SYNCH_WIN_tag   0x0B2B0003


#define Phase_Shift 0
#define Frequency_Beating 0

#define Phase_Shift_Time 9000000000 % unit ps

/* Timining message structure */
#define Param_os      0x4
#define TEF_os        (Param_os + 0x4)
#define Reserved_os   (TEF_os + 0x2)
#define Timestamp_os  (Reserved_os + 0x2)


/* variable to the base address */
volatile unsigned short* pSCUbm = 0;
volatile unsigned short* pEBm   = 0;
volatile unsigned short* pECA   = 0;
volatile unsigned short* pECA_Q = 0;
volatile unsigned * pTLU = 0;


/* variables for h1 phase read from the PAP module */
static uint16_t predicted_phase_h1_src = 0;
static uint16_t predicted_phase_ts = 0;
static uint16_t predicted_phase_h1_trg = 0;
/* variables for calculated high harmonic phase */
static uint16_t phase_high_harmonic_src = 0;
static uint16_t phase_high_harmonic_trg = 0;
/* variables for calculated phase correction and phase shift for the source machine */
static uint16_t phase_correction_h1_src = 0;
static uint16_t phase_shift_high_src = 0;
/* variables for the parameters for source and target machines */
// source and target machine high harmonic rf frequencies
static uint32_t freq_high_harmonic_src = 1572200;
static uint32_t freq_high_harmonic_trg = 1572000;
//source and target machines rf harmonic number
static uint16_t harmonic_src = 2;
static uint16_t harmonic_trg = 10;
//source and target machine rf period with cavity harmonics: uint ps
static uint64_t period_high_harmonic_src = 0;
static uint64_t period_high_harmonic_trg = 0;
//source and target machine rf requency with h=1 : uint ps
static uint64_t period_h1_src = 0;
static uint64_t period_h1_trg = 0;
//source and target machine tm for the zero-crossing of high harmonics
static uint64_t tm_high_zero_src = 0;
static uint64_t tm_high_zero_trg = 0;
//timestamp for the predicted phase of PAP
static uint64_t predict_phase_ts = 0;
//the start time of the B2B transfer system
static uint64_t EVT_B2B_START_ts = 0;
// the TOF
static uint64_t tof_time = 0;
// the predicted phase uncertainty
static uint16_t predicted_phase_uncertainty_time = 0;

sdb_location lm32_irq_endp[10];


unsigned int cpuId;


void init()
{
  /* Get uart unit address */
  discoverPeriphery();
  uart_init_hw();
  ebmInit();
  cpuId = getCpuIdx();
  //init_irq_handler();

}

/* TLU simulates the next zero cross timestamp of two machines */
uint64_t get_rising_edge_tm_from_FG ()
{
  mprintf("/************************TLU************************/\n");
  uint32_t B1_timestamp_hi;
  uint32_t B1_timestamp_lo;
  uint32_t B2_timestamp_hi;
  uint32_t B2_timestamp_lo;
  uint32_t count;

  /* TLU configure */
  /*Configure the TLU to record rising edge timestamps at the same time*/
  *(pTLU + (TLU_ACTIVE_SET >> 2)) = 0x3;
  count = *(pTLU + (TLU_CH_FILL_COUNT >> 2));
  mprintf("TLU active %x \n",count);
  *(pTLU + (TLU_CLEAR >> 2)) = 0x3;
  count = *(pTLU + (TLU_CH_FILL_COUNT >> 2));
  mprintf("TLU clear activate %x \n",count);

  *(pTLU + (TLU_CH_SELECT >> 2)) = 0x0;
  B1_timestamp_hi = *(pTLU + (TLU_CH_TIME1 >> 2));
  B1_timestamp_lo = *(pTLU + (TLU_CH_TIME0 >> 2));
  tm_high_zero_src = ((uint64_t)B1_timestamp_hi << 32) | B1_timestamp_lo;
  mprintf("SIS18 high harmonic zero-cross timestamp (CH 0) %x %x\n",B1_timestamp_hi,B1_timestamp_lo);


  *(pTLU + (TLU_CH_SELECT >> 2)) = 0x1;  //channel 1
  B2_timestamp_hi = *(pTLU + (TLU_CH_TIME1 >> 2));
  B2_timestamp_lo = *(pTLU + (TLU_CH_TIME0 >> 2));
  tm_high_zero_trg = ((uint64_t)B2_timestamp_hi << 32) | B2_timestamp_lo;
  mprintf("SIS100 high harmonic zero-cross timestamp (CH 1) %x %x\n",B2_timestamp_hi,B2_timestamp_lo);

  return tm_high_zero_src, tm_high_zero_trg;
}

//void init_irq_handler()
//{
//  isr_table_clr();
//  isr_ptr_table[1] = & irq_handler;
//  //isr_ptr_table[2] = &
//  //isr_ptr_table[3] = &
//}

/* Etherbone master send Eb telegram */
void ebmInit()
{
  ebm_init();
  //config the source and destination MAC, ip, port
  ebm_config_if(LOCAL, "hw/00:26:7b:00:03:d7/udp/192.168.0.1/port/60368");
  ebm_config_if(REMOTE, "hw/ff:ff:ff:ff:ff:ff/udp/192.168.0.2/port/60369");
  ebm_config_meta(80, 0x11, 16, 0x00000000 );
}

int ebm_send_msg (uint32_t WB_Addr_t, uint32_t FID_GID_t, uint64_t Param_t, uint32_t TEF, uint32_t Reserved, uint64_t Timestamp_t)
{
  uint32_t EventID_H = FID_GID_t & 0xffff0000;
  uint32_t EventID_L = 0x00000000;
  uint32_t Param_H   = (uint32_t)((Param_t & 0xffffffff00000000 ) >> 32);
  uint32_t Param_L   = (uint32_t)(Param_t & 0x00000000ffffffff );
  //uint32_t TEF       = 0x11111111;
  //uint32_t Reserved  = 0x22222222;
  uint32_t Timestamp_H = (uint32_t)((Timestamp_t & 0xffffffff00000000 ) >> 32);
  uint32_t Timestamp_L = (uint32_t)(Timestamp_t & 0x00000000ffffffff );
  //creat WB cy (uint32_t)(Param & 0x00000000ffffffff );  cle : WRITE
  //Format of Timing Message : 32 bits WB Addr + 256 bits Payload
  //32 bits WB Addr
  ebm_hi(WB_Addr);
  atomic_on();
  //256 bits Payload
  ebm_op(WB_Addr_t, EventID_H  , Ebm_WRITE);
  ebm_op(WB_Addr_t, EventID_L  , Ebm_WRITE);
  ebm_op(WB_Addr_t, Param_H    , Ebm_WRITE);
  ebm_op(WB_Addr_t, Param_L    , Ebm_WRITE);
  ebm_op(WB_Addr_t, TEF        , Ebm_WRITE);
  ebm_op(WB_Addr_t, Reserved   , Ebm_WRITE);
  ebm_op(WB_Addr_t, Timestamp_H, Ebm_WRITE);
  ebm_op(WB_Addr_t, Timestamp_L, Ebm_WRITE);
  atomic_off();
  ebm_flush();

}

/* interrupt read Param, TEF, Reserved, Timestamp */
//void  Irq_handler ()
//{
//  switch(TGM_tag)
//  {
//    //EVT_START_B2B
//    case 0x0B2B0001:
//    {
//      start_ts     = pECA_Q + Timestamp_os;
//      mprintf ("EVT_START_B2B: start_ts = %x\n", start_ts);break;
//    }
//    //TGM_PHASE_TIME
//    case 0x0B2B0002:
//    {
//      predicted_phase_h1_trg   = pECA_Q + Param_os;
//      mprintf ("TGM_PHASE_TIME: PHASE = %x\n", predicted_phase_h1_trg);break;
//    }
//    //TGM_TRIGGER_KICKER_TS_S
//    case 0x0B2B0004:
//    {
//      trigger_ts_s = pECA_Q + Param_os;
//      extract_ts_s = (pECA_Q + TEF_os) + (pECA_Q + Reserved_os);
//      mprintf ("TGM_TRIGGER_KICKER_TS_S: trigger time = %x; extraction time = %x\n", trigger_ts_s, extract_ts_s);break;
//    }
//    //TGM_TRIGGER_KICKER_TS_T
//    case 0x0B2B0005:
//    {
//      trigger_ts_t = pECA_Q + Param_os;
//      extract_ts_t = (pECA_Q + TEF_os) + (pECA_Q + Reserved_os);
//      mprintf ("TGM_TRIGGER_KICKER_TS_S: trigger time = %x; extraction time = %x\n", trigger_ts_s, extract_ts_s);break;
//    }
//  }
//}

/* send phase correction value to SCU slave: phase correction module */
void write_phase_correction_to_PCM (int slave_nr, int offset, uint16_t phase_correction)
{
  pSCUbm[(slave_nr << 16) + offset] = phase_correction;
  mprintf("write pc offset %x %x \n", pSCUbm[(slave_nr << 16) + offset],&pSCUbm[(slave_nr << 16) + offset]);
}

/* send phase shift value and phase shift restrictions dot(f) 2 dot(f) to SCU slave: phase shift module */
void write_phase_shift_to_PCM (int slave_nr, int offset, uint16_t phase_shift, uint16_t ps_restriction1, uint16_t ps_restriction2)
{
  pSCUbm[(slave_nr << 16) + offset]       = phase_shift;
  //pSCUbm[(slave_nr << 16) + offset + 0x2] = ps_restriction1;
  //pSCUbm[(slave_nr << 16) + offset + 0x4] = ps_restriction2;
  mprintf("write ps offset %x %x \n", pSCUbm[(slave_nr << 16) + offset],&pSCUbm[(slave_nr << 16) + offset]);
}

/* read predicted phase from SCU slave: phase advance prediction module */
void read_predicted_phase_from_PAP (int slave_nr, int offset, uint32_t value)
{
  value = pSCUbm[(slave_nr << 16) + offset];
  mprintf("read PAP %x \n", value);
  return;
}

/* phase shift for high harmonic e.g SIS18 h=2. phase shift range [-180, 180] */
    // Phase shift for the synchronizationc.
    // src    TOF    trg  phase_shift (degree)
    // 180    20     160     0             (1)
    // 180    10     160    -10            (2)
    // 230    10     10     150            (3)
    // 10     10     120    120            (4)
    // 10     10     230    -130           (5)

uint64_t calculate_phase_shift_value (uint64_t phase_high_src, uint64_t phase_high_trg, uint64_t phase_high_tof)
{
  mprintf("/********************Phase Shift****************************/\n");
  uint64_t shift_high_src = 0;
  // Predicted Phase from PAP is 16 bits [0, 360]
  if (phase_high_trg == phase_high_src + 0xFFFF - phase_high_tof || phase_high_trg == phase_high_src - 0xFFFF - phase_high_tof || phase_high_trg == phase_high_src - phase_high_tof)//(1)
  {
    shift_high_src = 0;
  }
  else
  {
    if (phase_high_src > (phase_high_trg + phase_high_tof))
    {
      if ((phase_high_src - phase_high_trg - phase_high_tof) < 0x7FFF )//(2)
        shift_high_src = 0x7FFF - (phase_high_src - (phase_high_trg + phase_high_tof));
      else //(3)
        shift_high_src = 0xFFFF - (phase_high_src - (phase_high_trg + phase_high_tof));

      mprintf ("phase shift for the Group DDS = 0x%x %x",shift_high_src, shift_high_src >>32);
      mprintf (" => -%d(degree) \n",(uint32_t)(0x7FFF-shift_high_src)*180/0x7FFF);
    }
    else
    {
      if ((phase_high_trg - phase_high_src + phase_high_tof) < 0x7FFF ) //(4)
        shift_high_src = phase_high_trg + phase_high_tof - phase_high_src;
      else //(5)
        shift_high_src = 0x7FFF - (0xFFFF - (phase_high_trg + phase_high_tof - phase_high_src));

      mprintf ("phase shift for the Group DDS = 0x%x %x",shift_high_src, shift_high_src >>32);
      mprintf (" => %d(degree) \n",(uint32_t)(shift_high_src*360/0xFFFF));
    }
  }
  return shift_high_src;
}


/* Phase correction for the Phase correction module */
uint64_t calculate_phase_correction_value (uint64_t predicted_phase_h_trg, uint64_t phase_h_tof)
{
  uint64_t phase_correction_h_src = 0;
  mprintf("/********************Phase Correction****************************/\n");
  //phase corrction is triggered at t0 + 1ms + 10 us by BuTiS T0, phase correction [0,360]
  phase_correction_h_src = predicted_phase_h_trg + ((1000000 - 400000 + 10000)*1000 % period_h1_trg)*0xFFFF/ period_h1_trg +  phase_h_tof;
  //mprintf ("SIS18 h=1 period = %x %d(ps); SIS100 h=1 period = 0x%x %d(ps)\n",period_h1_src, period_h1_src,period_h1_trg, period_h1_trg);

  if (phase_correction_h_src > 0xFFFF)
  {
    phase_correction_h_src = phase_correction_h_src - 0xFFFF;
  }
  mprintf ("phase correction for the SR = 0x%x %x ",phase_correction_h_src, phase_correction_h_src>>32);
  mprintf (" =>  %d(degree) \n",(uint32_t)phase_correction_h_src*360/0xFFFF);
  return phase_correction_h_src;
}

/* Calculate the timestamp of the zero-crossing of the high Harmonic h=2(SIS18) &
 * h=10(SIS100), here we assume the phase from PAP is h=1 */
uint64_t* next_high_harmonic_zero_ts (uint64_t predicted_phase_h_src, uint64_t predicted_phase_h_trg, uint64_t predicted_phase_time, uint64_t * tm_high_0_st)
{
  //mprintf("predicted phase timestamp  %x %x\n",predicted_phase_ts,predicted_phase_time >>32);
  mprintf("/********************Next High Zero TS****************************/\n");
  tm_high_0_st[0] = (predicted_phase_time*8*1000 + (period_high_harmonic_src - (predicted_phase_h_src * period_h1_src /0xFFFF) % period_high_harmonic_src))/1000/8;
  tm_high_0_st[1] = (predicted_phase_time*8*1000 + (period_high_harmonic_trg - (predicted_phase_h_trg * period_h1_trg / 0xFFFF) % period_high_harmonic_trg))/1000/8;
  mprintf("tm_high_zero SIS18 0x%x %x\n", tm_high_0_st[0], tm_high_0_st[0] >>32);
  mprintf("tm_high_zero SIS100 0x%x %x\n", tm_high_0_st[1], tm_high_0_st[1] >>32);
  return tm_high_0_st;
}

/* calculate the phase of high harmonic at the phase prediction time */
uint64_t* phase_h1_to_high_calculate (uint64_t predicted_phase_h_src, uint64_t predicted_phase_h_trg, uint64_t * phase_high_st)
{
  mprintf("/********************Phase High****************************/\n");
  //phase uses 16 bit 0x0000FFFF for 360 degree
  //phase_high_harmonic_src = ((predicted_phase_h1_src * period_h1_src/0xFFFF) % period_high_harmonic_src) * 0xFFFF / period_high_harmonic_src;
  phase_high_st[0] = ((predicted_phase_h_src * period_h1_src/0xFFFF) % period_high_harmonic_src) * 0XFFFF / period_high_harmonic_src;
  phase_high_st[1] = ((predicted_phase_h_trg * period_h1_trg/0xFFFF) % period_high_harmonic_trg) * 0xFFFF / period_high_harmonic_trg;

  mprintf("phase h1 => phase high harmonic: SIS18  0x%x %x",phase_high_st[0],phase_high_st[0]>>32);
  mprintf(" => %d(degree)\n",(uint32_t)(phase_high_st[0]*360/0xFFFF));
  mprintf("phase h1 => phase high harmonic: SIS100 0x%x %x",phase_high_st[1],phase_high_st[1]>>32);
  mprintf(" => %d(degree)\n",(uint32_t)(phase_high_st[1]*360/0xFFFF));

  return phase_high_st;
}

/* Synchronization window calculation*/

uint64_t calculate_freq_beating_time (uint64_t tm_high_0_src, uint64_t tm_high_0_trg, uint64_t tof_time)
{
  uint64_t frequency_beat_time;
  uint64_t cycle_num = 0;
  mprintf("/********************Frequency Beating****************************/\n");

  if (tm_high_0_src * 8 *1000 >= (tm_high_0_trg * 8*1000 -tof_time))
    {
      cycle_num = (tm_high_0_src*8*1000 + tof_time - tm_high_0_trg*8*1000)/(period_high_harmonic_trg - period_high_harmonic_src);
    }
  else
    {
      cycle_num = (tm_high_0_src*8*1000 + tof_time - tm_high_0_trg*8*1000 + period_high_harmonic_trg) / (period_high_harmonic_trg - period_high_harmonic_src);
    }

  frequency_beat_time = cycle_num * period_high_harmonic_src;
  mprintf("The number of SIS18 for the synchronization = 0x%x %x",cycle_num,cycle_num >>32);
  mprintf(" =>  %d\n",(uint32_t)cycle_num);
  mprintf("frequency beating time = 0x%x %x", frequency_beat_time, frequency_beat_time>>32);
  mprintf(" => %d (ns) %d(ms)\n", (uint32_t)frequency_beat_time/1000, (uint32_t)frequency_beat_time/1000000000);
  return frequency_beat_time;
}
/* The uncertainty introduced by the predicted phase, the DDS rf frequency
 * uncertainty is negligible */
uint32_t calculate_synch_window_uncertainty(uint64_t predicted_phase_uncertainty)
{
  uint64_t synch_window_uncertainty = 0;
  //us the uncertity squre "sqrt" does not work properly
  //synch_window_uncertainty = sqrt((pow(freq_high_harmonic_src,2)+pow(freq_high_harmonic_trg,2))*pow(predicted_phase_uncertainty,2)/pow((freq_high_harmonic_src-freq_high_harmonic_trg),2))/1000000;
  synch_window_uncertainty = ((pow(freq_high_harmonic_src,2)+pow(freq_high_harmonic_trg,2))*pow(predicted_phase_uncertainty,2)/pow((freq_high_harmonic_src-freq_high_harmonic_trg),2))/1000000;
  mprintf("%%%%%%%%%%%%%% %x %x",synch_window_uncertainty,synch_window_uncertainty>>32);
  return synch_window_uncertainty;
}

/* Function main */
int main (void)
{
  uint32_t fre_src;
  uint32_t fre_trg;
  uint32_t phase_shift_time;
  sdb_location found_sdb[20];
  uint32_t lm32_endp_idx = 0;
  uint32_t clu_cb_idx = 0;
  uint64_t phase_h1_tof;
  uint64_t phase_high_harmonic_tof;
  uint64_t * phase_high_h_st;
  uint64_t * tm_high_0_st;
  uint32_t synchronization_window_uncertainty;
  uint64_t frequency_beating_time;

  //source and target machine rf period with cavity harmonics
  period_high_harmonic_src = SCALE/freq_high_harmonic_src;
  period_high_harmonic_trg = SCALE/freq_high_harmonic_trg;
  //source and target machine rf requency with h=1
  period_h1_src = SCALE/freq_high_harmonic_src * harmonic_src;
  period_h1_trg = SCALE/freq_high_harmonic_trg * harmonic_trg;

  phase_h1_tof = tof_time % period_h1_src * 0xFFFF / period_h1_src;
  phase_high_harmonic_tof = tof_time % period_high_harmonic_src * 0xFFFF / period_high_harmonic_src;;

  mprintf ("SIS18  h=1 period  =  %d(ps)\n",(uint32_t)period_h1_src);
  mprintf ("SIS100 h=1 period  =  %d(ps)\n",(uint32_t)period_h1_trg);
  mprintf ("SIS18  high harmonic period = %d(ps)\n",(uint32_t)period_high_harmonic_src);
  mprintf ("SIS100 high harmonic period = %d(ps)\n",(uint32_t)period_high_harmonic_trg);
  init();
  //SM_param_load(fre_src,fre_trg,har_src,har_trg,phase_shift_time, tof_time, kicker_delay, rst_fmax1, rst_fmax2);

  /* Base address of the related ep cores */
  pSCUbm =(unsigned short*)find_device_adr(venID,devID_SCUBM);
  pEBm = (unsigned int*)find_device_adr(venID,devID_EBM);
  pECA = (unsigned int*)find_device_adr(venID,devID_ECA);
  pTLU = (unsigned int*)find_device_adr(venID,devID_TLU);

  find_device_multi(&found_sdb[0], &clu_cb_idx, 20, GSI, LM32_CB_CLUSTER); // find location of cluster crossbar
  find_device_multi_in_subtree(&found_sdb[0], lm32_irq_endp, &lm32_endp_idx, 10, GSI, LM32_IRQ_EP); // list irq endpoints in cluster crossbar
  for (int i=0; i < lm32_endp_idx; i++)
  {
    mprintf("irq_endp[%d] is: 0x%x\n",i, getSdbAdr(&lm32_irq_endp[i]));
  }

  //mprintf("base address %x %x\n",pSCUbm,pEBm);

  /* Function Generator simulates the zero crossing point of high harmonic. TLU gets the timestamp of two TTL signals */
  get_rising_edge_tm_from_FG ();

  //Here we assume the predicted phase is half rf period of the FG measured timestamp.
  //FG measured ts is the 3rd rf zero crossing pointd within h=1 frequency.
  //SIS100 h=1 is 54 degree (0x2666)(predicted phase) <=> t0+400us;
  //h=2 of SIS18 270 degree (0XC000), h=10 of SIS100 180 degree(0x7FFF)
  //Phase shift is -80 degree; phase correction is 19 degree

  phase_correction_h1_src = (uint16_t) calculate_phase_correction_value (0x2666, 0x4B);
  //phase_correction_h1_src = (uint16_t) calculate_phase_correction_value ((uint64_t) predicted_phase_h1_trg, (uint64_t) phase_h1_tof);
  mprintf(">>>>>>>>>>>>>>Phase correction : 0x%x\n",phase_correction_h1_src);
  //phase h1 => phase at high harmonic
  phase_h1_to_high_calculate (0xE000,0x2666,phase_high_h_st);
  phase_high_harmonic_src = phase_high_h_st[0];
  phase_high_harmonic_trg = phase_high_h_st[1];
  mprintf(">>>>>>>>>>>>>>>The phase_h1_to_high_calculate output: SIS18 0x%x SIS100 0x%x\n", phase_high_harmonic_src,phase_high_harmonic_trg);
  //phase h1 => timestamp of the next zero crossing of high harmonic
  next_high_harmonic_zero_ts (0xE000, 0x2666, tm_high_zero_trg - period_high_harmonic_trg/2/8/1000, tm_high_0_st);
  tm_high_zero_src = tm_high_0_st[0];
  tm_high_zero_trg = tm_high_0_st[1];
  mprintf(">>>>>>>>>>>>>>>The next_high_harmonic_zero_ts output: SIS18 0x%x %x\n", tm_high_zero_src, tm_high_zero_src>>32);
  mprintf(">>>>>>>>>>>>>>>The next_high_harmonic_zero_ts output: SIS100 0x%x %x\n",tm_high_zero_trg, tm_high_zero_trg>>32);

  phase_shift_high_src =(uint16_t)calculate_phase_shift_value (0xC000, 0x7FFF, 0x71C);
  //phase_shift_high_src = (uint16_t)calculate_phase_shift_value ((uint64_t)phase_high_harmonic_src, (uint64_t)phase_high_harmonic_trg, (uint64_t)phase_high_harmonic_tof);
  mprintf(">>>>>>>>>>>>>>>Phase shift : 0x%x\n",phase_shift_high_src);

  //the distance between SIS18 and SIS100 is 300m. For U28+ is about 1760ns
  frequency_beating_time = calculate_freq_beating_time (tm_high_zero_src, tm_high_zero_trg, 0x1ADB00);
  mprintf(">>>>>>>>>>>>>>>frequency beating time : 0x%x %x\n",frequency_beating_time, frequency_beating_time >>32);
  // the uncertainty is 250 ps => 0xFA
  synchronization_window_uncertainty = calculate_synch_window_uncertainty(0xFA);
  mprintf(">>>>>>>>>>>>>>>synchronization window uncertainty : %d (us^2)\n",(uint32_t)synchronization_window_uncertainty/1000000);
  /* Timestamp corresponds to the predicted phase t0+100us */
  //predict_phase_ts = EVT_B2B_START_ts + 100000;

  /* Read the predicted phase from the PAP module: + 100us */
 // while ()
 // {
 //   //waiting for interrupt ???
 //   if process_ID = EVT_START_B2B_tag
 //     read_SCUs_param(2, OS_PAP, predicted_phase_h1_src);
 //     break;
 //   end if;
 // }

 // /* Read the predicted phase of the target machine from the ECA: + 1ms */
 // while ()
 // {
 //   //waiting for interrupt ???
 //   if process_ID = TGM_PHASE_TIME_tag
 //     read_ECA(predicted_phase_h1_trg);
 //     break;
 //   end if;
 // }

 // /* Phase shift */
 // if (choose_phase_shift_method == 1)
 // {
 //   int phase_shift_high_src = 0;

 //   //set phase shift value to the PS module via SCU bus
 //   calculate_phase_shift_value();
 //   write_SCUs_param(2, OS_PS, phase_shift_high_src);

 //   syn_win_start_tm =  EVT_B2B_START_ts + phase_shift_time;
 // }

 // /* Frequency beating */
 // if (chose_frequency_beating_method == 1)
 // {
 //   calculate_freq_beating_time (fre_src, fre_trg, predicted_phase_h1_src, predicted_phase_h1_trg, EVT_B2B_START_tm);
 //   syn_win_start_tm = EVT_B2B_START_ts + frequency_beating_time;
 // }
 // //send the TGM_SYNCH_WIN
 // ebm_send_msg(WB_Addr, TGM_SYNCH_WIN_eid, TGM_SYNCH_WIN_tag, syn_win);
 // /* Phase correction to the B2B SCU slave ???, the trigger is based on
 //  * EVT_B2B_START +2ms */
 // phase_correction();
 // write_SCUs_param(2, OS_PC, phase_correction_h1_src);


  //  write_SCUs_param(2, OS_PS, );

  ///* Send  */
  //ebm_send_msg(WB_Addr, FID_GID, Param, Timestamp);

  write_phase_correction_to_PCM(2, OS_PC, 0xbab0);
  read_predicted_phase_from_PAP(2, OS_PC, fre_src);
  //v_PC - OS_PC;
  ////mprintf("--------%x \n",v_PC);
  //ebm_send_msg(WB_Addr, FID_GID, Param, Timestamp);


}



