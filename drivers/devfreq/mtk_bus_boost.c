// SPDX-License-Identifier: GPL-2.0
/* Input driven DDR boost for MediaTek's DVFSRC bus. */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

static bool enabled = true;
static unsigned int boost_opp = 1;
static unsigned int duration_ms = 100;
static unsigned int cooldown_ms = 16;
module_param(enabled, bool, 0644);
MODULE_PARM_DESC(enabled, "Enable touchscreen DDR boost");
module_param(boost_opp, uint, 0644);
MODULE_PARM_DESC(boost_opp, "MT6768 DDR OPP request (0 fastest, 2 slowest)");
module_param(duration_ms, uint, 0644);
MODULE_PARM_DESC(duration_ms, "Boost duration in milliseconds");
module_param(cooldown_ms, uint, 0644);
MODULE_PARM_DESC(cooldown_ms, "Minimum delay between touch boosts");

static struct pm_qos_request bus_boost_req;
static unsigned long last_boost;
#ifdef CONFIG_MTK_MALI_BOOST
extern void ged_dvfs_boost_gpu_freq(void);
#endif
static void mtk_bus_boost_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(bus_boost_work, mtk_bus_boost_work);

static void mtk_bus_boost_work(struct work_struct *work)
{
	unsigned int opp;
	unsigned int ms;

	if (!READ_ONCE(enabled))
		return;
	if (time_before(jiffies, READ_ONCE(last_boost) +
			msecs_to_jiffies(min(READ_ONCE(cooldown_ms), 1000U))))
		return;
	WRITE_ONCE(last_boost, jiffies);

	opp = min(READ_ONCE(boost_opp), 2U);
	ms = clamp(READ_ONCE(duration_ms), 20U, 1000U);
	pm_qos_update_request_timeout(&bus_boost_req, opp, ms * 1000UL);
#ifdef CONFIG_MTK_MALI_BOOST
	ged_dvfs_boost_gpu_freq();
#endif
}

void mtk_touch_boost_kick(void)
{
	if (READ_ONCE(enabled))
		mod_delayed_work(system_unbound_wq, &bus_boost_work, 0);
}
EXPORT_SYMBOL_GPL(mtk_touch_boost_kick);

static void mtk_bus_boost_event(struct input_handle *handle,
				unsigned int type, unsigned int code, int value)
{
	if (!READ_ONCE(enabled))
		return;

	/* Boost on contact, not on every coordinate update during a swipe. */
	if ((type == EV_ABS && code == ABS_MT_TRACKING_ID && value >= 0) ||
	    (type == EV_KEY && code == BTN_TOUCH && value > 0))
		mtk_touch_boost_kick();
}

static int mtk_bus_boost_connect(struct input_handler *handler,
				struct input_dev *dev,
				const struct input_device_id *id)
{
	struct input_handle *handle;
	int ret;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;
	handle->dev = dev;
	handle->handler = handler;
	handle->name = "mtk_bus_boost";

	ret = input_register_handle(handle);
	if (ret)
		goto free_handle;
	ret = input_open_device(handle);
	if (ret)
		goto unregister_handle;
	return 0;

unregister_handle:
	input_unregister_handle(handle);
free_handle:
	kfree(handle);
	return ret;
}

static void mtk_bus_boost_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id mtk_bus_boost_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_TRACKING_ID)] =
			BIT_MASK(ABS_MT_TRACKING_ID) },
	},
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
	},
	{ }
};

static struct input_handler mtk_bus_boost_handler = {
	.event = mtk_bus_boost_event,
	.connect = mtk_bus_boost_connect,
	.disconnect = mtk_bus_boost_disconnect,
	.name = "mtk_bus_boost",
	.id_table = mtk_bus_boost_ids,
};

static int __init mtk_bus_boost_init(void)
{
	int ret;

	pm_qos_add_request(&bus_boost_req, PM_QOS_DDR_OPP,
			   PM_QOS_DDR_OPP_DEFAULT_VALUE);
	ret = input_register_handler(&mtk_bus_boost_handler);
	if (ret)
		pm_qos_remove_request(&bus_boost_req);
	return ret;
}
late_initcall(mtk_bus_boost_init);
