/**
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

#ifndef _CIR_DW3000_H_
#define _CIR_DW3000_H_

#include <stdlib.h>
#include <stdint.h>
#include <os/os.h>
#include <stats/stats.h>
#include <dw3000-c0/dw3000_dev.h>
#include <cir/cir.h>

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(CIR_STATS)
STATS_SECT_START(cir_dw3000_stat_section)
    STATS_SECT_ENTRY(complete)
STATS_SECT_END
#endif

typedef union{
    struct  _cir_dw3000_complex_t{
        int16_t real;
        int16_t imag;
    }__attribute__((__packed__));
    uint8_t array[sizeof(struct _cir_dw3000_complex_t)];
}cir_dw3000_complex_t;

typedef struct _cir_dw3000_t{
    uint8_t dummy;  //Errata
    struct _cir_dw3000_complex_t array[MYNEWT_VAL(CIR_MAX_SIZE)];
} __attribute__((packed, aligned(1))) cir_dw3000_t;


struct cir_dw3000_instance{
    struct cir_instance cir_inst;
    struct _dw3000_dev_instance_t * dev_inst; //!< Structure of DW3000_dev_instance
#if MYNEWT_VAL(CIR_STATS)
    STATS_SECT_DECL(cir_dw3000_stat_section) stat; //!< Stats instance
#endif
    uint16_t fp_amp1;
    dpl_float32_t fp_idx;
    dpl_float32_t fp_power;
    dpl_float32_t rcphase;
    dpl_float32_t angle;
    uint64_t raw_ts;
    uint8_t resampler_delay;
    uint16_t pacc_cnt;
#if MYNEWT_VAL(CIR_VERBOSE)
    struct dpl_event display_event;
#endif
    uint16_t length;
    uint16_t offset;
    cir_dw3000_t cir;
};

struct cir_dw3000_instance * cir_dw3000_init(struct _dw3000_dev_instance_t * inst, struct cir_dw3000_instance * cir);
dpl_float32_t cir_dw3000_remap_fp_index(struct cir_dw3000_instance *cir0, struct cir_dw3000_instance *cir1);
bool cir_dw3000_reread_from_cir(dw3000_dev_instance_t * inst, struct cir_dw3000_instance *master_cir);
void cir_dw3000_enable(struct cir_dw3000_instance * inst, bool mode);
void cir_dw3000_free(struct cir_dw3000_instance * inst);
dpl_float32_t cir_dw3000_get_pdoa(struct cir_dw3000_instance * master, struct cir_dw3000_instance *slave);

#ifdef __cplusplus
}
#endif

#endif /* _CIR_DW3000_H_ */
