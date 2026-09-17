// SPDX-License-Identifier: GPL-2.0
/* Load-based CPU frequency profiles for Lucreticus. */

#include <linux/init.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "cpufreq_governor.h"

struct lucreti_profile {
	unsigned int target_load;
	unsigned int floor_load;
	unsigned int down_samples;
	unsigned int sampling_rate_us;
};

struct lucreti_policy {
	struct policy_dbs_info policy_dbs;
	unsigned int down_count;
};

struct lucreti_governor {
	struct dbs_governor dbs;
	const struct lucreti_profile *profile;
};

static const struct lucreti_profile perf_profile = {
	.target_load = 65,
	.floor_load = 15,
	.down_samples = 3,
	.sampling_rate_us = 10000,
};

static const struct lucreti_profile balance_profile = {
	.target_load = 80,
	.floor_load = 0,
	.down_samples = 2,
	.sampling_rate_us = 20000,
};

static const struct lucreti_profile battery_profile = {
	.target_load = 95,
	.floor_load = 0,
	.down_samples = 1,
	.sampling_rate_us = 40000,
};

static const struct lucreti_profile *lucreti_profile_of(struct cpufreq_policy *policy)
{
	struct dbs_governor *dbs = dbs_governor_of(policy);

	return container_of(dbs, struct lucreti_governor, dbs)->profile;
}

static unsigned int lucreti_update(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct lucreti_policy *state = container_of(policy_dbs,
						struct lucreti_policy, policy_dbs);
	struct dbs_data *data = policy_dbs->dbs_data;
	const struct lucreti_profile *profile = lucreti_profile_of(policy);
	unsigned int load = max(dbs_update(policy), profile->floor_load);
	unsigned int demand = min(100U, load * 100 / profile->target_load);
	unsigned int target = policy->min +
		div_u64((u64)(policy->max - policy->min) * demand, 100);

	/* Hold a higher frequency briefly without delaying a new load spike. */
	if (target < policy->cur && policy->cur <= policy->max &&
	    ++state->down_count < profile->down_samples)
		return data->sampling_rate;

	state->down_count = 0;
	if (target != policy->cur)
		__cpufreq_driver_target(policy, target, CPUFREQ_RELATION_C);

	return data->sampling_rate;
}

static struct policy_dbs_info *lucreti_alloc(void)
{
	struct lucreti_policy *state = kzalloc(sizeof(*state), GFP_KERNEL);

	return state ? &state->policy_dbs : NULL;
}

static void lucreti_free(struct policy_dbs_info *policy_dbs)
{
	kfree(container_of(policy_dbs, struct lucreti_policy, policy_dbs));
}

static int lucreti_init(struct dbs_data *data)
{
	data->ignore_nice_load = 0;
	data->io_is_busy = 0;
	data->sampling_down_factor = 1;
	return 0;
}

static void lucreti_exit(struct dbs_data *data)
{
	data->tuners = NULL;
}

static void lucreti_start(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct lucreti_policy *state = container_of(policy_dbs,
						struct lucreti_policy, policy_dbs);
	struct dbs_data *data = policy_dbs->dbs_data;
	const struct lucreti_profile *profile = lucreti_profile_of(policy);
	unsigned int minimum_rate = max(profile->sampling_rate_us,
				cpufreq_policy_transition_delay_us(policy));

	state->down_count = 0;
	data->sampling_rate = max(data->sampling_rate, minimum_rate);
}

static struct attribute *lucreti_attributes[] = { NULL };

#define LUCRETI_GOVERNOR(_name, _profile) { \
	.dbs = { \
		.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER(_name), \
		.kobj_type = { .default_attrs = lucreti_attributes }, \
		.gov_dbs_update = lucreti_update, \
		.alloc = lucreti_alloc, \
		.free = lucreti_free, \
		.init = lucreti_init, \
		.exit = lucreti_exit, \
		.start = lucreti_start, \
	}, \
	.profile = &_profile, \
}

static struct lucreti_governor perf_gov =
	LUCRETI_GOVERNOR("lucretiperf", perf_profile);
static struct lucreti_governor balance_gov =
	LUCRETI_GOVERNOR("lucretibalance", balance_profile);
static struct lucreti_governor battery_gov =
	LUCRETI_GOVERNOR("lucretibattery", battery_profile);

#if defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LUCRETIPERF)
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &perf_gov.dbs.gov;
}
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LUCRETIBATTERY)
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &battery_gov.dbs.gov;
}
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LUCRETIBALANCE)
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &balance_gov.dbs.gov;
}
#endif

static int __init lucreti_register(void)
{
	int ret;

	ret = cpufreq_register_governor(&perf_gov.dbs.gov);
	if (ret)
		return ret;

	ret = cpufreq_register_governor(&balance_gov.dbs.gov);
	if (ret)
		goto unregister_perf;

	ret = cpufreq_register_governor(&battery_gov.dbs.gov);
	if (!ret)
		return 0;

	cpufreq_unregister_governor(&balance_gov.dbs.gov);
unregister_perf:
	cpufreq_unregister_governor(&perf_gov.dbs.gov);
	return ret;
}
fs_initcall(lucreti_register);

MODULE_DESCRIPTION("Lucreticus performance, balance and battery CPUFreq governors");
MODULE_LICENSE("GPL");
