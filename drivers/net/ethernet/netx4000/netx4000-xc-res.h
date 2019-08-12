
#ifndef NETX4000_XC_RES_H
#define NETX4000_XC_RES_H

#include "ethmac_xpec_regdef.h"

/* ===================================================================== */

/* AREA xmac */
/* Area of xc0_xmac0_regs, xc0_xmac1_regs, xc1_xmac0_regs, xc1_xmac1_regs */

/* ===================================================================== */

#define Addr_NX4000_xc0_xmac0_regs 0xF4022800U
#define NX4000_NETX_XC0_XM0_BASE   0xF4022800U
#define Addr_NX4000_xc0_xmac1_regs 0xF4022A00U
#define NX4000_NETX_XC0_XM1_BASE   0xF4022A00U
#define Addr_NX4000_xc1_xmac0_regs 0xF4022C00U
#define NX4000_NETX_XC1_XM0_BASE   0xF4022C00U
#define Addr_NX4000_xc1_xmac1_regs 0xF4022E00U
#define NX4000_NETX_XC1_XM1_BASE   0xF4022E00U


/* ===================================================================== */

/* AREA xpec */
/* Area of xc0_rpec0_regs, xc0_tpec0_regs, xc0_rpec1_regs, xc0_tpec1_regs, xc1_rpec0_regs, xc1_tpec0_regs, xc1_rpec1_regs, xc1_tpec1_regs */

/* ===================================================================== */

#define Addr_NX4000_xc0_rpec0_regs 0xF4022000U
#define NX4000_NETX_XC0_XRP0_BASE  0xF4022000U
#define Addr_NX4000_xc0_tpec0_regs 0xF4022100U
#define NX4000_NETX_XC0_XTP0_BASE  0xF4022100U
#define Addr_NX4000_xc0_rpec1_regs 0xF4022200U
#define NX4000_NETX_XC0_XRP1_BASE  0xF4022200U
#define Addr_NX4000_xc0_tpec1_regs 0xF4022300U
#define NX4000_NETX_XC0_XTP1_BASE  0xF4022300U
#define Addr_NX4000_xc1_rpec0_regs 0xF4022400U
#define NX4000_NETX_XC1_XRP0_BASE  0xF4022400U
#define Addr_NX4000_xc1_tpec0_regs 0xF4022500U
#define NX4000_NETX_XC1_XTP0_BASE  0xF4022500U
#define Addr_NX4000_xc1_rpec1_regs 0xF4022600U
#define NX4000_NETX_XC1_XRP1_BASE  0xF4022600U
#define Addr_NX4000_xc1_tpec1_regs 0xF4022700U
#define NX4000_NETX_XC1_XTP1_BASE  0xF4022700U


/* ===================================================================== */

/* AREA xmac_ram */
/* Area of xc0_rpu0_ram, xc0_tpu0_ram, xc0_rpu1_ram, xc0_tpu1_ram, xc1_rpu0_ram, xc1_tpu0_ram, xc1_rpu1_ram, xc1_tpu1_ram */

/* ===================================================================== */

#define Addr_NX4000_xc0_rpu0_ram 0xF4020000U
#define Addr_NX4000_xc0_tpu0_ram 0xF4020400U
#define Addr_NX4000_xc0_rpu1_ram 0xF4020800U
#define Addr_NX4000_xc0_tpu1_ram 0xF4020C00U
#define Addr_NX4000_xc1_rpu0_ram 0xF4021000U
#define Addr_NX4000_xc1_tpu0_ram 0xF4021400U
#define Addr_NX4000_xc1_rpu1_ram 0xF4021800U
#define Addr_NX4000_xc1_tpu1_ram 0xF4021C00U

/* --------------------------------------------------------------------- */
/* Register xmac_ram_start */
/* => xMAC RPU program-RAM start address. */
/*    The Program-RAM is not addressable for xMAC RPU and TPU ! */
/* => Mode:  */
/* --------------------------------------------------------------------- */

#define REL_Adr_NX4000_xmac_ram_start          0x00000000U
#define Adr_NX4000_xc0_rpu0_ram_xmac_ram_start 0xF4020000U
#define Adr_NX4000_xc0_tpu0_ram_xmac_ram_start 0xF4020400U
#define Adr_NX4000_xc0_rpu1_ram_xmac_ram_start 0xF4020800U
#define Adr_NX4000_xc0_tpu1_ram_xmac_ram_start 0xF4020C00U
#define Adr_NX4000_xc1_rpu0_ram_xmac_ram_start 0xF4021000U
#define Adr_NX4000_xc1_tpu0_ram_xmac_ram_start 0xF4021400U
#define Adr_NX4000_xc1_rpu1_ram_xmac_ram_start 0xF4021800U
#define Adr_NX4000_xc1_tpu1_ram_xmac_ram_start 0xF4021C00U

/* --------------------------------------------------------------------- */
/* Register xmac_ram_end */
/* => xMAC RPU program-RAM end address. */
/*    This value is not used by design flow, only for documentation */
/* => Mode:  */
/* --------------------------------------------------------------------- */

#define REL_Adr_NX4000_xmac_ram_end          0x000003FCU
#define Adr_NX4000_xc0_rpu0_ram_xmac_ram_end 0xF40203FCU
#define Adr_NX4000_xc0_tpu0_ram_xmac_ram_end 0xF40207FCU
#define Adr_NX4000_xc0_rpu1_ram_xmac_ram_end 0xF4020BFCU
#define Adr_NX4000_xc0_tpu1_ram_xmac_ram_end 0xF4020FFCU
#define Adr_NX4000_xc1_rpu0_ram_xmac_ram_end 0xF40213FCU
#define Adr_NX4000_xc1_tpu0_ram_xmac_ram_end 0xF40217FCU
#define Adr_NX4000_xc1_rpu1_ram_xmac_ram_end 0xF4021BFCU
#define Adr_NX4000_xc1_tpu1_ram_xmac_ram_end 0xF4021FFCU

/* --------------------------------------------------------------------- */
/* Register ram_end */
/* =>  */
/* => Mode:  */
/* --------------------------------------------------------------------- */

#define REL_Adr_NX4000_ram_end            0x00001FFCU
#define Adr_NX4000_xc0_rpec0_dram_ram_end 0xF4001FFCU
#define Adr_NX4000_xc0_tpec0_dram_ram_end 0xF4003FFCU
#define Adr_NX4000_xc0_rpec1_dram_ram_end 0xF4005FFCU
#define Adr_NX4000_xc0_tpec1_dram_ram_end 0xF4007FFCU
#define Adr_NX4000_xc1_rpec0_dram_ram_end 0xF4009FFCU
#define Adr_NX4000_xc1_tpec0_dram_ram_end 0xF400BFFCU
#define Adr_NX4000_xc1_rpec1_dram_ram_end 0xF400DFFCU
#define Adr_NX4000_xc1_tpec1_dram_ram_end 0xF400FFFCU
#define Adr_NX4000_xc0_rpec0_pram_ram_end 0xF4011FFCU
#define Adr_NX4000_xc0_tpec0_pram_ram_end 0xF4013FFCU
#define Adr_NX4000_xc0_rpec1_pram_ram_end 0xF4015FFCU
#define Adr_NX4000_xc0_tpec1_pram_ram_end 0xF4017FFCU
#define Adr_NX4000_xc1_rpec0_pram_ram_end 0xF4019FFCU
#define Adr_NX4000_xc1_tpec0_pram_ram_end 0xF401BFFCU
#define Adr_NX4000_xc1_rpec1_pram_ram_end 0xF401DFFCU
#define Adr_NX4000_xc1_tpec1_pram_ram_end 0xF401FFFCU

#define DFLT_VAL_NX4000_xmac_config_obu2           0x000007c8U

#define MSK_NX4000_xmac_config_obu2_tx_nof_bits                    0x0000000fU
#define SRT_NX4000_xmac_config_obu2_tx_nof_bits                    0
#define DFLT_VAL_NX4000_xmac_config_obu2_tx_nof_bits               0x00000008U
#define DFLT_BF_VAL_NX4000_xmac_config_obu2_tx_nof_bits            0x00000008U
#define MSK_NX4000_xmac_config_obu2_tx_shift_lr                    0x00000010U
#define SRT_NX4000_xmac_config_obu2_tx_shift_lr                    4
#define DFLT_VAL_NX4000_xmac_config_obu2_tx_shift_lr               0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu2_tx_shift_lr            0x00000000U
#define MSK_NX4000_xmac_config_obu2_tx_count_stops_at_ovfl         0x00000020U
#define SRT_NX4000_xmac_config_obu2_tx_count_stops_at_ovfl         5
#define DFLT_VAL_NX4000_xmac_config_obu2_tx_count_stops_at_ovfl    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu2_tx_count_stops_at_ovfl 0x00000000U
#define MSK_NX4000_xmac_config_obu2_utx_watermark                  0x000007c0U
#define SRT_NX4000_xmac_config_obu2_utx_watermark                  6
#define DFLT_VAL_NX4000_xmac_config_obu2_utx_watermark             0x000007c0U
#define DFLT_BF_VAL_NX4000_xmac_config_obu2_utx_watermark          0x0000001fU
#define MSK_NX4000_xmac_config_obu2_tx_clk_phase                   0x00003800U
#define SRT_NX4000_xmac_config_obu2_tx_clk_phase                   11
#define DFLT_VAL_NX4000_xmac_config_obu2_tx_clk_phase              0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu2_tx_clk_phase           0x00000000U

/* all used bits of 'NX4000_xmac_config_obu2': */
#define MSK_USED_BITS_NX4000_xmac_config_obu2 0x00003fffU

#define DFLT_VAL_NX4000_xmac_config_sbu2           0x00000008U

#define MSK_NX4000_xmac_config_sbu2_rx_nof_bits                    0x0000000fU
#define SRT_NX4000_xmac_config_sbu2_rx_nof_bits                    0
#define DFLT_VAL_NX4000_xmac_config_sbu2_rx_nof_bits               0x00000008U
#define DFLT_BF_VAL_NX4000_xmac_config_sbu2_rx_nof_bits            0x00000008U
#define MSK_NX4000_xmac_config_sbu2_rx_shift_lr                    0x00000010U
#define SRT_NX4000_xmac_config_sbu2_rx_shift_lr                    4
#define DFLT_VAL_NX4000_xmac_config_sbu2_rx_shift_lr               0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_sbu2_rx_shift_lr            0x00000000U
#define MSK_NX4000_xmac_config_sbu2_rx_count_stops_at_ovfl         0x00000020U
#define SRT_NX4000_xmac_config_sbu2_rx_count_stops_at_ovfl         5
#define DFLT_VAL_NX4000_xmac_config_sbu2_rx_count_stops_at_ovfl    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_sbu2_rx_count_stops_at_ovfl 0x00000000U
#define MSK_NX4000_xmac_config_sbu2_urx_write_odd                  0x00000040U
#define SRT_NX4000_xmac_config_sbu2_urx_write_odd                  6
#define DFLT_VAL_NX4000_xmac_config_sbu2_urx_write_odd             0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_sbu2_urx_write_odd          0x00000000U
#define MSK_NX4000_xmac_config_sbu2_rx_clk_phase                   0x00000380U
#define SRT_NX4000_xmac_config_sbu2_rx_clk_phase                   7
#define DFLT_VAL_NX4000_xmac_config_sbu2_rx_clk_phase              0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_sbu2_rx_clk_phase           0x00000000U

/* all used bits of 'NX4000_xmac_config_sbu2': */
#define MSK_USED_BITS_NX4000_xmac_config_sbu2 0x000003ffU

#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo           0x00001000U

#define MSK_NX4000_xmac_config_rx_nibble_fifo_sample_phase              0x00000007U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_sample_phase              0
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_sample_phase         0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_sample_phase      0x00000000U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_delay_rx_data             0x00000008U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_delay_rx_data             3
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_delay_rx_data        0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_delay_rx_data     0x00000000U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_fifo_depth_add            0x00000030U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_fifo_depth_add            4
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_fifo_depth_add       0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_fifo_depth_add    0x00000000U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_fifo_depth                0x000003c0U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_fifo_depth                6
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_fifo_depth           0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_fifo_depth        0x00000000U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_rx_write_phase            0x00000c00U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_rx_write_phase            10
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_rx_write_phase       0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_rx_write_phase    0x00000000U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_rpu_sync_delay            0x00003000U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_rpu_sync_delay            12
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_rpu_sync_delay       0x00001000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_rpu_sync_delay    0x00000001U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_rx_err_low_active         0x00004000U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_rx_err_low_active         14
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_rx_err_low_active    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_rx_err_low_active 0x00000000U
#define MSK_NX4000_xmac_config_rx_nibble_fifo_crs_low_active            0x00008000U
#define SRT_NX4000_xmac_config_rx_nibble_fifo_crs_low_active            15
#define DFLT_VAL_NX4000_xmac_config_rx_nibble_fifo_crs_low_active       0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_rx_nibble_fifo_crs_low_active    0x00000000U

/* all used bits of 'NX4000_xmac_config_rx_nibble_fifo': */
#define MSK_USED_BITS_NX4000_xmac_config_rx_nibble_fifo 0x0000ffffU

#define DFLT_VAL_NX4000_xmac_config_mii           0x00000000U

#define MSK_NX4000_xmac_config_mii_tx_en                            0x00000001U
#define SRT_NX4000_xmac_config_mii_tx_en                            0
#define DFLT_VAL_NX4000_xmac_config_mii_tx_en                       0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_mii_tx_en                    0x00000000U
#define MSK_NX4000_xmac_config_mii_tx_err                           0x00000002U
#define SRT_NX4000_xmac_config_mii_tx_err                           1
#define DFLT_VAL_NX4000_xmac_config_mii_tx_err                      0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_mii_tx_err                   0x00000000U
#define MSK_NX4000_xmac_config_mii_txd_oe_switch_with_tx_en         0x00000004U
#define SRT_NX4000_xmac_config_mii_txd_oe_switch_with_tx_en         2
#define DFLT_VAL_NX4000_xmac_config_mii_txd_oe_switch_with_tx_en    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_mii_txd_oe_switch_with_tx_en 0x00000000U
#define MSK_NX4000_xmac_config_mii_data_reserved                    0x000000f8U
#define SRT_NX4000_xmac_config_mii_data_reserved                    3
#define DFLT_VAL_NX4000_xmac_config_mii_data_reserved               0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_mii_data_reserved            0x00000000U

/* all used bits of 'NX4000_xmac_config_mii': */
#define MSK_USED_BITS_NX4000_xmac_config_mii 0x000000ffU

#define DFLT_VAL_NX4000_xmac_config_obu           0x00000180U

#define MSK_NX4000_xmac_config_obu_sync_to_eclk                         0x00000001U
#define SRT_NX4000_xmac_config_obu_sync_to_eclk                         0
#define DFLT_VAL_NX4000_xmac_config_obu_sync_to_eclk                    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_sync_to_eclk                 0x00000000U
#define MSK_NX4000_xmac_config_obu_sync_to_bitstream                    0x00000002U
#define SRT_NX4000_xmac_config_obu_sync_to_bitstream                    1
#define DFLT_VAL_NX4000_xmac_config_obu_sync_to_bitstream               0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_sync_to_bitstream            0x00000000U
#define MSK_NX4000_xmac_config_obu_sync_to_posedge                      0x00000004U
#define SRT_NX4000_xmac_config_obu_sync_to_posedge                      2
#define DFLT_VAL_NX4000_xmac_config_obu_sync_to_posedge                 0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_sync_to_posedge              0x00000000U
#define MSK_NX4000_xmac_config_obu_sync_to_negedge                      0x00000008U
#define SRT_NX4000_xmac_config_obu_sync_to_negedge                      3
#define DFLT_VAL_NX4000_xmac_config_obu_sync_to_negedge                 0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_sync_to_negedge              0x00000000U
#define MSK_NX4000_xmac_config_obu_count_modulo                         0x00000010U
#define SRT_NX4000_xmac_config_obu_count_modulo                         4
#define DFLT_VAL_NX4000_xmac_config_obu_count_modulo                    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_count_modulo                 0x00000000U
#define MSK_NX4000_xmac_config_obu_invert_tx_oe                         0x00000020U
#define SRT_NX4000_xmac_config_obu_invert_tx_oe                         5
#define DFLT_VAL_NX4000_xmac_config_obu_invert_tx_oe                    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_invert_tx_oe                 0x00000000U
#define MSK_NX4000_xmac_config_obu_predivide_clk                        0x00000040U
#define SRT_NX4000_xmac_config_obu_predivide_clk                        6
#define DFLT_VAL_NX4000_xmac_config_obu_predivide_clk                   0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_predivide_clk                0x00000000U
#define MSK_NX4000_xmac_config_obu_tx_three_state                       0x00000080U
#define SRT_NX4000_xmac_config_obu_tx_three_state                       7
#define DFLT_VAL_NX4000_xmac_config_obu_tx_three_state                  0x00000080U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_tx_three_state               0x00000001U
#define MSK_NX4000_xmac_config_obu_eclk_noe                             0x00000100U
#define SRT_NX4000_xmac_config_obu_eclk_noe                             8
#define DFLT_VAL_NX4000_xmac_config_obu_eclk_noe                        0x00000100U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_eclk_noe                     0x00000001U
#define MSK_NX4000_xmac_config_obu_sample_bitstream_eclk                0x00000200U
#define SRT_NX4000_xmac_config_obu_sample_bitstream_eclk                9
#define DFLT_VAL_NX4000_xmac_config_obu_sample_bitstream_eclk           0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_sample_bitstream_eclk        0x00000000U
#define MSK_NX4000_xmac_config_obu_repeat_bit_at_sync                   0x00000400U
#define SRT_NX4000_xmac_config_obu_repeat_bit_at_sync                   10
#define DFLT_VAL_NX4000_xmac_config_obu_repeat_bit_at_sync              0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_repeat_bit_at_sync           0x00000000U
#define MSK_NX4000_xmac_config_obu_disable_sync_in_output_phase         0x00000800U
#define SRT_NX4000_xmac_config_obu_disable_sync_in_output_phase         11
#define DFLT_VAL_NX4000_xmac_config_obu_disable_sync_in_output_phase    0x00000000U
#define DFLT_BF_VAL_NX4000_xmac_config_obu_disable_sync_in_output_phase 0x00000000U

/* all used bits of 'NX4000_xmac_config_obu': */
#define MSK_USED_BITS_NX4000_xmac_config_obu 0x00000fffU

#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_rpec0         0x00000001U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_rpec0         0
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpec0    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpec0 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_tpec0         0x00000002U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_tpec0         1
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpec0    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpec0 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_rpu0          0x00000004U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_rpu0          2
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpu0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpu0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_tpu0          0x00000008U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_tpu0          3
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpu0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpu0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_rpec1         0x00000010U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_rpec1         4
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpec1    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpec1 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_tpec1         0x00000020U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_tpec1         5
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpec1    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpec1 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_rpu1          0x00000040U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_rpu1          6
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpu1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_rpu1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_start_tpu1          0x00000080U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_start_tpu1          7
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpu1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_start_tpu1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_rpec0         0x00000100U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_rpec0         8
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpec0    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpec0 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_tpec0         0x00000200U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_tpec0         9
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpec0    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpec0 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_rpu0          0x00000400U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_rpu0          10
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpu0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpu0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_tpu0          0x00000800U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_tpu0          11
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpu0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpu0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_rpec1         0x00001000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_rpec1         12
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpec1    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpec1 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_tpec1         0x00002000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_tpec1         13
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpec1    0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpec1 0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_rpu1          0x00004000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_rpu1          14
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpu1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_rpu1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_start_tpu1          0x00008000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_start_tpu1          15
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpu1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_start_tpu1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_rpec0          0x00010000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_rpec0          16
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpec0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpec0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_tpec0          0x00020000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_tpec0          17
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpec0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpec0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_rpu0           0x00040000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_rpu0           18
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpu0      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpu0   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_tpu0           0x00080000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_tpu0           19
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpu0      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpu0   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_rpec1          0x00100000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_rpec1          20
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpec1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpec1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_tpec1          0x00200000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_tpec1          21
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpec1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpec1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_rpu1           0x00400000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_rpu1           22
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpu1      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_rpu1   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc0_stop_tpu1           0x00800000U
#define SRT_NX4000_xc_start_stop_ctrl_xc0_stop_tpu1           23
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpu1      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc0_stop_tpu1   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_rpec0          0x01000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_rpec0          24
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpec0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpec0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_tpec0          0x02000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_tpec0          25
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpec0     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpec0  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_rpu0           0x04000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_rpu0           26
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpu0      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpu0   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_tpu0           0x08000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_tpu0           27
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpu0      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpu0   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_rpec1          0x10000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_rpec1          28
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpec1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpec1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_tpec1          0x20000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_tpec1          29
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpec1     0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpec1  0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_rpu1           0x40000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_rpu1           30
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpu1      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_rpu1   0x00000000U
#define MSK_NX4000_xc_start_stop_ctrl_xc1_stop_tpu1           0x80000000U
#define SRT_NX4000_xc_start_stop_ctrl_xc1_stop_tpu1           31
#define DFLT_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpu1      0x00000000U
#define DFLT_BF_VAL_NX4000_xc_start_stop_ctrl_xc1_stop_tpu1   0x00000000U

/* all used bits of 'NX4000_xc_start_stop_ctrl': */
#define MSK_USED_BITS_NX4000_xc_start_stop_ctrl 0xffffffffU

#define MSK_NX4000_xpec_config_timer0                  0x00000007U
#define SRT_NX4000_xpec_config_timer0                  0
#define DFLT_VAL_NX4000_xpec_config_timer0             0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_timer0          0x00000000U
#define MSK_NX4000_xpec_config_timer1                  0x00000038U
#define SRT_NX4000_xpec_config_timer1                  3
#define DFLT_VAL_NX4000_xpec_config_timer1             0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_timer1          0x00000000U
#define MSK_NX4000_xpec_config_timer2                  0x000001c0U
#define SRT_NX4000_xpec_config_timer2                  6
#define DFLT_VAL_NX4000_xpec_config_timer2             0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_timer2          0x00000000U
#define MSK_NX4000_xpec_config_timer3                  0x00000e00U
#define SRT_NX4000_xpec_config_timer3                  9
#define DFLT_VAL_NX4000_xpec_config_timer3             0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_timer3          0x00000000U
#define MSK_NX4000_xpec_config_timer4                  0x00007000U
#define SRT_NX4000_xpec_config_timer4                  12
#define DFLT_VAL_NX4000_xpec_config_timer4             0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_timer4          0x00000000U
#define MSK_NX4000_xpec_config_timer5                  0x00038000U
#define SRT_NX4000_xpec_config_timer5                  15
#define DFLT_VAL_NX4000_xpec_config_timer5             0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_timer5          0x00000000U
#define MSK_NX4000_xpec_config_register_mode           0x00300000U
#define SRT_NX4000_xpec_config_register_mode           20
#define DFLT_VAL_NX4000_xpec_config_register_mode      0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_register_mode   0x00000000U
#define MSK_NX4000_xpec_config_reset_urx_fifo0         0x01000000U
#define SRT_NX4000_xpec_config_reset_urx_fifo0         24
#define DFLT_VAL_NX4000_xpec_config_reset_urx_fifo0    0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_reset_urx_fifo0 0x00000000U
#define MSK_NX4000_xpec_config_reset_utx_fifo0         0x02000000U
#define SRT_NX4000_xpec_config_reset_utx_fifo0         25
#define DFLT_VAL_NX4000_xpec_config_reset_utx_fifo0    0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_reset_utx_fifo0 0x00000000U
#define MSK_NX4000_xpec_config_reset_urx_fifo1         0x04000000U
#define SRT_NX4000_xpec_config_reset_urx_fifo1         26
#define DFLT_VAL_NX4000_xpec_config_reset_urx_fifo1    0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_reset_urx_fifo1 0x00000000U
#define MSK_NX4000_xpec_config_reset_utx_fifo1         0x08000000U
#define SRT_NX4000_xpec_config_reset_utx_fifo1         27
#define DFLT_VAL_NX4000_xpec_config_reset_utx_fifo1    0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_reset_utx_fifo1 0x00000000U
#define MSK_NX4000_xpec_config_reset_eld0              0x10000000U
#define SRT_NX4000_xpec_config_reset_eld0              28
#define DFLT_VAL_NX4000_xpec_config_reset_eld0         0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_reset_eld0      0x00000000U
#define MSK_NX4000_xpec_config_reset_eld1              0x20000000U
#define SRT_NX4000_xpec_config_reset_eld1              29
#define DFLT_VAL_NX4000_xpec_config_reset_eld1         0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_reset_eld1      0x00000000U
#define MSK_NX4000_xpec_config_debug_mode              0x40000000U
#define SRT_NX4000_xpec_config_debug_mode              30
#define DFLT_VAL_NX4000_xpec_config_debug_mode         0x00000000U
#define DFLT_BF_VAL_NX4000_xpec_config_debug_mode      0x00000000U

typedef struct NX4000_XC_START_STOP_AREA_Ttag
{
	volatile uint32_t ulXc_start_stop_ctrl;
	volatile uint32_t ulXc_hold_status;
} NX4000_XC_START_STOP_AREA_T;

typedef struct NX4000_XMAC_AREA_Ttag
{
  volatile uint32_t aulXmac_sr[16];
  volatile uint32_t  ulXmac_status_shared0;
  volatile uint32_t  ulXmac_config_shared0;
  volatile uint32_t  ulXmac_io_oe_shared0;
  volatile uint32_t  ulXmac_status_shared1;
  volatile uint32_t  ulXmac_config_shared1;
  volatile uint32_t  ulXmac_io_oe_shared1;
  volatile uint32_t aulReserved0[6];
  volatile uint32_t aulXmac_urx_utx[2];
  volatile uint32_t aulReserved1[2];
  volatile uint32_t  ulXmac_urx;
  volatile uint32_t  ulXmac_utx;
  volatile uint32_t  ulXmac_rx;
  volatile uint32_t  ulXmac_other_rx;
  volatile uint32_t  ulXmac_rx_hw;
  volatile uint32_t  ulXmac_rx_hw_count;
  volatile uint32_t  ulXmac_tx;
  volatile uint32_t  ulXmac_tx_hw;
  volatile uint32_t  ulXmac_tx_hw_count;
  volatile uint32_t  ulXmac_tx_sent;
  volatile uint32_t  ulXmac_rpu_pc;
  volatile uint32_t  ulXmac_rpu_jmp_latch;
  volatile uint32_t  ulXmac_tpu_pc;
  volatile uint32_t  ulXmac_tpu_jmp_latch;
  volatile uint32_t aulXmac_wr[10];
  volatile uint32_t  ulXmac_sys_time;
  volatile uint32_t  ulXmac_sys_time_upper;
  volatile uint32_t  ulXmac_sys_time_uc;
  volatile uint32_t  ulXmac_sys_time_uc_upper;
  volatile uint32_t aulXmac_cmp_status[5];
  volatile uint32_t  ulXmac_alu_flags;
  volatile uint32_t  ulXmac_status_int;
  volatile uint32_t  ulXmac_stat_bits;
  volatile uint32_t  ulXmac_stat_bits_shared_lower;
  volatile uint32_t  ulXmac_stat_bits_shared_upper;
  volatile uint32_t  ulXmac_status_mii;
  volatile uint32_t  ulXmac_status_mii2;
  volatile uint32_t  ulXmac_other_status_mii2;
  volatile uint32_t  ulXmac_config_mii;
  volatile uint32_t  ulXmac_config_rx_nibble_fifo;
  volatile uint32_t  ulXmac_config_tx_nibble_fifo;
  volatile uint32_t  ulXmac_config_sbu;
  volatile uint32_t  ulXmac_config_sbu2;
  volatile uint32_t  ulXmac_sbu_rate_mul_add;
  volatile uint32_t  ulXmac_sbu_rate_mul_start;
  volatile uint32_t  ulXmac_sbu_rate_mul;
  volatile uint32_t  ulXmac_start_sample_pos;
  volatile uint32_t  ulXmac_stop_sample_pos;
  volatile uint32_t  ulXmac_config_obu;
  volatile uint32_t  ulXmac_config_obu2;
  volatile uint32_t  ulXmac_obu_rate_mul_add;
  volatile uint32_t  ulXmac_obu_rate_mul_start;
  volatile uint32_t  ulXmac_obu_rate_mul;
  volatile uint32_t  ulXmac_start_trans_pos;
  volatile uint32_t  ulXmac_stop_trans_pos;
  volatile uint32_t  ulXmac_rpu_count1;
  volatile uint32_t  ulXmac_rpu_count2;
  volatile uint32_t  ulXmac_tpu_count1;
  volatile uint32_t  ulXmac_tpu_count2;
  volatile uint32_t  ulXmac_rx_count;
  volatile uint32_t  ulXmac_tx_count;
  volatile uint32_t  ulXmac_rpm_mask0;
  volatile uint32_t  ulXmac_rpm_val0;
  volatile uint32_t  ulXmac_rpm_mask1;
  volatile uint32_t  ulXmac_rpm_val1;
  volatile uint32_t  ulXmac_tpm_mask0;
  volatile uint32_t  ulXmac_tpm_val0;
  volatile uint32_t  ulXmac_tpm_mask1;
  volatile uint32_t  ulXmac_tpm_val1;
  volatile uint32_t aulReserved2[8];
  volatile uint32_t  ulXmac_rx_crc32_l;
  volatile uint32_t  ulXmac_rx_crc32_h;
  volatile uint32_t  ulXmac_rx_crc32_cfg;
  volatile uint32_t  ulXmac_tx_crc32_l;
  volatile uint32_t  ulXmac_tx_crc32_h;
  volatile uint32_t  ulXmac_tx_crc32_cfg;
  volatile uint32_t  ulXmac_rx_crc_polynomial_l;
  volatile uint32_t  ulXmac_rx_crc_polynomial_h;
  volatile uint32_t  ulXmac_rx_crc_l;
  volatile uint32_t  ulXmac_rx_crc_h;
  volatile uint32_t  ulXmac_rx_crc_cfg;
  volatile uint32_t  ulXmac_tx_crc_polynomial_l;
  volatile uint32_t  ulXmac_tx_crc_polynomial_h;
  volatile uint32_t  ulXmac_tx_crc_l;
  volatile uint32_t  ulXmac_tx_crc_h;
  volatile uint32_t  ulXmac_tx_crc_cfg;
} NX4000_XMAC_AREA_T;

typedef struct NX4000_XPEC_AREA_Ttag
{
  volatile uint32_t aulXpec_r[8];
  volatile uint32_t  ulXpec_stat_bits_shared;
  volatile uint32_t  ulRange_urtx_count;
  volatile uint32_t  ulRange45;
  volatile uint32_t  ulRange67;
  volatile uint32_t aulTimer[4];
  volatile uint32_t  ulUrx_count;
  volatile uint32_t  ulUtx_count;
  volatile uint32_t  ulXpec_pc;
  volatile uint32_t  ulZero;
  volatile uint32_t  ulXpec_config;
  volatile uint32_t  ulEc_maska;
  volatile uint32_t  ulEc_maskb;
  volatile uint32_t aulEc_mask[10];
  volatile uint32_t  ulTimer4;
  volatile uint32_t  ulTimer5;
  volatile uint32_t  ulIrq;
  volatile uint32_t  ulXpec_systime_ns;
  volatile uint32_t  ulFifo_data;
  volatile uint32_t  ulXpec_systime_s;
  volatile uint32_t  ulDatach_data;
  volatile uint32_t aulXpec_sr[16];
  volatile uint32_t aulStatcfg[2];
  volatile uint32_t  ulDatach_wr_cfg;
  volatile uint32_t  ulDatach_rd_cfg;
  volatile uint32_t aulUrtx[2];
  volatile uint32_t  ulSysch_data;
  volatile uint32_t  ulSysch_addr;
} NX4000_XPEC_AREA_T;

#define Addr_NX4000_xc_start_stop 0xF4023A80U
#define HW_PTR_XC_START_STOP(var)      static NX4000_XC_START_STOP_AREA_T*        const var   = (NX4000_XC_START_STOP_AREA_T*) Addr_NX4000_xc_start_stop;

#define Addr_NX4000_xc0_rpec0_regs 0xF4022000U
#define Addr_NX4000_xc0_rpec1_regs 0xF4022200U
#define Addr_NX4000_xc1_rpec0_regs 0xF4022400U
#define Addr_NX4000_xc1_rpec1_regs 0xF4022600U
#define HW_PTR_RPEC_REGS(var)          static NX4000_XPEC_AREA_T* const var[] = \
{\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc0_rpec0_regs,\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc0_rpec1_regs,\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc1_rpec0_regs,\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc1_rpec1_regs\
};

#define Adr_NX4000_xc0_rpu0_ram_xmac_ram_end 0xF40203FCU
#define Adr_NX4000_xc0_rpu1_ram_xmac_ram_end 0xF4020BFCU
#define Adr_NX4000_xc1_rpu0_ram_xmac_ram_end 0xF40213FCU
#define Adr_NX4000_xc1_rpu1_ram_xmac_ram_end 0xF4021BFCU
#define HW_PTR_RPU_PRAM_END(var)       static uint32_t* const var[] = \
{\
	(uint32_t*)  Adr_NX4000_xc0_rpu0_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc0_rpu1_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc1_rpu0_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc1_rpu1_ram_xmac_ram_end\
};

#define Adr_NX4000_xc0_tpec0_pram_ram_end 0xF4013FFCU
#define Adr_NX4000_xc0_tpec1_pram_ram_end 0xF4017FFCU
#define Adr_NX4000_xc1_tpec0_pram_ram_end 0xF401BFFCU
#define Adr_NX4000_xc1_tpec1_pram_ram_end 0xF401FFFCU
#define HW_PTR_TPEC_PRAM_END(var)      static uint32_t* const var[] = \
{\
	(uint32_t*) Adr_NX4000_xc0_tpec0_pram_ram_end,\
	(uint32_t*) Adr_NX4000_xc0_tpec1_pram_ram_end,\
	(uint32_t*) Adr_NX4000_xc1_tpec0_pram_ram_end,\
	(uint32_t*) Adr_NX4000_xc1_tpec1_pram_ram_end\
};

#define Adr_NX4000_xc0_tpu0_ram_xmac_ram_start 0xF4020400U
#define Adr_NX4000_xc0_tpu1_ram_xmac_ram_start 0xF4020C00U
#define Adr_NX4000_xc1_tpu0_ram_xmac_ram_start 0xF4021400U
#define Adr_NX4000_xc1_tpu1_ram_xmac_ram_start 0xF4021C00U
#define HW_PTR_TPU_PRAM_START(var)     static uint32_t* const var[] = \
{\
	(uint32_t*)  Adr_NX4000_xc0_tpu0_ram_xmac_ram_start,\
	(uint32_t*)  Adr_NX4000_xc0_tpu1_ram_xmac_ram_start,\
	(uint32_t*)  Adr_NX4000_xc1_tpu0_ram_xmac_ram_start,\
	(uint32_t*)  Adr_NX4000_xc1_tpu1_ram_xmac_ram_start\
};

#define Addr_NX4000_xc0_tpec0_regs 0xF4022100U
#define Addr_NX4000_xc0_rpec1_regs 0xF4022200U
#define Addr_NX4000_xc1_rpec0_regs 0xF4022400U
#define Addr_NX4000_xc1_rpec1_regs 0xF4022600U
#define HW_PTR_TPEC_REGS(var)          static NX4000_XPEC_AREA_T* const var[] = \
{\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc0_tpec0_regs,\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc0_tpec1_regs,\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc1_tpec0_regs,\
	(NX4000_XPEC_AREA_T*) Addr_NX4000_xc1_tpec1_regs\
};

#define Adr_NX4000_xc0_rpec0_pram_ram_end 0xF4011FFCU
#define Adr_NX4000_xc0_rpec1_pram_ram_end 0xF4015FFCU
#define Adr_NX4000_xc1_rpec0_pram_ram_end 0xF4019FFCU
#define Adr_NX4000_xc1_rpec1_pram_ram_end 0xF401DFFCU
#define HW_PTR_RPEC_PRAM_END(var)      static uint32_t* const var[] = \
{\
	(uint32_t*) Adr_NX4000_xc0_rpec0_pram_ram_end,\
	(uint32_t*) Adr_NX4000_xc0_rpec1_pram_ram_end,\
	(uint32_t*) Adr_NX4000_xc1_rpec0_pram_ram_end,\
	(uint32_t*) Adr_NX4000_xc1_rpec1_pram_ram_end\
};

#define Addr_NX4000_xc0_xmac0_regs 0xF4022800U
#define Addr_NX4000_xc0_xmac1_regs 0xF4022A00U
#define Addr_NX4000_xc1_xmac0_regs 0xF4022C00U
#define Addr_NX4000_xc1_xmac1_regs 0xF4022E00U
#define HW_PTR_XMAC(var)               static NX4000_XMAC_AREA_T* const var[] = \
{\
(NX4000_XMAC_AREA_T*) Addr_NX4000_xc0_xmac0_regs,\
	(NX4000_XMAC_AREA_T*) Addr_NX4000_xc0_xmac1_regs,\
	(NX4000_XMAC_AREA_T*) Addr_NX4000_xc1_xmac0_regs,\
	(NX4000_XMAC_AREA_T*) Addr_NX4000_xc1_xmac1_regs\
};

#define Adr_NX4000_xc0_tpu0_ram_xmac_ram_end 0xF40207FCU
#define Adr_NX4000_xc0_tpu1_ram_xmac_ram_end 0xF4020FFCU
#define Adr_NX4000_xc1_tpu0_ram_xmac_ram_end 0xF40217FCU
#define Adr_NX4000_xc1_tpu1_ram_xmac_ram_end 0xF4021FFCU
#define HW_PTR_TPU_PRAM_END(var)       static uint32_t* const var[] = \
{\
	(uint32_t*)  Adr_NX4000_xc0_tpu0_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc0_tpu1_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc1_tpu0_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc1_tpu1_ram_xmac_ram_end\
};

#define Adr_NX4000_xc0_rpu0_ram_xmac_ram_end 0xF40203FCU
#define Adr_NX4000_xc0_rpu1_ram_xmac_ram_end 0xF4020BFCU
#define Adr_NX4000_xc1_rpu0_ram_xmac_ram_end 0xF40213FCU
#define Adr_NX4000_xc1_rpu1_ram_xmac_ram_end 0xF4021BFCU
#define HW_PTR_RPU_PRAM_END(var)       static uint32_t* const var[] = \
{\
	(uint32_t*)  Adr_NX4000_xc0_rpu0_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc0_rpu1_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc1_rpu0_ram_xmac_ram_end,\
	(uint32_t*)  Adr_NX4000_xc1_rpu1_ram_xmac_ram_end\
};


#define Adr_NX4000_xc0_tpec0_pram_ram_start 0xF4012000U
#define Adr_NX4000_xc0_tpec1_pram_ram_start 0xF4016000U
#define Adr_NX4000_xc1_tpec0_pram_ram_start 0xF401A000U
#define Adr_NX4000_xc1_tpec1_pram_ram_start 0xF401E000U
#define HW_PTR_TPEC_PRAM_START(var)    static uint32_t* const var[] = \
{\
	(uint32_t*) Adr_NX4000_xc0_tpec0_pram_ram_start,\
	(uint32_t*) Adr_NX4000_xc0_tpec1_pram_ram_start,\
	(uint32_t*) Adr_NX4000_xc1_tpec0_pram_ram_start,\
	(uint32_t*) Adr_NX4000_xc1_tpec1_pram_ram_start\
};

#define Adr_NX4000_xc0_rpu0_ram_xmac_ram_start 0xF4020000U
#define Adr_NX4000_xc0_rpu1_ram_xmac_ram_start 0xF4020800U
#define Adr_NX4000_xc1_rpu0_ram_xmac_ram_start 0xF4021000U
#define Adr_NX4000_xc1_rpu1_ram_xmac_ram_start 0xF4021800U
#define HW_PTR_RPU_PRAM_START(var)     static uint32_t* const var[] = \
{\
	(uint32_t*)  Adr_NX4000_xc0_rpu0_ram_xmac_ram_start,\
	(uint32_t*)  Adr_NX4000_xc0_rpu1_ram_xmac_ram_start,\
	(uint32_t*)  Adr_NX4000_xc1_rpu0_ram_xmac_ram_start,\
	(uint32_t*)  Adr_NX4000_xc1_rpu1_ram_xmac_ram_start\
};

#define Adr_NX4000_xc0_rpec0_pram_ram_start 0xF4010000U
#define Adr_NX4000_xc0_rpec1_pram_ram_start 0xF4014000U
#define Adr_NX4000_xc1_rpec0_pram_ram_start 0xF4018000U
#define Adr_NX4000_xc1_rpec1_pram_ram_start 0xF401C000U
#define HW_PTR_RPEC_PRAM_START(var)    static uint32_t* const var[] = \
{\
	(uint32_t*) Adr_NX4000_xc0_rpec0_pram_ram_start,\
	(uint32_t*) Adr_NX4000_xc0_rpec1_pram_ram_start,\
	(uint32_t*) Adr_NX4000_xc1_rpec0_pram_ram_start,\
	(uint32_t*) Adr_NX4000_xc1_rpec1_pram_ram_start\
};

#define Addr_NX4000_xc0_pointer_fifo        0xF4023000U
#define Addr_NX4000_xc1_pointer_fifo        0xF4023200U

#define Adr_NX4000_xc0_tpec0_dram_ram_start 0xF4002000U
#define Adr_NX4000_xc0_tpec1_dram_ram_start 0xF4006000U
#define Adr_NX4000_xc1_tpec1_dram_ram_start 0xF400E000U
#define Adr_NX4000_xc1_tpec0_dram_ram_start 0xF400A000U


#define Adr_NX4000_xc0_irq_xpec0                       0xF4023A60U
#define Adr_NX4000_xc0_irq_xpec1                       0xF4023A64U
#define Adr_NX4000_xc1_irq_xpec0                       0xF4023A68U
#define Adr_NX4000_xc1_irq_xpec1                       0xF4023A6CU

#define Addr_NX4000_intram0         0x05080000U
#define Addr_NX4000_intram1         0x05090000U
#define Addr_NX4000_intram2         0x050A0000U
#define Addr_NX4000_intram3         0x050B0000U

#define REL_Adr_NX4000_ram_end            0x00001FFCU

#define REL_Adr_NX4000_intram_end             0x0000FFFCU

HW_PTR_XC_START_STOP(s_ptXcStartStop)

HW_PTR_RPEC_REGS(s_aptRpecRegArea)
HW_PTR_RPEC_PRAM_START(s_apulRpecPramArea)
HW_PTR_RPEC_PRAM_END(s_apulRpecPramAreaEnd)

HW_PTR_TPEC_PRAM_START(s_apulTpecPramArea)
HW_PTR_TPEC_PRAM_END(s_apulTpecPramAreaEnd)

HW_PTR_TPU_PRAM_START(s_aptTpuPramArea)
HW_PTR_TPU_PRAM_END(s_aptTpuPramAreaEnd)

HW_PTR_TPEC_REGS(s_aptTpecRegArea)

HW_PTR_XMAC(s_aptXmacArea)

HW_PTR_RPU_PRAM_START(s_aptRpuPramArea)
HW_PTR_RPU_PRAM_END(s_aptRpuPramAreaEnd)

typedef uint8_t ETHERNET_MAC_ADDR_T[6];

typedef struct NX4000_POINTER_FIFO_AREA_Ttag
{
  volatile uint32_t aulPfifo[32];
  volatile uint32_t aulPfifo_border[32];
  volatile uint32_t  ulPfifo_reset;
  volatile uint32_t  ulPfifo_full;
  volatile uint32_t  ulPfifo_empty;
  volatile uint32_t  ulPfifo_overflow;
  volatile uint32_t  ulPfifo_underrun;
  volatile uint32_t aulReserved2[27];
  volatile uint32_t aulPfifo_fill_level[32];
} NX4000_POINTER_FIFO_AREA_T;

#endif
