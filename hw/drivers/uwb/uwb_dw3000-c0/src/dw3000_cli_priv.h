#ifndef _DW3000_CLI_PRIV_H_
#define _DW3000_CLI_PRIV_H_

#include <streamer/streamer.h>

void dw3000_cli_dump_registers(struct _dw3000_dev_instance_t * inst, struct streamer *streamer);
void dw3000_cli_dump_event_counters(struct _dw3000_dev_instance_t * inst, struct streamer *streamer);
void dw3000_cli_dump_address(struct _dw3000_dev_instance_t * inst, uint32_t addr, uint16_t length, struct streamer *streamer);
void dw3000_cli_backtrace(struct _dw3000_dev_instance_t * inst, uint16_t verbose, struct streamer *streamer);
void dw3000_cli_spi_backtrace(struct _dw3000_dev_instance_t * inst, uint16_t verbose, struct streamer *streamer);
void dw3000_cli_interrupt_backtrace(struct _dw3000_dev_instance_t * inst, uint16_t verbose, struct streamer *streamer);

#endif /* _DW3000_CLI_PRIV_H_ */
