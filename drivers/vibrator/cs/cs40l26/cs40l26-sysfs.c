// SPDX-License-Identifier: GPL-2.0
//
// cs40l26-syfs.c -- CS40L26 Boosted Haptic Driver with Integrated DSP and
// Waveform Memory with Advanced Closed Loop Algorithms and LRA protection
//
// Copyright 2021 Cirrus Logic, Inc.
//
// Author: Fred Treven <fred.treven@cirrus.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
#include <linux/vibrator/cs40l26.h>
#else
#include <linux/mfd/cs40l26.h>
#endif

static ssize_t dsp_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u8 dsp_state;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cs40l26_dsp_state_get(cs40l26, &dsp_state);

	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%u\n",
				(unsigned int) (dsp_state & 0xFF));
}
static DEVICE_ATTR_RO(dsp_state);

static ssize_t halo_heartbeat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, halo_heartbeat;
	int ret;

	ret = cl_dsp_get_reg(cs40l26->dsp, "HALO_HEARTBEAT",
			CL_DSP_XM_UNPACKED_TYPE, cs40l26->fw.id, &reg);
	if (ret)
		return ret;

	pm_runtime_get_sync(cs40l26->dev);

	ret = regmap_read(cs40l26->regmap, reg, &halo_heartbeat);

	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "%d\n", halo_heartbeat);
}
static DEVICE_ATTR_RO(halo_heartbeat);

static ssize_t fw_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret = 0;
	unsigned int mode;

	mutex_lock(&cs40l26->lock);

	if (cs40l26->fw_mode != CS40L26_FW_MODE_ROM
			&& cs40l26->fw_mode != CS40L26_FW_MODE_RAM) {
		dev_err(cs40l26->dev, "Invalid firmware mode: %u\n",
				cs40l26->fw_mode);
		ret = -EINVAL;
	} else {
		mode = cs40l26->fw_mode;
	}

	mutex_unlock(&cs40l26->lock);

	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "%u\n", mode);
}
static DEVICE_ATTR_RO(fw_mode);

static ssize_t pm_timeout_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 timeout_ms;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cs40l26_pm_timeout_ms_get(cs40l26, &timeout_ms);

	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "%u\n", timeout_ms);
}

static ssize_t pm_timeout_ms_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 timeout_ms;
	int ret;

	ret = kstrtou32(buf, 10, &timeout_ms);
	if (ret || timeout_ms < CS40L26_PM_TIMEOUT_MS_MIN)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cs40l26_pm_timeout_ms_set(cs40l26, timeout_ms);

	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_RW(pm_timeout_ms);

static ssize_t vibe_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	unsigned int state;

	mutex_lock(&cs40l26->lock);
	state = cs40l26->vibe_state;
	mutex_unlock(&cs40l26->lock);

	return snprintf(buf, PAGE_SIZE, "%u\n", state);
}
static DEVICE_ATTR_RO(vibe_state);

static ssize_t power_on_seq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	struct list_head *op_head = &cs40l26->pseq_op_head;
	u32 base = cs40l26->pseq_base;
	int i, count = 0;
	struct cs40l26_pseq_op *pseq_op;

	mutex_lock(&cs40l26->lock);

	if (list_empty(&cs40l26->pseq_op_head)) {
		dev_err(cs40l26->dev, "Power on sequence is empty\n");
		mutex_unlock(&cs40l26->lock);
		return -EINVAL;
	}

	list_for_each_entry_reverse(pseq_op, op_head, list) {
		dev_info(cs40l26->dev, "%d: Address: 0x%08X, Size: %d words\n",
			count + 1, base + pseq_op->offset, pseq_op->size);

		for (i = 0; i < pseq_op->size; i++)
			dev_info(cs40l26->dev, "0x%08X\n",
					*(pseq_op->words + i));

		count++;
	}

	mutex_unlock(&cs40l26->lock);

	if (count != cs40l26->pseq_num_ops) {
		dev_err(cs40l26->dev, "Malformed Power on seq.\n");
		return -EINVAL;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", cs40l26->pseq_num_ops);
}
static DEVICE_ATTR_RO(power_on_seq);

static ssize_t owt_free_space_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, words;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cl_dsp_get_reg(cs40l26->dsp, "OWT_SIZE_XM",
		CL_DSP_XM_UNPACKED_TYPE, CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_pm;

	ret = regmap_read(cs40l26->regmap, reg, &words);
	if (ret) {
		dev_err(cs40l26->dev, "Failed to get remaining OWT space\n");
		goto err_pm;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n", words * CL_DSP_BYTES_PER_WORD);

err_pm:
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}
static DEVICE_ATTR_RO(owt_free_space);

static ssize_t die_temp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	struct regmap *regmap = cs40l26->regmap;
	u16 die_temp;
	int ret;
	u32 val;

	pm_runtime_get_sync(cs40l26->dev);

	ret = regmap_read(regmap, CS40L26_GLOBAL_ENABLES, &val);
	if (ret) {
		dev_err(cs40l26->dev, "Failed to read GLOBAL_EN status\n");
		goto err_pm;
	}

	if (!(val & CS40L26_GLOBAL_EN_MASK)) {
		dev_err(cs40l26->dev,
			"Global enable must be set to get die temp.\n");
		ret = -EPERM;
		goto err_pm;
	}

	ret = regmap_read(regmap, CS40L26_ENABLES_AND_CODES_DIG, &val);
	if (ret) {
		dev_err(cs40l26->dev, "Failed to get die temperature\n");
		goto err_pm;
	}

	die_temp = (val & CS40L26_TEMP_RESULT_FILT_MASK) >>
			CS40L26_TEMP_RESULT_FILT_SHIFT;

	ret = snprintf(buf, PAGE_SIZE, "0x%03X\n", die_temp);

err_pm:
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}
static DEVICE_ATTR_RO(die_temp);

static ssize_t num_waves_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 nwaves;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cs40l26_get_num_waves(cs40l26, &nwaves);
	if (ret)
		goto err_pm;

	ret = snprintf(buf, PAGE_SIZE, "%u\n", nwaves);

err_pm:
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}
static DEVICE_ATTR_RO(num_waves);

/* boost_disable_delay is in units of 125us, e.g. 8 ->  1ms */
static ssize_t boost_disable_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, boost_disable_delay;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cl_dsp_get_reg(cs40l26->dsp, "BOOST_DISABLE_DELAY",
			CL_DSP_XM_UNPACKED_TYPE, CS40L26_EXT_ALGO_ID, &reg);
	if (ret)
		goto err_pm;

	ret = regmap_read(cs40l26->regmap, reg, &boost_disable_delay);
	if (ret)
		goto err_pm;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", boost_disable_delay);

err_pm:
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}

static ssize_t boost_disable_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, boost_disable_delay;
	int ret;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s: %s", __func__, buf);
#else
	dev_dbg(cs40l26->dev, "%s: %s", __func__, buf);
#endif

	ret = kstrtou32(buf, 10, &boost_disable_delay);

	if (ret ||
		boost_disable_delay < CS40L26_BOOST_DISABLE_DELAY_MIN ||
		boost_disable_delay > CS40L26_BOOST_DISABLE_DELAY_MAX)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);

	ret = cl_dsp_get_reg(cs40l26->dsp, "BOOST_DISABLE_DELAY",
			CL_DSP_XM_UNPACKED_TYPE, CS40L26_EXT_ALGO_ID, &reg);
	if (ret)
		goto err_pm;

	ret = regmap_write(cs40l26->regmap, reg, boost_disable_delay);

err_pm:
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return count;
}
static DEVICE_ATTR_RW(boost_disable_delay);

static ssize_t f0_offset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	unsigned int reg, val;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "F0_OFFSET",
			CL_DSP_XM_UNPACKED_TYPE, CS40L26_VIBEGEN_ALGO_ID,
			&reg);
	if (ret)
		goto err_mutex;


	ret = regmap_read(cs40l26->regmap, reg, &val);
	if (ret)
		goto err_mutex;

	ret = snprintf(buf, PAGE_SIZE, "%u\n", val);

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}

static ssize_t f0_offset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	unsigned int reg, val;
	int ret;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val > CS40L26_F0_OFFSET_MAX && val < CS40L26_F0_OFFSET_MIN)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "F0_OFFSET",
			CL_DSP_XM_UNPACKED_TYPE, CS40L26_VIBEGEN_ALGO_ID,
			&reg);
	if (ret)
		goto err_mutex;

	ret = regmap_write(cs40l26->regmap, reg, val);
	if (ret)
		goto err_mutex;

	ret = count;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}
static DEVICE_ATTR_RW(f0_offset);

static ssize_t delay_before_stop_playback_us_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&cs40l26->lock);

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
					cs40l26->delay_before_stop_playback_us);

	mutex_unlock(&cs40l26->lock);

	return ret;
}

static ssize_t delay_before_stop_playback_us_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 val;
	int ret;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	mutex_lock(&cs40l26->lock);

	cs40l26->delay_before_stop_playback_us = val;

	mutex_unlock(&cs40l26->lock);

	return count;
}
static DEVICE_ATTR_RW(delay_before_stop_playback_us);

static struct attribute *cs40l26_dev_attrs[] = {
	&dev_attr_num_waves.attr,
	&dev_attr_die_temp.attr,
	&dev_attr_owt_free_space.attr,
	&dev_attr_power_on_seq.attr,
	&dev_attr_dsp_state.attr,
	&dev_attr_halo_heartbeat.attr,
	&dev_attr_fw_mode.attr,
	&dev_attr_pm_timeout_ms.attr,
	&dev_attr_vibe_state.attr,
	&dev_attr_boost_disable_delay.attr,
	&dev_attr_f0_offset.attr,
	&dev_attr_delay_before_stop_playback_us.attr,
	NULL,
};

struct attribute_group cs40l26_dev_attr_group = {
	.name = "default",
	.attrs = cs40l26_dev_attrs,
};

static ssize_t trigger_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 mailbox_command, calibration_request_payload;
	int ret;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s: %s", __func__, buf);
#else
	dev_dbg(cs40l26->dev, "%s: %s", __func__, buf);
#endif

	ret = kstrtou32(buf, 16, &calibration_request_payload);
	if (ret ||
		calibration_request_payload < 1 ||
		calibration_request_payload > 2)
		return -EINVAL;

	mailbox_command = ((CS40L26_DSP_MBOX_CMD_INDEX_CALIBRATION_CONTROL <<
				CS40L26_DSP_MBOX_CMD_INDEX_SHIFT) &
				CS40L26_DSP_MBOX_CMD_INDEX_MASK) |
				(calibration_request_payload &
				CS40L26_DSP_MBOX_CMD_PAYLOAD_MASK);

	/* pm_runtime_put occurs is irq_handler after diagnostic is finished */
	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cs40l26_ack_write(cs40l26, CS40L26_DSP_VIRTUAL1_MBOX_1,
		mailbox_command, CS40L26_DSP_MBOX_RESET);

	if (ret) {
		dev_err(cs40l26->dev, "Failed to request calibration\n");
		cs40l26->cal_requested = 0;
	} else {
		cs40l26->cal_requested = calibration_request_payload;
		ret = count;
	}

	mutex_unlock(&cs40l26->lock);

	return ret;
}
static DEVICE_ATTR_WO(trigger_calibration);

static ssize_t f0_measured_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, f0_measured;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "F0_EST",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_F0_EST_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &f0_measured);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", f0_measured);
}
static DEVICE_ATTR_RO(f0_measured);

static ssize_t q_measured_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, q_measured;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "Q_EST",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_F0_EST_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &q_measured);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", q_measured);
}
static DEVICE_ATTR_RO(q_measured);

static ssize_t redc_measured_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, redc_measured;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "RE_EST_STATUS",
			CL_DSP_YM_UNPACKED_TYPE,
			CS40l26_SVC_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &redc_measured);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", redc_measured);
}
static DEVICE_ATTR_RO(redc_measured);

static ssize_t redc_est_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, redc_est;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "REDC",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_F0_EST_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &redc_est);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", redc_est);
}

static ssize_t redc_est_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, redc_est;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s: %s", __func__, buf);
#else
	dev_dbg(cs40l26->dev, "%s: %s", __func__, buf);
#endif

	ret = kstrtou32(buf, 16, &redc_est);
	if (ret)
		return ret;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "REDC",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_F0_EST_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_write(cs40l26->regmap, reg, redc_est);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return count;
}
static DEVICE_ATTR_RW(redc_est);

static ssize_t f0_stored_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, f0_stored;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "F0_OTP_STORED",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &f0_stored);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", f0_stored);
}

static ssize_t f0_stored_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, f0_stored;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s: %s", __func__, buf);
#else
	dev_dbg(cs40l26->dev, "%s: %s", __func__, buf);
#endif

	ret = kstrtou32(buf, 16, &f0_stored);

	if (ret ||
		f0_stored < CS40L26_F0_EST_MIN ||
		f0_stored > CS40L26_F0_EST_MAX)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "F0_OTP_STORED",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_write(cs40l26->regmap, reg, f0_stored);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return count;
}
static DEVICE_ATTR_RW(f0_stored);

static ssize_t q_stored_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, q_stored;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "Q_STORED",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &q_stored);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", q_stored);
}

static ssize_t q_stored_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, q_stored;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s: %s", __func__, buf);
#else
	dev_dbg(cs40l26->dev, "%s: %s", __func__, buf);
#endif

	ret = kstrtou32(buf, 16, &q_stored);

	if (ret || q_stored < CS40L26_Q_EST_MIN || q_stored > CS40L26_Q_EST_MAX)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "Q_STORED",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_write(cs40l26->regmap, reg, q_stored);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return count;
}
static DEVICE_ATTR_RW(q_stored);

static ssize_t redc_stored_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, redc_stored;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "REDC_OTP_STORED",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &redc_stored);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%08X\n", redc_stored);
}

static ssize_t redc_stored_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, redc_stored;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s: %s", __func__, buf);
#else
	dev_dbg(cs40l26->dev, "%s: %s", __func__, buf);
#endif

	ret = kstrtou32(buf, 16, &redc_stored);
	if (ret)
		return ret;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "REDC_OTP_STORED",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_VIBEGEN_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_write(cs40l26->regmap, reg, redc_stored);
	if (ret)
		goto err_mutex;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return count;
}
static DEVICE_ATTR_RW(redc_stored);

static ssize_t f0_and_q_cal_time_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, freq_span, freq_centre, f0_and_q_cal_time_ms;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "FREQ_SPAN",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_F0_EST_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &freq_span);
	if (ret)
		goto err_mutex;

	ret = cl_dsp_get_reg(cs40l26->dsp, "FREQ_CENTRE",
			CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_F0_EST_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &freq_centre);

	f0_and_q_cal_time_ms = ((CS40L26_F0_CHIRP_DURATION_FACTOR *
				(freq_span >> CS40L26_F0_EST_FREQ_SHIFT)) /
				(freq_centre >> CS40L26_F0_EST_FREQ_SHIFT)) +
				CS40L26_F0_AND_Q_CALIBRATION_BUFFER_MS;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", f0_and_q_cal_time_ms);
}
static DEVICE_ATTR_RO(f0_and_q_cal_time_ms);

static ssize_t redc_cal_time_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/* FIRMWARE_STUMPY_CALIB_REDC_PLAYTIME_MS + SVC_INIT + buffer */
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	u32 reg, redc_playtime_ms, redc_total_cal_time_ms;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "REDC_PLAYTIME_MS",
			CL_DSP_XM_UNPACKED_TYPE,
			cs40l26->fw.id, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &redc_playtime_ms);

	redc_total_cal_time_ms = redc_playtime_ms +
					CS40L26_SVC_INITIALIZATION_PERIOD_MS +
					CS40L26_REDC_CALIBRATION_BUFFER_MS;

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", redc_total_cal_time_ms);
}
static DEVICE_ATTR_RO(redc_cal_time_ms);

static ssize_t logging_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, enable;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "ENABLE", CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_LOGGER_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg, &enable);
	if (ret)
		dev_err(cs40l26->dev, "Failed to read logging enable\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "%d\n", enable);

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return ret;
}

static ssize_t logging_en_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 src_val = CS40L26_LOGGER_SRC_SIZE_MASK;
	u32 src_mask = src_val | CS40L26_LOGGER_SRC_ID_MASK;
	u32 enable, reg;
	int ret;

	ret = kstrtou32(buf, 10, &enable);
	if (ret)
		return ret;

	if (enable != 0 && enable != 1)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "ENABLE", CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_LOGGER_ALGO_ID, &reg);
	if (ret)
		goto exit_mutex;

	ret = regmap_write(cs40l26->regmap, reg, enable);
	if (ret) {
		dev_err(cs40l26->dev, "Failed to %s logging\n",
				enable ? "enable" : "disable");
		goto exit_mutex;
	}

	if (!enable)
		goto exit_mutex;

	ret = cl_dsp_get_reg(cs40l26->dsp, "COUNT", CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_LOGGER_ALGO_ID, &reg);
	if (ret)
		goto exit_mutex;

	ret = regmap_write(cs40l26->regmap, reg, 2);
	if (ret) {
		dev_err(cs40l26->dev, "Failed to set up logging sources\n");
		goto exit_mutex;
	}

	ret = cl_dsp_get_reg(cs40l26->dsp, "SOURCE", CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_LOGGER_ALGO_ID, &reg);
	if (ret)
		goto exit_mutex;

	ret = regmap_update_bits(cs40l26->regmap, reg, src_mask, src_val |
			(1 << CS40L26_LOGGER_SRC_ID_SHIFT));
	if (ret) {
		dev_err(cs40l26->dev, "Failed to set BEMF Logger Source ID\n");
		goto exit_mutex;
	}

	ret = regmap_update_bits(cs40l26->regmap, reg + 4, src_mask, src_val |
			(2 << CS40L26_LOGGER_SRC_ID_SHIFT));
	if (ret)
		dev_err(cs40l26->dev, "Failed to set VBST Logger Source ID\n");

exit_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return count;
}
static DEVICE_ATTR_RW(logging_en);

static ssize_t logging_max_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 rst;
	int ret;

	ret = kstrtou32(buf, 10, &rst);
	if (ret)
		return ret;

	if (rst != 1)
		return -EINVAL;

	pm_runtime_get_sync(cs40l26->dev);

	cs40l26_ack_write(cs40l26, CS40L26_DSP_VIRTUAL1_MBOX_1,
		CS40L26_DSP_MBOX_CMD_LOGGER_MAX_RESET, CS40L26_DSP_MBOX_RESET);

	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	return count;
}
static DEVICE_ATTR_WO(logging_max_reset);

static ssize_t max_bemf_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{

	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, max_bemf;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "DATA", CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_LOGGER_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg + 4, &max_bemf);
	if (ret)
		dev_err(cs40l26->dev, "Failed to get max. back EMF\n");

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "0x%06X\n", max_bemf);
}
static DEVICE_ATTR_RO(max_bemf);

static ssize_t max_vbst_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{

	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	u32 reg, max_vbst;
	int ret;

	pm_runtime_get_sync(cs40l26->dev);
	mutex_lock(&cs40l26->lock);

	ret = cl_dsp_get_reg(cs40l26->dsp, "DATA", CL_DSP_XM_UNPACKED_TYPE,
			CS40L26_LOGGER_ALGO_ID, &reg);
	if (ret)
		goto err_mutex;

	ret = regmap_read(cs40l26->regmap, reg + 16, &max_vbst);
	if (ret)
		dev_err(cs40l26->dev, "Failed to get max. back EMF\n");

err_mutex:
	mutex_unlock(&cs40l26->lock);
	pm_runtime_mark_last_busy(cs40l26->dev);
	pm_runtime_put_autosuspend(cs40l26->dev);

	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "0x%06X\n", max_vbst);
}
static DEVICE_ATTR_RO(max_vbst);

static ssize_t calib_fw_load_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&cs40l26->lock);

	if (cs40l26->fw.id == CS40L26_FW_ID)
		ret = snprintf(buf, PAGE_SIZE, "%d\n", 0);
	else if (cs40l26->fw.id == CS40L26_FW_CALIB_ID)
		ret = snprintf(buf, PAGE_SIZE, "%d\n", 1);
	else
		ret = -EINVAL;

	mutex_unlock(&cs40l26->lock);

	return ret;
}

static ssize_t calib_fw_load_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cs40l26_private *cs40l26 = dev_get_drvdata(dev);
	int ret;
	unsigned int variant;

	ret = kstrtou32(buf, 10, &variant);
	if (ret)
		return ret;

#ifdef CONFIG_CS40L26_SAMSUNG_FEATURE
	dev_info(cs40l26->dev, "%s %d\n", __func__, variant);
#endif
	if (variant == 0)
		ret = cs40l26_fw_swap(cs40l26, CS40L26_FW_ID);
	else if (variant == 1)
		ret = cs40l26_fw_swap(cs40l26, CS40L26_FW_CALIB_ID);
	else
		ret = -EINVAL;

	return ret ? ret : count;
}
static DEVICE_ATTR_RW(calib_fw_load);

static struct attribute *cs40l26_dev_attrs_cal[] = {
	&dev_attr_calib_fw_load.attr,
	&dev_attr_max_vbst.attr,
	&dev_attr_max_bemf.attr,
	&dev_attr_logging_max_reset.attr,
	&dev_attr_logging_en.attr,
	&dev_attr_trigger_calibration.attr,
	&dev_attr_f0_measured.attr,
	&dev_attr_q_measured.attr,
	&dev_attr_redc_measured.attr,
	&dev_attr_redc_est.attr,
	&dev_attr_f0_stored.attr,
	&dev_attr_q_stored.attr,
	&dev_attr_redc_stored.attr,
	&dev_attr_f0_and_q_cal_time_ms.attr,
	&dev_attr_redc_cal_time_ms.attr,
	NULL,
};

struct attribute_group cs40l26_dev_attr_cal_group = {
	.name = "calibration",
	.attrs = cs40l26_dev_attrs_cal,
};
