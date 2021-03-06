/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file dw3000_phy.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief physical layer
 *
 * @details This is the phy base class which utilises the functions to set the clocks,initializes phy layer and configures the required
 * parameters.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <math.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <dw3000-c0/dw3000_phy.h>

static inline void _dw3000_phy_load_microcode(struct _dw3000_dev_instance_t * inst);

void dw3000_phy_enable_ext_pa(struct _dw3000_dev_instance_t* inst, bool enable)
{
    uint8_t buf[2] = {0x00,0x00};

    if(enable == true) {
        dw3000_gpio4_config_ext_pa(inst);
        dw3000_gpio5_config_ext_txe(inst);
        //if an external power amplifier is being used, TX fine grain power dequeencing must be disabled
        dw3000_write(inst, PMSC_ID, PMSC_TXFINESEQ_OFFSET, buf, 2);
    } else {
        //TODO, config as gpio mode
    }
}

void dw3000_phy_enable_ext_lna(struct _dw3000_dev_instance_t* inst, bool enable)
{
    if(enable == true) {
        dw3000_gpio6_config_ext_rxe(inst);
    } else {
        //TODO, config as gpio mode
    }
}

/**
 * API that force system clock to be the 19.2 MHz XTI clock.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_sysclk_XTAL(struct _dw3000_dev_instance_t * inst)
{
    uint8_t reg = (uint8_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint8_t));
    reg &= (uint8_t)~PMSC_CTRL0_SYSCLKS_19M & (uint8_t)~PMSC_CTRL0_SYSCLKS_125M;
    reg |= (uint8_t) PMSC_CTRL0_SYSCLKS_19M;
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, reg, sizeof(uint8_t));
}

/**
 * API that force system clock to be the 125 MHz PLL clock.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_sysclk_PLL(struct _dw3000_dev_instance_t * inst)
{
    uint8_t reg = (uint8_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint8_t));
    reg &= (uint8_t)~PMSC_CTRL0_SYSCLKS_19M & (uint8_t)~PMSC_CTRL0_SYSCLKS_125M;
    reg |= (uint8_t) PMSC_CTRL0_SYSCLKS_125M;
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, reg, sizeof(uint8_t));
}

/**
 * API that force txclock on
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_txclk_PLL(struct _dw3000_dev_instance_t * inst)
{
    uint8_t reg = (uint8_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint8_t));
    reg &= (uint8_t)~PMSC_CTRL0_TXCLKS_19M & (uint8_t)~PMSC_CTRL0_TXCLKS_125M;
    reg |= (uint8_t) PMSC_CTRL0_TXCLKS_125M;
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, reg, sizeof(uint8_t));
}

/**
 * API to enable running of the LDE algorithm.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_sysclk_LDE(struct _dw3000_dev_instance_t * inst){
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, 0x01, sizeof(uint8_t));
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET + 1 , 0x03, sizeof(uint8_t));
}

/**
 * API to enable PLL2 on/off sequencing by SNIFF mode.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_sysclk_SEQ(struct _dw3000_dev_instance_t * inst)
{
    uint8_t reg = (uint8_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint8_t));
    reg &= (uint8_t)~PMSC_CTRL0_SYSCLKS_19M & (uint8_t)~PMSC_CTRL0_SYSCLKS_125M;
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, reg, sizeof(uint8_t));
}

/**
 * API to enable PLL2 on/off sequencing by SNIFF mode through masking of pmsc_ctrl_lo and pmsc_ctrl_hi.
 *
 * @param inst   Pointer to dw3000_dev_instance_t.
 * @param mode   Switch to the case specified.
 * @return void
 */
void dw3000_phy_sysclk_ACC(struct _dw3000_dev_instance_t * inst, uint8_t mode)
{
    uint8_t pmsc_ctrl_lo = (uint8_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint8_t));
    uint8_t pmsc_ctrl_hi = (uint8_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET + 1, sizeof(uint8_t));

    switch(mode){
        case true:
            pmsc_ctrl_lo  =  0x48 | (pmsc_ctrl_lo & 0xb3);
            pmsc_ctrl_hi  =  0x80 | pmsc_ctrl_hi;
            break;
        default:
            pmsc_ctrl_lo  =  pmsc_ctrl_lo & 0xb3;
            pmsc_ctrl_hi  =  0x7f & pmsc_ctrl_hi ;
            break;
    }
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, pmsc_ctrl_lo, sizeof(uint8_t));
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET+1, pmsc_ctrl_hi, sizeof(uint8_t));
}


/**
 * API to disable PMSC control of analog RF subsystems.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_disable_sequencing(struct _dw3000_dev_instance_t * inst)
{
    dw3000_phy_sysclk_XTAL(inst);
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL1_OFFSET, PMSC_CTRL1_PKTSEQ_DISABLE, sizeof(uint16_t)); // Disable PMSC ctrl of RF and RX clk blocks
}

/**
 * API to initialise the phy layer.
 *
 * @param inst         Pointer to dw3000_dev_instance_t.
 * @param txrf_config  Pointer to dw3000_dev_txrf_config_t.
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
dw3000_phy_init(struct _dw3000_dev_instance_t * inst, struct uwb_dev_txrf_config * txrf_config)
{
    uint8_t reg;
    uint32_t otp_addr;
    uint32_t ldo_tune;
    if (txrf_config == NULL)
        txrf_config = &inst->uwb_dev.config.txrf;
    else
        memcpy(&inst->uwb_dev.config.txrf, txrf_config, sizeof(struct uwb_dev_txrf_config));

    dw3000_softreset(inst);
    dw3000_phy_sysclk_XTAL(inst);
#if MYNEWT_VAL(DW3000_RXTX_LEDS)
    dw3000_gpio_config_leds(inst, DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
#endif

#if MYNEWT_VAL(DW3000_RXTX_GPIO)
    dw3000_gpio5_config_ext_txe(inst);
    dw3000_gpio6_config_ext_rxe(inst);
#endif

    // Configure the CPLL lock detect
    reg = dw3000_read_reg(inst, EXT_SYNC_ID, EC_CTRL_OFFSET, sizeof(uint8_t));
    reg |= EC_CTRL_PLLLCK;
    dw3000_write_reg(inst, EXT_SYNC_ID, EC_CTRL_OFFSET, reg, sizeof(uint8_t));

    // Read OTP revision number
    otp_addr = (uint32_t) _dw3000_otp_read(inst, OTP_XTRIM_ADDRESS) & 0xffff;    // Read 32 bit value, XTAL trim val is in low octet-0 (5 bits)
    inst->otp_rev = (otp_addr >> 8) & 0xff;                                               // OTP revision is next byte

    // Load LDO tune from OTP and kick it if there is a value actually programmed.
    ldo_tune = _dw3000_otp_read(inst, OTP_LDOTUNE_ADDRESS);
    if((ldo_tune & 0xFF) != 0){
        dw3000_write_reg(inst, OTP_IF_ID, OTP_SF, OTP_SF_LDO_KICK, sizeof(uint8_t)); // Set load LDE kick bit
        inst->uwb_dev.status.LDO_enabled = 1; // LDO tune must be kicked at wake-up
    }
    // Load Part and Lot ID from OTP
    inst->part_id = _dw3000_otp_read(inst, OTP_PARTID_ADDRESS);
    inst->lot_id = _dw3000_otp_read(inst, OTP_LOTID_ADDRESS);
    // Load vbat and vtemp from OTP
    inst->otp_vbat = _dw3000_otp_read(inst, OTP_VBAT_ADDRESS);
    inst->otp_temp = _dw3000_otp_read(inst, OTP_VTEMP_ADDRESS);

    // XTAL trim value is set in OTP for DW3000 module and EVK/TREK boards but that might not be the case in a custom design
    inst->otp_xtal_trim = otp_addr & 0x1F;
    /* A value of 0 in OTP means that the crystal has not been trimmed
     * Only use the OTP value if we don't have an overriding value in config
     */
    if (inst->otp_xtal_trim && (inst->uwb_dev.config.rx.xtalTrim == 0xff)) {
        inst->uwb_dev.config.rx.xtalTrim = inst->otp_xtal_trim;
    }

    // The 3 MSb in this 8-bit register must be kept to 0b011 to avoid any malfunction.
    reg = (3 << 5) | (inst->uwb_dev.config.rx.xtalTrim & FS_XTALT_MASK);
    dw3000_write_reg(inst, FS_CTRL_ID, FS_XTALT_OFFSET, reg, sizeof(uint8_t));

    if(inst->uwb_dev.config.LDE_enable)
        _dw3000_phy_load_microcode(inst);

    dw3000_phy_sysclk_SEQ(inst);    // Enable clocks for sequencing

    // The 3 bits in AON CFG1 register must be cleared to ensure proper operation of the DW3000 in DEEPSLEEP mode.
    reg = dw3000_read_reg(inst, AON_ID, AON_CFG1_OFFSET, sizeof(uint8_t));
    reg |= ~AON_CFG1_SMXX;
    dw3000_write_reg(inst, AON_ID, AON_CFG1_OFFSET, reg, sizeof(uint8_t));

    // Enable Temp & Vbat SAR onwake mode
    dw3000_write_reg(inst, AON_ID, AON_WCFG_OFFSET , AON_WCFG_ONW_RADC, sizeof(uint16_t));

    // Apply default antenna delay value. See NOTE 2 below. */
    dw3000_phy_set_rx_antennadelay(inst, inst->uwb_dev.rx_antenna_delay);
    dw3000_phy_set_tx_antennadelay(inst, inst->uwb_dev.tx_antenna_delay);

    // Apply tx power settings */
    dw3000_phy_config_txrf(inst, txrf_config);

    // Read system register / store local copy
    inst->sys_cfg_reg = dw3000_read_reg(inst, SYS_CFG_ID, 0, sizeof(uint32_t)) ; // Read sysconfig register

    return inst->uwb_dev.status;
}

/**
 * API to load LDE microcode.
 *
 * @param inst   Pointer to _dw3000_dev_instance_t.
 * @return void
 */
void _dw3000_phy_load_microcode(struct _dw3000_dev_instance_t * inst)
{
    // Set up clocks
    dw3000_phy_sysclk_LDE(inst);

    // Kick off the LDE Load
    dw3000_write_reg(inst, OTP_IF_ID, OTP_CTRL, OTP_CTRL_LDELOAD, sizeof(uint16_t)); // Set load LDE kick bit
    dpl_cputime_delay_usecs(120); // Allow time for code to upload (should take up to 120 us)
    dw3000_phy_sysclk_SEQ(inst); // Enable clocks for sequencing
    inst->uwb_dev.status.LDE_enabled = 1;
}

/**
 * API to Configure LDE algorithm parameters.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @param prf   This is the PRF index (0 or 1) 0 corresponds to 16 and 1 to 64 PRF.
 * @return void
 */
void dw3000_phy_config_lde(struct _dw3000_dev_instance_t * inst, int prfIndex)
{
    dw3000_write_reg(inst, LDE_IF_ID, LDE_CFG1_OFFSET, LDE_PARAM1, sizeof(uint8_t)); // 8-bit configuration register

    if(prfIndex)
        dw3000_write_reg(inst, LDE_IF_ID, LDE_CFG2_OFFSET, LDE_PARAM3_64, sizeof(uint16_t)); // 16-bit LDE configuration tuning register
    else
        dw3000_write_reg(inst, LDE_IF_ID, LDE_CFG2_OFFSET, LDE_PARAM3_16, sizeof(uint16_t));
}


/**
 * API for the configuration of the TX spectrum
 * including the power and pulse generator delay. The input is a pointer to the data structure
 * of type dwt_txconfig_t that holds all the configurable items.
 *
 * @param inst      Pointer to dw3000_dev_instance_t.
 * @param config    Pointer to dw3000_dev_txrf_config_t.
 * @return void
 */
void dw3000_phy_config_txrf(struct _dw3000_dev_instance_t * inst, struct uwb_dev_txrf_config *config)
{
    // Configure RF TX PG_DELAY
    dw3000_write_reg(inst, TX_CAL_ID, TC_PGDELAY_OFFSET, config->PGdly, sizeof(uint8_t));
    // Configure TX power
    dw3000_write_reg(inst, TX_POWER_ID, 0, config->power, sizeof(uint32_t));
}


#ifndef __KERNEL__
/**
 * API to read the temperature of the DW3000 that was sampled
 * on waking from Sleep/Deepsleep. They are not current values, but read on last
 * wakeup if DWT_TANDV bit is set in mode parameter of dwt_configuresleep.
 *
 * @param inst    Pointer to dw3000_dev_instance_t.
 * @return float  value for temperature sensor in SI units (Degrees C).
 */
float dw3000_phy_read_wakeuptemp_SI(struct _dw3000_dev_instance_t * inst)
{
   return 1.14 * (dw3000_phy_read_wakeuptemp(inst) - inst->otp_temp) + 23;
}

/**
 * API to read the battery voltage of the DW3000 that was sampled
 * on waking from Sleep/Deepsleep. They are not current values, but read on last
 * wakeup if DWT_TANDV bit is set in mode parameter of dwt_configure_sleep.
 *
 * @param inst    Pointer to dw3000_dev_instance_t.
 * @return float  Value of battery voltage sensor in SI units (Volts).
 */
float dw3000_phy_read_read_wakeupvbat_SI(struct _dw3000_dev_instance_t * inst)
{
    return (1.0/173) * (dw3000_phy_read_wakeupvbat(inst) - inst->otp_vbat) + 3.3;
}
#endif

/**
 * API to reset the receiver of the DW3000.
 *
 * @param inst   Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_rx_reset(struct _dw3000_dev_instance_t * inst)
{
    dpl_error_t err = dpl_mutex_pend(&inst->mutex, DPL_WAIT_FOREVER);
    if (err != DPL_OK) {
        inst->uwb_dev.status.mtx_error = 1;
        return;
    }

    // Set RX reset
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_SOFTRESET_OFFSET, PMSC_CTRL0_RESET_RX, sizeof(uint8_t));
    // Clear RX reset
    dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_SOFTRESET_OFFSET, PMSC_CTRL0_RESET_CLEAR, sizeof(uint8_t));

    err = dpl_mutex_release(&inst->mutex);
    assert(err == DPL_OK);
}


/**
 * API to turn off the transceiver.
 *
 * @param inst  Pointer to dw3000_dev_instance_t.
 * @return void
 */
void dw3000_phy_forcetrxoff(struct _dw3000_dev_instance_t * inst)
{
    dpl_error_t err;
    struct uwb_mac_interface * cbs = NULL;
    uint32_t mask = dw3000_read_reg(inst, SYS_MASK_ID, 0 , sizeof(uint32_t)) ; // Read set interrupt mask

    // Need to beware of interrupts occurring in the middle of following read modify write cycle
    // We can disable the radio, but before the status is cleared an interrupt can be set (e.g. the
    // event has just happened before the radio was disabled)
    // thus we need to disable interrupt during this operation

    err = dpl_mutex_pend(&inst->mutex, DPL_WAIT_FOREVER);
    if (err != DPL_OK) {
        inst->uwb_dev.status.mtx_error = 1;
        return;
    }

    dw3000_write_reg(inst, SYS_MASK_ID, 0, 0, sizeof(uint32_t)) ; // Clear interrupt mask - so we don't get any unwanted events
    dw3000_write_reg(inst, SYS_CTRL_ID, SYS_CTRL_OFFSET, SYS_CTRL_TRXOFF, sizeof(uint8_t)) ; // Disable the radio
    // Forcing Transceiver off - so we do not want to see any new events that may have happened
    dw3000_write_reg(inst, SYS_STATUS_ID, 0, (SYS_STATUS_ALL_TX | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_GOOD| SYS_STATUS_TXBERR), sizeof(uint32_t));

    if (inst->uwb_dev.config.dblbuffon_enabled)
        dw3000_sync_rxbufptrs(inst);

    dw3000_write_reg(inst, SYS_MASK_ID, 0, mask, sizeof(uint32_t)); // Restore mask to what it was

    if(!(SLIST_EMPTY(&inst->uwb_dev.interface_cbs))){
        SLIST_FOREACH(cbs, &inst->uwb_dev.interface_cbs, next){
            if (cbs!=NULL && cbs->reset_cb)
                if(cbs->reset_cb((struct uwb_dev*)inst, cbs)) continue;
            }
    }
    // Enable/restore interrupts again...
    err = dpl_mutex_release(&inst->mutex);
    assert(err == DPL_OK);

    inst->control.wait4resp_enabled = 0;
    inst->control.rxauto_disable = false;
    inst->control.abs_timeout = false;

    /* Reset semaphore if needed */
    if (dpl_sem_get_count(&inst->tx_sem) == 0) {
        err = dpl_sem_release(&inst->tx_sem);
        assert(err == DPL_OK);
        inst->uwb_dev.status.sem_force_released = 1;
    }
}

/**
 * API to enable the specified events to trigger an interrupt.
 * The following events can be enabled:
 * DWT_INT_TFRS         0x00000080          // frame sent
 * DWT_INT_RFCG         0x00004000          // frame received with good CRC
 * DWT_INT_RPHE         0x00001000          // receiver PHY header error
 * DWT_INT_RFCE         0x00008000          // receiver CRC error
 * DWT_INT_RFSL         0x00010000          // receiver sync loss error
 * DWT_INT_RFTO         0x00020000          // frame wait timeout
 * DWT_INT_RXPTO        0x00200000          // preamble detect timeout
 * DWT_INT_SFDT         0x04000000          // SFD timeout
 * DWT_INT_ARFE         0x20000000          // frame rejected (due to frame filtering configuration)
 *
 * @param inst      Pointer to dw3000_dev_instance_t.
 * @param bitmask   Sets the events which generates interrupt.
 * @param enable    If set, the interrupts are enabled else they are cleared.
 * @return void
 */
void dw3000_phy_interrupt_mask(struct _dw3000_dev_instance_t * inst, uint32_t bitmask, uint8_t enable)
{
    // Critical region, atomic lock with mutex
    uint32_t mask;
    dpl_error_t err = dpl_mutex_pend(&inst->mutex, DPL_WAIT_FOREVER);
    if (err != DPL_OK) {
        inst->uwb_dev.status.mtx_error = 1;
        goto mtx_error;
    }

    mask = dw3000_read_reg(inst, SYS_MASK_ID, 0, sizeof(uint32_t)) ; // Read register

    if(enable)
        mask |= bitmask ;
    else
        mask &= ~bitmask ; // Clear the bit

    dw3000_write_reg(inst, SYS_MASK_ID, 0, mask, sizeof(uint32_t));

    // Critical region, unlock mutex
    err = dpl_mutex_release(&inst->mutex);
    assert(err == DPL_OK);
mtx_error:
    return;
}

/**
 * API to synchronise DW3000 with external clocks or events or with other DW3000???s.
 * For example, this would be required in a TDOA RTLS system employing wired clock synchronisation of the
 * anchor nodes or AoA node for phase measurement.
 *
 * @param inst      Pointer to dw3000_dev_instance_t.
 * @param delay     To configure DW3000 for OSTR mode, the delay value is set to the desired delay value.
 * @param enable    True/false.
 * @return void
 */
void dw3000_phy_external_sync(struct _dw3000_dev_instance_t * inst, uint8_t delay, bool enable){

    uint16_t reg = dw3000_read_reg(inst, EXT_SYNC_ID, EC_CTRL_OFFSET, sizeof(uint16_t));
    if (enable) {
        reg &= ~EC_CTRL_WAIT_MASK; //clear timer value, clear OSTRM
        reg |= EC_CTRL_OSTRM;      //External timebase reset mode enable
        reg |= ((((uint16_t) delay) & 0xff) << 3); //set new timer value

    }else {
        reg &= ~(EC_CTRL_WAIT_MASK | EC_CTRL_OSTRM); //clear timer value, clear OSTRM
    }
    dw3000_write_reg(inst, EXT_SYNC_ID, EC_CTRL_OFFSET, reg, sizeof(uint16_t));
}

/**
 * @brief This function enables repeated frames to be generated given a frame repetition rate.
 *
 * input parameters:
 * @param inst     Pointer to dw3000_dev_instance_t.
 * @param rate     Rate at which frames with be repeated in dtu, set to 0 to disable
 * @return void
 */
void
dw3000_phy_repeated_frames(struct _dw3000_dev_instance_t * inst, uint64_t rate)
{
    int rc;

    if (!rate) {
        /* Stop sending packets */
        dw3000_write_reg(inst, RF_CONF_ID, 0, 0, sizeof(uint32_t));
        dw3000_write_reg(inst, DIG_DIAG_ID, DIAG_TMC_OFFSET, 0, sizeof(uint8_t));
    } else {
        /* Lower the speed of the SPI
         * This is needed because we disable the higher sysclk and thus
         * dw3000 only support < 2Mbit spi */
        inst->spi_settings.baudrate = inst->spi_baudrate_low;
        rc = hal_spi_disable(inst->spi_num);
        assert(rc == 0);
        rc = hal_spi_config(inst->spi_num, &inst->spi_settings);
        assert(rc == 0);
        rc = hal_spi_enable(inst->spi_num);
        assert(rc == 0);

        printf("PMSC_ID[0]: %"PRIx32"\n", (uint32_t)dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint32_t)));
        printf("PMSC_ID[1]: %"PRIx32"\n", (uint32_t)dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL1_OFFSET, sizeof(uint32_t)));

        dw3000_phy_disable_sequencing(inst);

        /* enable RF PLL */
        dw3000_write_reg(inst, RF_CONF_ID, 0, RF_CONF_TXPLLPOWEN_MASK, sizeof(uint32_t));
        dw3000_write_reg(inst, RF_CONF_ID, 0, RF_CONF_TXALLEN_MASK, sizeof(uint32_t));

        dw3000_phy_sysclk_PLL(inst);
        dw3000_phy_txclk_PLL(inst);

        if ((rate>>8) < 4) {
            rate = ((uint64_t)4<<8);
        }
        dw3000_write_reg(inst, DX_TIME_ID, 0, rate>>8, sizeof(uint32_t));

        /* Turn the tx power spectrum test mode - continuous sending of frames */
        dw3000_write_reg(inst, DIG_DIAG_ID, DIAG_TMC_OFFSET,
                         (uint8_t)(DIAG_TMC_TX_PSTM), sizeof(uint8_t));

        /* Trigger first frame - Needed?? */
        dw3000_write_reg(inst, SYS_CTRL_ID, SYS_CTRL_OFFSET, SYS_CTRL_TXSTRT, sizeof(uint8_t));
    }
}

/**
 * API to calculate the SHR (Preamble + SFD) duration. This is used to calculate the correct rx_timeout.
 * @param attrib    Pointer to _phy_attributes_t * struct. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw3000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec
 */
uint16_t
dw3000_phy_SHR_duration(struct uwb_phy_attributes * attrib)
{
    uint16_t duration;
    /* ceilf(attrib->Tpsym * (attrib->nsync + attrib->nsfd)); */
    /* TXsym is represented as a float32_t */
#ifdef __KERNEL__
    duration = f32_to_i32(
        f32_mul(attrib->Tpsym, ui32_to_f32(attrib->nsync + attrib->nsfd)), 3, false);
#else
    duration = ceilf(attrib->Tpsym * (attrib->nsync + attrib->nsfd));
#endif
    return duration;
}

/**
 * API to calculate the data duration (airtime).
 * @param attrib    Pointer to _phy_attributes_t * struct. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw3000_hal.c for the default behaviour.
 * @param nlen      The length of the frame to be transmitted/received excluding crc. Set to 0 to estimate frame duration
 *                  when using Cipher Mode 3 (no payload)
 * @return uint16_t duration in usec
 */
uint16_t
dw3000_phy_data_duration(struct uwb_phy_attributes * attrib, uint16_t nlen)
{
    /* TXsym is represented as a dpl_float32_t */
    int32_t parity_data_bits = 48;
    int32_t total_payload_bits;
    uint16_t duration;
    /* We need to add 48 parity bits for every 330 bits in the data payload (including crc)
     * I.e. for < 330 bits we need 48 parity bits, for > 330 bits we need 48+48 parity bits,
     * for > 660 bits we need 3*48 parity bits, etc. */
    parity_data_bits += ((8*(nlen+2))/330) * 48;
    total_payload_bits = 8*(nlen+2) + parity_data_bits;

#ifdef __KERNEL__
    {
        dpl_float32_t tmp;
        tmp = DPL_FLOAT32_MUL(attrib->Tbsym, DPL_FLOAT32_I32_TO_F32(attrib->nphr));
        tmp = DPL_FLOAT32_ADD(tmp, DPL_FLOAT32_MUL(attrib->Tdsym, DPL_FLOAT32_I32_TO_F32(total_payload_bits)));
        duration = f32_to_i32(DPL_FLOAT32_CEIL(tmp), softfloat_round_max, false);
    }
#else
    duration = (int)ceilf(attrib->Tbsym * attrib->nphr +
                          attrib->Tdsym * total_payload_bits);
#endif
    return duration;
}

/**
 * API to calculate the frame duration (airtime).
 * @param attrib    Pointer to _phy_attributes_t * struct. The phy attritubes are part of the IEEE802.15.4-2011 standard.
 * Note the morphology of the frame depends on the mode of operation, see the dw3000_hal.c for the default behaviour
 * @param nlen      The length of the frame to be transmitted/received excluding crc
 * @return uint16_t duration in usec
 */
inline uint16_t
dw3000_phy_frame_duration(struct uwb_phy_attributes * attrib, uint16_t nlen)
{
    return dw3000_phy_SHR_duration(attrib) + dw3000_phy_data_duration(attrib, nlen);
}

/**
 * Translate coarse and fine power levels to a registry value used in struct uwb_dev_txrf_config.
 *
 * @param inst      Pointer to struct uwb_dev
 * @param reg       Pointer to where to store the registry value
 * @param coarse    Coarse power control value in dBm (DA)
 * @param fine      Fine power value in dBm (Mixer)
 * @return true on success, false otherwise
 */
bool
dw3000_phy_txrf_power_value(struct _dw3000_dev_instance_t * inst, uint8_t *reg,
                            dpl_float32_t coarse, dpl_float32_t fine)
{
    int c = DPL_FLOAT32_INT(coarse);
    if (!reg) {
        return false;
    }
    if (DPL_FLOAT32_INT(fine) < 0 || DPL_FLOAT32_INT(fine) > 32) {
        return false;
    }

    switch (c) {
    case(18): *reg = dw3000_power_valuef(DW3000_txrf_config_18db, fine);break;
    case(15): *reg = dw3000_power_valuef(DW3000_txrf_config_15db, fine);break;
    case(12): *reg = dw3000_power_valuef(DW3000_txrf_config_12db, fine);break;
    case(9):  *reg = dw3000_power_valuef(DW3000_txrf_config_9db, fine);break;
    case(6):  *reg = dw3000_power_valuef(DW3000_txrf_config_6db, fine);break;
    case(3):  *reg = dw3000_power_valuef(DW3000_txrf_config_3db, fine);break;
    case(0):  *reg = dw3000_power_valuef(DW3000_txrf_config_0db, fine);break;
    default:
        return false;
    }
    return true;
}

/**
 * Enable and/or reset device event counters
 *
 * @param inst    Pointer to struct dw3000_dev_instance
 * @param enable  Enables the device internal event counters
 * @param reset   If true, reset counters
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
dw3000_phy_event_cnt_ctrl(struct _dw3000_dev_instance_t *inst, bool enable, bool reset)
{
    if (reset) {
        /* A reset command must also write enable = 0 or the clearing will not happen */
        dw3000_write_reg(inst, DIG_DIAG_ID, EVC_CTRL_OFFSET, EVC_CLR, sizeof(uint32_t));
    }

    /* Intentionally writing 32 bits here and above as the register requires at least 16 bits
     * written to take effect. */
    dw3000_write_reg(inst, DIG_DIAG_ID, EVC_CTRL_OFFSET, (enable) ? EVC_EN : 0, sizeof(uint32_t));
    return inst->uwb_dev.status;
}

/**
 * Read device event counters
 *
 * @param inst    Pointer to struct dw3000_dev_instance
 * @param res     Pointer to struct uwb_dev_evcnt
 * @return struct uwb_dev_status
 */
struct uwb_dev_status
dw3000_phy_event_cnt_read(struct _dw3000_dev_instance_t *inst, struct uwb_dev_evcnt *res)
{
    if (!res) {
        return inst->uwb_dev.status;
    }
    /* Read first 6 counter registers in one go */
    dw3000_read(inst, DIG_DIAG_ID, EVC_PHE_OFFSET, (uint8_t*)res,
                offsetof(struct uwb_dev_evcnt, event_count6));
    /* Apply masks */
    res->event_count0 &= ((EVC_RSE_MASK << 16) | EVC_PHE_MASK);
    res->event_count1 &= ((EVC_FCE_MASK << 16) | EVC_FCG_MASK);
    res->event_count2 &= ((EVC_OVR_MASK << 16) | EVC_FFR_MASK);
    res->event_count3 &= ((EVC_PTO_MASK << 16) | EVC_STO_MASK);
    res->event_count4 &= ((EVC_TXFS_MASK << 16) | EVC_FWTO_MASK);
    res->event_count5 &= ((EVC_TPW_MASK << 16) | EVC_HPW_MASK);
    return inst->uwb_dev.status;
}
