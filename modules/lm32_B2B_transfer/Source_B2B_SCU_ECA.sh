# ECA channel 0 => LED
# ECA channel 1 => SCU bus
# ECA channel 2 => LEMO
# ECA channel 3 => LM32
EVT_START_B2B= "0xB001000000000400"
Source_B2B_ECA="dev/ttyUSB0"
# Configure ECA for the Source B2B SCU
eca-ctl $Source_B2B_ECA enable # Enable
eca-ctl $Source_B2B_ECA idisable # Disable interrupts
eca-table $Source_B2B_ECA flush # Flush old stuff

eca-ctl $Source_B2B_ECA activate -c 1
eca-ctl $Source_B2B_ECA activate -c 3

# SCU bus: receive EVT_START_B2B to trigger the phase prediction => Base time 
# Base time must be align with the BuTiS T0 edege
eca-table $Source_B2B_ECA add $EVT_START_B2B/64 +0.0 1 0x0B2B0001
# LM32: receive EVT_START_B2B and read the predicted phase +100us
eca-table $Source_B2B_ECA add $EVT_START_B2B/64 +0.0001 3 0x0B2B0001

# LM32: receive TGM_PHASE_TIME +1ms
eca-table $Source_B2B_ECA add $TGM_PHASE_TIME/64 +0.0 3 0x0B2B0002

eca-table $Source_B2B_ECA add $TGM_PHASE_TIME/64 +0.001 3 0x0B2B0002
eca-table $Source_B2B_ECA add $TGM_PHASE_TIME/64 +0.001 3 0x0B2B0002
eca-table $Source_B2B_ECA add $TGM_PHASE_TIME/64 +0.001 3 0x0B2B0002
eca-table $Source_B2B_ECA add $TGM_PHASE_TIME/64 +0.001 3 0x0B2B0002




eca-table $Source_B2B_ECA flip-active
