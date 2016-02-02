ssh root@scuxl0041.acc.gsi.de
Pw: geheim

branch: network_monitor_asterisk_test_pps_out
bel_projects/modules/syncmon/scripts/configure-data-master.sh

config inst 1
config hw 0

# simulation DM to produce the EVT_B2B_BEGIN
~/test/bel_projects_dm/bel_projects/modules/DM_B2B$ 

eb-ls tcp/scuxl0041.acc.gsi.de

SCU: three lm32  
lm32 inst 0(ftm_lm32:\G1:0) => FG; LM32-IRQ-EP 20800 20900 20a00 (not aviable)
lm32 inst 1(ftm_lm32:\G1:1) => user(B2B); LM32-IRQ-EP 20b00 (irq_slave_i[0])
20b00(irq_slave_i[1]) 20c00(irq_slave_i[2] not aviable) 
lm32 inst 2 => PTP core

B2B_main.c.int => LM32 intrupt is OK
Need to be done:(ECA updates...)
ECA produces intrupt for LM32 and LM32 reads data from ECA Queue

/* TLU */ there is one FIFO for channel 0 and other FIFO for channel 1
TLU_CLEAR 0x3 //clear channel 0 and channel 1, they start to timestamp at same
time 
TLU_CH_SELECT 0x0 // select channel 0
TLU_CH_TIME1 
TLU_CH_TIME0 // read timestamp 
TLU_POP //channel 0 pop the timestamp out, the number of data in FIFO 0 minus 1

TLU_CH_SELECT 0x1 // select channel 1
TLU_CH_TIME1 
TLU_CH_TIME0 // read timestamp 
TLU_POP //channel 1 pop the timestamp out, the number of data in FIFO 1 minus 1

For SCU B2 input => output: modify scu_control.vhd gpio_i(1) lemo_io 


