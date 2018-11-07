#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/serial_core.h>

/**
 * of_get_rs485_mode() - Implement parsing rs485 properties
 * @np: uart node
 * @rs485conf: output parameter
 *
 * This function implements the device tree binding described in
 * Documentation/devicetree/bindings/serial/rs485.txt.
 *
 * Return: 0 on success, 1 if the node doesn't contain rs485 stuff.
 */
int of_get_rs485_mode(struct device_node *np, struct serial_rs485 *rs485conf)
{
	u32 rs485_delay[2];
	int ret;

	if (!IS_ENABLED(CONFIG_OF) || !np)
		return 1;

	ret = of_property_read_u32_array(np, "rs485-rts-delay", rs485_delay, 2);
	if (!ret) {
		rs485conf->delay_rts_before_send = rs485_delay[0];
		rs485conf->delay_rts_after_send = rs485_delay[1];
	} else {
		rs485conf->delay_rts_before_send = 0;
		rs485conf->delay_rts_after_send = 0;
	}

	/*
	 * clear full-duplex and enabled flags to get to a defined state with
	 * the two following properties.
	 */
	rs485conf->flags &= ~(SER_RS485_RX_DURING_TX | SER_RS485_ENABLED);

	if (of_property_read_bool(np, "rs485-rx-during-tx"))
		rs485conf->flags |= SER_RS485_RX_DURING_TX;

	if (of_property_read_bool(np, "linux,rs485-enabled-at-boot-time"))
		rs485conf->flags |= SER_RS485_ENABLED;

	return 0;
}
EXPORT_SYMBOL_GPL(of_get_rs485_mode);
