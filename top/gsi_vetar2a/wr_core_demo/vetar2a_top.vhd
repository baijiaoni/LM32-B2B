library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.monster_pkg.all;

entity vetar2a_top is
  port(
    clk_20m_vcxo_i    : in std_logic; -- N3          --  20 MHz WR VCXO clock
    clk_125m_pllref_i : in std_logic; -- AE15p AF15n -- 125 MHz WR PLL reference
    clk_125m_local_i  : in std_logic; -- D14p  C14n  -- 125 MHz local oscillator (CKPLL_125P)
    clk_pll_o         : out std_logic;-- V24p  U24p  -- clock pll output (lvds_buf3)
        
    -----------------------------------------------------------------------
    -- OneWire 3.3V
    -----------------------------------------------------------------------
    rom_data_io : inout std_logic; -- E3
    
    -----------------------------------------
    -- Timing SFPs 3.3v
    -----------------------------------------
    sfp_ref_clk_i  : in    std_logic; -- AE27p AE28n
    sfp_td_o       : out   std_logic; -- AE24p AF24n
    sfp_rd_i       : in    std_logic; -- AG23p AH23n
    sfp_tx_fault_i : in    std_logic; -- J4
    sfp_los_i      : in    std_logic; -- J5
    sfp_tx_dis_o   : out   std_logic; -- K9
    sfp_mod0_i     : in    std_logic; -- K7
    sfp_mod1_io    : inout std_logic; -- J8
    sfp_mod2_io    : inout std_logic; -- K8

    ------------------------------------------------------------------------
    -- WR DAC signals 3.3V
    ------------------------------------------------------------------------
    dac_sclk_o      : out std_logic; -- T1
    dac_din_o       : out std_logic; -- P3
    ndac_cs_o       : out std_logic_vector(2 downto 1);
    -- ndac_cs_o(1) -- AG1
    -- ndac_cs_o(2) -- AF1
    
    -----------------------------------------
    -- Logic analyzer HPLA1 2.5V
    -----------------------------------------
    hpw_io : inout std_logic_vector(15 downto 0);
    -- hpw_io( 0) -- AB16
    -- hpw_io( 1) -- AC17
    -- hpw_io( 2) -- AC16
    -- hpw_io( 3) -- AE17
    -- hpw_io( 4) -- AF17
    -- hpw_io( 5) -- AB17
    -- hpw_io( 6) -- AD18
    -- hpw_io( 7) -- AF19
    -- hpw_io( 8) -- AF20
    -- hpw_io( 9) -- AE19
    -- hpw_io(10) -- AE20
    -- hpw_io(11) -- AE21
    -- hpw_io(12) -- AC22
    -- hpw_io(13) -- AE22
    -- hpw_io(14) -- AC23
    -- hpw_io(15) -- AC18
    
    -----------------------------------------
    -- LEDs on baseboard 2.5V
    -----------------------------------------
    -- CHANGE FOR LED_O
    leds_o : out std_logic_vector(15 downto 0);
    -- leds_o(0) --      -- leds_o(1) -- -- AH15 - Y19
    -- leds_o(2) --      -- leds_o(3) -- -- AH18 - AG18
    -- leds_o(4) --      -- leds_o(5) -- -- AH19 - AG19
    -- leds_o(6) --      -- leds_o(7) -- -- AD21 - AD22
    -- leds_o(8) --      -- leds_o(9) -- -- AD23 - AD24
    -- leds_o(10)--      -- leds_o(11)-- -- AC24 - AC21
    -- leds_o(12)--      -- leds_o(13)-- -- Y20  - Y22
    -- leds_o(14)--      -- leds_o(15)-- -- W21  - V23

    -----------------------------------------
    -- USB micro controller 3.3V
    -----------------------------------------
    --pres_o  is '0', it is by design 
    sres_o    : out   std_logic; -- AB8 - active low reset#
    slrdn_o   : out   std_logic; -- AC10 - read strobe
    slwrn_o   : out   std_logic; -- AB9 - write strobe
    speed_i   : in    std_logic; -- PA0 = AF8
    shift_i   : in    std_logic; -- PA1 = AE8
    sloen_o   : out   std_logic; -- PA2 = W11
    ebcyc_i   : in    std_logic; -- PA3 = W12
    fifoadr_o : out   std_logic_vector(1 downto 0); -- 0=PA4=AC12, 1=PA5=W13
    pktendn_o : out   std_logic; -- PA6 = Y12
    readyn_io : inout std_logic; -- PA7 = AD12
    fulln_i   : in    std_logic; -- CTL1 = AA9
    emptyn_i  : in    std_logic; -- CTL2 = AB10
    fd_io     : inout std_logic_vector(7 downto 0); -- FIFO bus
                                                    -- AH2,AA10,AC6,AH3,
                                                    -- Y10,AD6,W10,Y1

    -----------------------------------------------------------------------
    -- RAM
    -----------------------------------------------------------------------
--    ram_gw      : in    std_logic;                          -- Synchronous Global Write Enable
--    ram_bwe     : out   std_logic;                          -- Synchronous Byte Write Enable
--    ram_bwx     : out   std_logic_vector(3 downto 0);       -- Synchronous Byte Write Enable
--    ram_oe      : out   std_logic;                          -- Output Enable
--    ram_ce      : out   std_logic_vector(1 downto 0);       -- Synchronous Chip Enable
--    ram_adv     : out   std_logic;                          -- Synchronous Burst Write Enable
--    ram_adsc    : out   std_logic;                          -- Synchronous Controller Address Status
--    ram_adsp    : out   std_logic;                           -- Synchronous Processor Address Status
--    ram_address : in    std_logic_vector(18 downto 0);
--    ram_data    : inout std_logic_vector(31 downto 0);
--    ram_clk     : out   std_logic;

    -----------------------------------------------------------------------
    -- Display
    -----------------------------------------------------------------------	
   di_o : out std_logic_vector(3 downto 0); 
    -- di[0] AH7-DIS0 DIN  shift register in ?????
    -- di[0] Y14-DIS0 DOUT shift register out?????
    -- di[1] AD7-DIS1 LP   latch pulse (end-of-40-bit-row)
    -- di[2] AH8-DIS2 FLM  first-line marker               
    -- di[3] AC7-DIS3 SCP  clock                              

    -- di_i : in std_logic;
    -- di_i  AH7-DIS0 DIN  shift register in????

    color_o : out std_logic_vector(2 downto 0);
    -- color[0] AH13-BLU Blue
    -- color[1] AH5-DIS4 Green                                           
    -- color[2] AH4-DIS5 Red                                           
   
    -----------------------------------------------------------------------
    -- VME bus
    -----------------------------------------------------------------------
    vme_as_n_i          : in    std_logic;   -- M3               
    vme_rst_n_i         : in    std_logic;   -- J1              
    vme_write_n_i       : in    std_logic;   -- M4               
    vme_am_i            : in    std_logic_vector(5 downto 0);    -- 5=V3, 4=Y1, 3=AA1, 2=AD1, 1=AE1, 0=Y4
    vme_ds_n_i          : in    std_logic_vector(1 downto 0);    -- 1=L3, 0=M2
    vme_ga_i            : in    std_logic_vector(3 downto 0);    -- 3=U4, 2=w3, 1=W2, 0=V4 -- MON3..MON0
    vme_addr_data_b     : inout std_logic_vector(31 downto 0);   -- 31=L6, 30=M5, 29=P1,  28=R1,  27=M6, 26=N6, 25=T4,  24=U3
                                                                 -- 23=P6, 22=R6, 21=V1,  20=W1,  19=P5, 18=N4, 17=AB1, 16=AC1
                                                                 -- 15=T7, 14=T6, 13=AB3, 12=AB2, 11=U6, 10=V6, 9=AC3,  8=AC22
                                                                 -- 7=V7,  6=W6,  5=AB4,  4=AC4,  3=Y9,  2=W8,  1=AB7,  0=AA6
    vme_iackin_n_i      : in    std_logic;   -- H1               
    vme_iackout_n_o     : out   std_logic;   -- E1               
    vme_iack_n_i        : in    std_logic;   -- K3               
    vme_irq_n_o         : out   std_logic_vector(6 downto 0);    -- 6=W4, 5=AA4, 4=AA3, 3=AD3, 2=AE3, 1=Y6, 0=Y5
    vme_berr_o          : out   std_logic;   -- D1               
    vme_dtack_oe_o      : out   std_logic;   -- L4               
    vme_buffer_latch_o  : out   std_logic_vector(3 downto 0);    -- 3=F1(LDIV), 2=G1(LDVI), 1=J7(LAIV), 0=J6(LAVI)
    vme_data_oe_ab_o    : out   std_logic;   -- L7               -- ODVI
    vme_data_oe_ba_o    : out   std_logic;   -- J3               -- CDVI
    vme_addr_oe_ab_o    : out   std_logic;   -- E4               -- OAIV
    vme_addr_oe_ba_o    : out   std_logic;   -- L1               -- CAIV
    
    -----------------------------------------
    -- LEMO on front panel NIM/TTL
    -----------------------------------------
    lemo_i            : in std_logic;   -- K4
    lemo_o            : out std_logic;  -- H4
    lemo_o_en_o       : out std_logic;  -- H3
    lemo_i_en_o       : out std_logic;  -- C1

    -----------------------------------------
    -- VETAR1DB2a ADD-ON Board 
    -----------------------------------------
   
    -- LVDS
    lvds_in_i : in  std_logic_vector(1 downto 0);
    --lvds_in_i[0] PG1P12 5 G13
    --lvds_in_i[1] PG1N12 7 F13
 
    lvds_out_o : out std_logic_vector(1 downto 0);
    --lvds_out_o[0] PG1P13 11  E12
    --lvds_out_o[1] PG1N13 13  D12

    -- HDMI
    -- hdmi_o : out std_logic_vector(1 downto 0);
    hdmi_o : out std_logic;
    -- hdmi_o => B3 => PG2P14 => 85 A3 => PG2P15 => 87

    hdmi_i : in  std_logic_vector(1 downto 0);
    -- hdmi_i[0]    F10  PG2P15-91   E10  PG2N15-93
    -- hdmi_i[1]    F9   PG2P16-85   F8   PG2N16-87

    -- NIM/TTL LEMOs 1 and 2
    lemo_nim_ttl_i : in  std_logic_vector(1  downto 0);
    -- lemo_nim_ttl_i[0] 5-PG1P1 E6 DON'T DRIVE FAST, CLOSE TO PLL!!
    -- lemo_nim_ttl_i[1] 7-PG1N1 D5 DON'T DRIVE FAST, CLOSE TO PLL!!

    -- Only Output LEMOs 3-5
    lemo_addOn_o : out std_logic_vector(2 downto 0);
    -- lemo_addOn_o[0] PG1P9  - 55 Y18
    -- lemo_addOn_o[1] PG1N9  - 57 AA19
    -- lemo_addOn_o[2] PG1P10 - 61 AA15
    
    lemo_addOn_eo_o : out std_logic;
    -- lemo_eo_o   LEN - 47 G11

    -- I/O LEMOs 6-8
    lemo_addOn_io_o  : out std_logic_vector(2 downto 0);
    lemo_addOn_io_i  : in  std_logic_vector(2 downto 0);
 
    -- lemo_addOn_io_X[0]  output P_LVDS_5/N_LVDS_5 11/13 - A5/A4
    --                     input  P_LVDS_6/N_LVDS_6 17/19 - D10/C10
    
    -- lemo_addOn_io_X[1]  output P_LVDS_7/N_LVDS_7 23/25 - E9/D9
    --                     input  P_LVDS_8/N_LVDS_8 29/31 - H10/G10
    
    -- lemo_addOn_io_X[2]  output P_LVDS_9/N_LVDS_9 35/37 - K11/J10
    --                     input  P_LVDS_10/N_LVDS_10 41/43 - J12/J11
   
    lemo_addOn_term_o  : out std_logic_vector(2 downto 0) := (others => 'Z'); 
    -- lemo_addOn_term_o  TERMEN1/TERMEN2/TERMEN3 67/69/73 Y16/AA16/AH16
  
    lemo_addOn_oen_o   : out std_logic_vector(2 downto 0) := (others => 'Z');
    -- lemo_addOn_eo_o   TTLEN1/TTLEN2/TTLEN3 75/79/81  AH17/AE18/AF18
                                                                                
    -- ROM 
    rom_addOn_io  : inout std_logic := 'Z';
    --rom_add_on_io  ROM_DATA-37 B9

    -- LEDS
    leds_lemo_addOn_o	: out std_logic_vector(2 downto 0);
    -- leds_lemo_addOn_o[0] E7  lemo_lemo_addOn_o[1] D7 lemo_lemo_addOn_o[2] C7
    -- leds_lemo_addOn_o[2 downto 0] DON'T DRIVE FAST, CLOSE TO PLL!!!
    leds_lemo_io_on_o	: out std_logic_vector(2 downto 0);
    -- leds_lemo_io_on_o[0] C6  leds_lemo_io_on_o[1] B6 leds_lemo_io_on_o[2] A6

    leds_lemo_io_off_o	: out std_logic_vector(2 downto 0);
    -- leds_lemo_io_off_o[2] D6 leds_lemo_io_off_o[1] C5  leds_lemo_io_off_o[0] D4
    
    led_lemo_term_o     : out std_logic);
    -- led_lemo_term_o C4 

end vetar2a_top;

architecture rtl of vetar2a_top is
  
  signal s_led_link_up     : std_logic;
  signal s_led_link_act    : std_logic;
  signal s_led_track       : std_logic;
  signal s_led_pps         : std_logic;
  
  signal s_clk_ref         : std_logic;
  signal s_clk_butis       : std_logic;
  signal s_butis_t0        : std_logic;
  signal s_dedicated_out   : std_logic;

  signal s_lemo_addOn      : std_logic_vector(2 downto 0);
  signal s_lemo_oen        : std_logic_vector(2 downto 0);
  signal s_lemo_addOn_io   : std_logic_vector(2 downto 0);
  signal s_leds_lemo_addOn : std_logic_vector(2 downto 0);
  
  signal s_di_scp          : std_logic;
  signal s_di_flm          : std_logic;
  signal s_di_lp           : std_logic; 
  signal s_di_dat          : std_logic;

  constant c_black         : std_logic_vector := "111";
  constant c_red           : std_logic_vector := "101";
  constant c_green         : std_logic_vector := "110";
  constant c_blue          : std_logic_vector := "011";
  
begin

  main : monster
    generic map(
      g_family     => "Arria II",
      g_project    => "vetar_top2a",
      g_gpio_inout => 3,
      g_gpio_in    => 10,
      g_gpio_out   => 10,
      -- g_gpio_out   => 7, for CSEE special (butis_t0 and 200Mhz on LVDS and HDMK)
      g_flash_bits => 24,
      g_en_vme     => true,
      g_en_usb     => true,
      g_en_lcd     => true)
    port map(
      core_clk_20m_vcxo_i    => clk_20m_vcxo_i,
      core_clk_125m_pllref_i => clk_125m_pllref_i,
      core_clk_125m_sfpref_i => sfp_ref_clk_i,
      core_clk_125m_local_i  => clk_125m_local_i,
      core_clk_wr_ref_o      => s_clk_ref,
      core_clk_butis_o       => s_clk_butis,
      core_clk_butis_t0_o    => s_butis_t0,
      -- gpio oe
      gpio_oen_o(2 downto 0) => s_lemo_oen(2 downto 0),
      -- gpio out
      gpio_o(1 downto 0)     => lvds_out_o(1 downto 0),
      gpio_o(2)              => lemo_o,
      gpio_o(5 downto 3)     => s_lemo_addOn(2 downto 0),
      gpio_o(8 downto 6)     => s_lemo_addOn_io(2 downto 0),
      gpio_o(9)              => hdmi_o,
      -- gpio in
      gpio_i(1 downto 0)     => lvds_in_i(1 downto 0),
      gpio_i(2)              => lemo_i,
      gpio_i(5 downto 3)     => lemo_addOn_io_i(2 downto 0),
      gpio_i(7 downto 6)     => hdmi_i(1 downto 0),
      gpio_i(9 downto 8)     => lemo_nim_ttl_i(1 downto 0),
      -- gpio out CSEE special
      --gpio_o(0)              => lemo_o,
      --gpio_o(3 downto 1)     => s_lemo_addOn(2 downto 0),
      --gpio_o(6 downto 4)     => s_lemo_addOn_io(2 downto 0),
      -- wr core
      wr_onewire_io          => rom_data_io,
      wr_sfp_sda_io          => sfp_mod2_io,
      wr_sfp_scl_io          => sfp_mod1_io,
      wr_sfp_det_i           => sfp_mod0_i,
      wr_sfp_tx_o            => sfp_td_o,
      wr_sfp_rx_i            => sfp_rd_i,
      wr_dac_sclk_o          => dac_sclk_o,
      wr_dac_din_o           => dac_din_o,
      wr_ndac_cs_o           => ndac_cs_o,
      wr_ext_clk_i           => lemo_nim_ttl_i(1),
      wr_ext_pps_i           => lemo_nim_ttl_i(0),
      led_link_up_o          => s_led_link_up,
      led_link_act_o         => s_led_link_act,
      led_track_o            => s_led_track,
      led_pps_o              => s_led_pps,
      -- vme
      vme_as_n_i             => vme_as_n_i,
      vme_rst_n_i            => vme_rst_n_i,
      vme_write_n_i          => vme_write_n_i,
      vme_am_i               => vme_am_i,
      vme_ds_n_i             => vme_ds_n_i,
      vme_ga_i               => vme_ga_i,
      vme_addr_data_b        => vme_addr_data_b,
      vme_iack_n_i           => vme_iack_n_i,
      vme_iackin_n_i         => vme_iackin_n_i,
      vme_iackout_n_o        => vme_iackout_n_o,
      vme_irq_n_o            => vme_irq_n_o,
      vme_berr_o             => vme_berr_o,
      vme_dtack_oe_o         => vme_dtack_oe_o,
      vme_buffer_latch_o     => vme_buffer_latch_o,
      vme_data_oe_ab_o       => vme_data_oe_ab_o,
      vme_data_oe_ba_o       => vme_data_oe_ba_o,
      vme_addr_oe_ab_o       => vme_addr_oe_ab_o,
      vme_addr_oe_ba_o       => vme_addr_oe_ba_o,
      -- usb
      usb_rstn_o             => sres_o,
      usb_ebcyc_i            => ebcyc_i,
      usb_speed_i            => speed_i,
      usb_shift_i            => shift_i,
      usb_readyn_io          => readyn_io,
      usb_fifoadr_o          => fifoadr_o,
      usb_sloen_o            => sloen_o,
      usb_fulln_i            => fulln_i,
      usb_emptyn_i           => emptyn_i,
      usb_slrdn_o            => slrdn_o,
      usb_slwrn_o            => slwrn_o,
      usb_pktendn_o          => pktendn_o,
      usb_fd_io              => fd_io,
      -- lcd
      lcd_scp_o              => s_di_scp,
      lcd_lp_o               => s_di_lp,
      lcd_flm_o              => s_di_flm,
      lcd_in_o               => s_di_dat);

  -- SFP
  ----------------
  sfp_tx_dis_o <= '0'; -- enable SFP
  
  -- Baseboard logic analyzer
  ------------------------------
  hpw_io(15 downto 0) <= (others => 'Z');

  -- Display
  ----------------
  di_o(3) <= '0' when (s_di_scp = '0') else 'Z'; -- clock (run at 2MHz)                            
  di_o(2) <= '0' when (s_di_flm = '0') else 'Z'; -- first-line marker
  di_o(1) <= '0' when (s_di_lp  = '0') else 'Z'; -- latch pulse (end-of-40-bit-row)  
  di_o(0) <= '0' when (s_di_dat = '0') else 'Z'; -- shift register in
  
  -- red=nolink, blue=link+notrack, green=track
  color_o <= 
  c_red   when (not s_led_link_up)                ='1' else
  c_blue  when (s_led_link_up and not s_led_track)='1' else
  c_green when (s_led_link_up and     s_led_track)='1' else
  c_black;          

  -- On board leds
  -----------------
  
  -- Link Activity
  --!!! DON'T USE leds_o(0) leds_o(1) are too close to
  -- a clock and they can be harmfull
  leds_o(15) <= not (s_led_link_act and s_led_link_up); -- Link active
  leds_o(14) <= not s_led_link_up;                      -- Link up
  leds_o(13) <= not s_led_track;                        -- Timing Valid
  leds_o(12) <= not s_led_pps;
  
  -- not assigned leds
  leds_o(11 downto 0)	<= (others => '1'); -- power off
  
  -- On board lemo
  ---------------- 
  lemo_i_en_o <= '1';
  lemo_o_en_o <= '1';

  -- VETAR1DB1 ADD-ON Board
  -------------------------
  -- LEMO outputs GPIO0-5
  lemo_addOn_o      <= s_lemo_addOn;
  lemo_addOn_io_o   <= s_lemo_addOn_io;
  leds_lemo_addOn_o <= s_leds_lemo_addOn;
  lemo_addOn_eo_o   <= '0';

  -- PLL
  clk_pll_o <= s_clk_butis;
  
  -- HDMI CSEE special
  --hdmi_o    <= s_butis_t0;
  
  -- LVDS CSEE special
  --lvds_out_o(0) <= s_clk_butis;
  --lvds_out_o(1) <= s_butis_t0;
  
  -- OE and TERM for LEMOs (s_lemo_oen is driven by monster iodir hack)
  lemo_addOn_oen_o(0)  <= '0' when s_lemo_oen(0)='0' else 'Z'; -- TTLIO1 output enable
  lemo_addOn_oen_o(1)  <= '0' when s_lemo_oen(1)='0' else 'Z'; -- TTLIO2 output enable
  lemo_addOn_oen_o(2)  <= '0' when s_lemo_oen(2)='0' else 'Z'; -- TTLIO3 output enable

  lemo_addOn_term_o(0) <= '1' when s_lemo_oen(0)='1' else '0'; -- TERMEN1 (terminate when input)
  lemo_addOn_term_o(1) <= '1' when s_lemo_oen(1)='1' else '0'; -- TERMEN2 (terminate when input)
  lemo_addOn_term_o(2) <= '1' when s_lemo_oen(2)='1' else '0'; -- TERMEN3 (terminate when input)

  led_lemo_term_o <= not(s_led_pps); -- TBD: This is DAK1 on the addon board
  
  -- INOUT LEMOs
  -- Red => Output enable LEDs
  leds_lemo_io_off_o(2 downto 0) <= s_lemo_oen(2 downto 0);
  -- Green => Activity LEDs
  leds_lemo_io_on_o(2 downto 0) <= not(s_lemo_addOn_io(2 downto 0) or lemo_addOn_io_i(2 downto 0));
  
  -- OUTPUT LEMOs
  -- Orange => Activity LEDs
  s_leds_lemo_addOn(2 downto 0) <= not(s_lemo_addOn(2 downto 0));
  
end rtl;
