/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_bt

#if !defined(_TRACE_MTK_BT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MTK_BT_H

#include <linux/string.h>
#include <linux/tracepoint.h>

TRACE_EVENT(mtk_bt_hci,
	TP_PROTO(bool tx, const u8 *data, size_t len),
	TP_ARGS(tx, data, len),
	TP_STRUCT__entry(
		__field(bool, tx)
		__field(size_t, len)
		__dynamic_array(u8, data, len)
	),
	TP_fast_assign(
		__entry->tx = tx;
		__entry->len = len;
		memcpy(__get_dynamic_array(data), data, len);
	),
	TP_printk("%s len=%zu data=%s",
		  __entry->tx ? "tx" : "rx", __entry->len,
		  __print_hex(__get_dynamic_array(data), __entry->len))
);

#endif

#include <trace/define_trace.h>
