// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2012-2021, The Linux Foundation. All rights reserved.
 * Copyright (C) 2021 XiaoMi, Inc.
 */

/* Combined RX/TX MI2S ABI from MiCode lisa-r-oss
 * 57ee2b068000ac79caa7c076d0d9bb1a9260e8a4.
 * Included only for CONFIG_TARGET_PRODUCT_LISA; separate-direction
 * configurations continue using the newer implementation.
 */

static int msm_dai_q6_mi2s_format_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct msm_dai_q6_dai_data *dai_data = kcontrol->private_data;
	int value = ucontrol->value.integer.value[0];

	dai_data->port_config.i2s.data_format = value;
	pr_debug("%s: value = %d, channel = %d, line = %d\n",
		 __func__, value, dai_data->port_config.i2s.mono_stereo,
		 dai_data->port_config.i2s.channel_mode);
	return 0;
}

static int msm_dai_q6_mi2s_format_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct msm_dai_q6_dai_data *dai_data = kcontrol->private_data;

	ucontrol->value.integer.value[0] =
		dai_data->port_config.i2s.data_format;
	return 0;
}

static int msm_dai_q6_mi2s_vi_feed_mono_put(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct msm_dai_q6_dai_data *dai_data = kcontrol->private_data;
	int value = ucontrol->value.integer.value[0];

	dai_data->vi_feed_mono = value;
	pr_debug("%s: value = %d\n", __func__, value);
	return 0;
}

static int msm_dai_q6_mi2s_vi_feed_mono_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	struct msm_dai_q6_dai_data *dai_data = kcontrol->private_data;

	ucontrol->value.integer.value[0] = dai_data->vi_feed_mono;
	return 0;
}

static const struct snd_kcontrol_new mi2s_config_controls[] = {
	SOC_ENUM_EXT("PRI MI2S RX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("SEC MI2S RX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("TERT MI2S RX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("QUAT MI2S RX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("QUIN MI2S RX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("SENARY MI2S RX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("PRI MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("SEC MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("TERT MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("QUAT MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("QUIN MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("SENARY MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
	SOC_ENUM_EXT("INT5 MI2S TX Format", mi2s_config_enum[0],
		     msm_dai_q6_mi2s_format_get,
		     msm_dai_q6_mi2s_format_put),
};

static const struct snd_kcontrol_new mi2s_vi_feed_controls[] = {
	SOC_ENUM_EXT("INT5 MI2S VI MONO", mi2s_config_enum[1],
		     msm_dai_q6_mi2s_vi_feed_mono_get,
		     msm_dai_q6_mi2s_vi_feed_mono_put),
};

static int msm_dai_q6_dai_mi2s_probe(struct snd_soc_dai *dai)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
			dev_get_drvdata(dai->dev);
	struct msm_mi2s_pdata *mi2s_pdata =
			(struct msm_mi2s_pdata *) dai->dev->platform_data;
	struct snd_kcontrol *kcontrol = NULL;
	int rc = 0;
	const struct snd_kcontrol_new *ctrl = NULL;
	const struct snd_kcontrol_new *vi_feed_ctrl = NULL;
	u16 dai_id = 0;

	dai->id = mi2s_pdata->intf_id;

	if (mi2s_dai_data->rx_dai.mi2s_dai_data.port_config.i2s.channel_mode) {
		if (dai->id == MSM_PRIM_MI2S)
			ctrl = &mi2s_config_controls[0];
		if (dai->id == MSM_SEC_MI2S)
			ctrl = &mi2s_config_controls[1];
		if (dai->id == MSM_TERT_MI2S)
			ctrl = &mi2s_config_controls[2];
		if (dai->id == MSM_QUAT_MI2S)
			ctrl = &mi2s_config_controls[3];
		if (dai->id == MSM_QUIN_MI2S)
			ctrl = &mi2s_config_controls[4];
		if (dai->id == MSM_SENARY_MI2S)
			ctrl = &mi2s_config_controls[5];
	}

	if (ctrl) {
		kcontrol = snd_ctl_new1(ctrl,
					&mi2s_dai_data->rx_dai.mi2s_dai_data);
		rc = snd_ctl_add(dai->component->card->snd_card, kcontrol);
		if (rc < 0) {
			dev_err(dai->dev, "%s: err add RX fmt ctl DAI = %s\n",
				__func__, dai->name);
			goto rtn;
		}
	}

	ctrl = NULL;
	if (mi2s_dai_data->tx_dai.mi2s_dai_data.port_config.i2s.channel_mode) {
		if (dai->id == MSM_PRIM_MI2S)
			ctrl = &mi2s_config_controls[6];
		if (dai->id == MSM_SEC_MI2S)
			ctrl = &mi2s_config_controls[7];
		if (dai->id == MSM_TERT_MI2S)
			ctrl = &mi2s_config_controls[8];
		if (dai->id == MSM_QUAT_MI2S)
			ctrl = &mi2s_config_controls[9];
		if (dai->id == MSM_QUIN_MI2S)
			ctrl = &mi2s_config_controls[10];
		if (dai->id == MSM_SENARY_MI2S)
			ctrl = &mi2s_config_controls[11];
		if (dai->id == MSM_INT5_MI2S)
			ctrl = &mi2s_config_controls[12];
	}

	if (ctrl) {
		rc = snd_ctl_add(dai->component->card->snd_card,
				snd_ctl_new1(ctrl,
				&mi2s_dai_data->tx_dai.mi2s_dai_data));
		if (rc < 0) {
			if (kcontrol)
				snd_ctl_remove(dai->component->card->snd_card,
						kcontrol);
			dev_err(dai->dev, "%s: err add TX fmt ctl DAI = %s\n",
				__func__, dai->name);
		}
	}

	if (dai->id == MSM_INT5_MI2S)
		vi_feed_ctrl = &mi2s_vi_feed_controls[0];

	if (vi_feed_ctrl) {
		rc = snd_ctl_add(dai->component->card->snd_card,
				snd_ctl_new1(vi_feed_ctrl,
				&mi2s_dai_data->tx_dai.mi2s_dai_data));

		if (rc < 0) {
			dev_err(dai->dev, "%s: err add TX vi feed channel ctl DAI = %s\n",
				__func__, dai->name);
		}
	}

	if (mi2s_dai_data->is_island_dai) {
		msm_mi2s_get_port_id(dai->id, SNDRV_PCM_STREAM_CAPTURE,
				     &dai_id);
		rc = msm_dai_q6_add_island_mx_ctls(
						dai->component->card->snd_card,
						dai->name, dai_id,
						(void *)mi2s_dai_data);
	}

	rc = msm_dai_q6_dai_add_route(dai);
rtn:
	return rc;
}


static int msm_dai_q6_dai_mi2s_remove(struct snd_soc_dai *dai)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
		dev_get_drvdata(dai->dev);
	int rc;

	/* If AFE port is still up, close it */
	if (test_bit(STATUS_PORT_STARTED,
		     mi2s_dai_data->rx_dai.mi2s_dai_data.status_mask)) {
		rc = afe_close(MI2S_RX); /* can block */
		if (rc < 0)
			dev_err(dai->dev, "fail to close MI2S_RX port\n");
		clear_bit(STATUS_PORT_STARTED,
			  mi2s_dai_data->rx_dai.mi2s_dai_data.status_mask);
	}
	if (test_bit(STATUS_PORT_STARTED,
		     mi2s_dai_data->tx_dai.mi2s_dai_data.status_mask)) {
		rc = afe_close(MI2S_TX); /* can block */
		if (rc < 0)
			dev_err(dai->dev, "fail to close MI2S_TX port\n");
		clear_bit(STATUS_PORT_STARTED,
			  mi2s_dai_data->tx_dai.mi2s_dai_data.status_mask);
	}
	return 0;
}

static int msm_dai_q6_mi2s_startup(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{

	return 0;
}

static int msm_mi2s_get_port_id(u32 mi2s_id, int stream, u16 *port_id)
{
	int ret = 0;

	switch (stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		switch (mi2s_id) {
		case MSM_PRIM_MI2S:
			*port_id = AFE_PORT_ID_PRIMARY_MI2S_RX;
			break;
		case MSM_SEC_MI2S:
			*port_id = AFE_PORT_ID_SECONDARY_MI2S_RX;
			break;
		case MSM_TERT_MI2S:
			*port_id = AFE_PORT_ID_TERTIARY_MI2S_RX;
			break;
		case MSM_QUAT_MI2S:
			*port_id = AFE_PORT_ID_QUATERNARY_MI2S_RX;
			break;
		case MSM_SEC_MI2S_SD1:
			*port_id = AFE_PORT_ID_SECONDARY_MI2S_RX_SD1;
			break;
		case MSM_QUIN_MI2S:
			*port_id = AFE_PORT_ID_QUINARY_MI2S_RX;
			break;
		case MSM_SENARY_MI2S:
			*port_id = AFE_PORT_ID_SENARY_MI2S_RX;
			break;
		case MSM_INT0_MI2S:
			*port_id = AFE_PORT_ID_INT0_MI2S_RX;
			break;
		case MSM_INT1_MI2S:
			*port_id = AFE_PORT_ID_INT1_MI2S_RX;
			break;
		case MSM_INT2_MI2S:
			*port_id = AFE_PORT_ID_INT2_MI2S_RX;
			break;
		case MSM_INT3_MI2S:
			*port_id = AFE_PORT_ID_INT3_MI2S_RX;
			break;
		case MSM_INT4_MI2S:
			*port_id = AFE_PORT_ID_INT4_MI2S_RX;
			break;
		case MSM_INT5_MI2S:
			*port_id = AFE_PORT_ID_INT5_MI2S_RX;
			break;
		case MSM_INT6_MI2S:
			*port_id = AFE_PORT_ID_INT6_MI2S_RX;
			break;
		default:
			pr_err("%s: playback err id 0x%x\n",
				__func__, mi2s_id);
			ret = -1;
			break;
		}
	break;
	case SNDRV_PCM_STREAM_CAPTURE:
		switch (mi2s_id) {
		case MSM_PRIM_MI2S:
			*port_id = AFE_PORT_ID_PRIMARY_MI2S_TX;
			break;
		case MSM_SEC_MI2S:
			*port_id = AFE_PORT_ID_SECONDARY_MI2S_TX;
			break;
		case MSM_TERT_MI2S:
			*port_id = AFE_PORT_ID_TERTIARY_MI2S_TX;
			break;
		case MSM_QUAT_MI2S:
			*port_id = AFE_PORT_ID_QUATERNARY_MI2S_TX;
			break;
		case MSM_QUIN_MI2S:
			*port_id = AFE_PORT_ID_QUINARY_MI2S_TX;
			break;
		case MSM_SENARY_MI2S:
			*port_id = AFE_PORT_ID_SENARY_MI2S_TX;
			break;
		case MSM_INT0_MI2S:
			*port_id = AFE_PORT_ID_INT0_MI2S_TX;
			break;
		case MSM_INT1_MI2S:
			*port_id = AFE_PORT_ID_INT1_MI2S_TX;
			break;
		case MSM_INT2_MI2S:
			*port_id = AFE_PORT_ID_INT2_MI2S_TX;
			break;
		case MSM_INT3_MI2S:
			*port_id = AFE_PORT_ID_INT3_MI2S_TX;
			break;
		case MSM_INT4_MI2S:
			*port_id = AFE_PORT_ID_INT4_MI2S_TX;
			break;
		case MSM_INT5_MI2S:
			*port_id = AFE_PORT_ID_INT5_MI2S_TX;
			break;
		case MSM_INT6_MI2S:
			*port_id = AFE_PORT_ID_INT6_MI2S_TX;
			break;
		default:
			pr_err("%s: capture err id 0x%x\n", __func__, mi2s_id);
			ret = -1;
			break;
		}
	break;
	default:
		pr_err("%s: default err %d\n", __func__, stream);
		ret = -1;
	break;
	}
	pr_debug("%s: port_id = 0x%x\n", __func__, *port_id);
	return ret;
}

static int msm_dai_q6_mi2s_prepare(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
		dev_get_drvdata(dai->dev);
	struct msm_dai_q6_dai_data *dai_data =
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
		 &mi2s_dai_data->rx_dai.mi2s_dai_data :
		 &mi2s_dai_data->tx_dai.mi2s_dai_data);
	u16 port_id = 0;
	int rc = 0;

	if (msm_mi2s_get_port_id(dai->id, substream->stream,
				 &port_id) != 0) {
		dev_err(dai->dev, "%s: Invalid Port ID 0x%x\n",
				__func__, port_id);
		return -EINVAL;
	}

	dev_dbg(dai->dev, "%s: dai id %d, afe port id = 0x%x\n"
		"dai_data->channels = %u sample_rate = %u\n", __func__,
		dai->id, port_id, dai_data->channels, dai_data->rate);

	if (!test_bit(STATUS_PORT_STARTED, dai_data->status_mask)) {
		/* PORT START should be set if prepare called
		 * in active state.
		 */
		rc = afe_port_start(port_id, &dai_data->port_config,
				    dai_data->rate);
		if (rc < 0)
			dev_err(dai->dev, "fail to open AFE port 0x%x\n",
				dai->id);
		else
			set_bit(STATUS_PORT_STARTED,
				dai_data->status_mask);
	}
	if (!test_bit(STATUS_PORT_STARTED, dai_data->hwfree_status)) {
		set_bit(STATUS_PORT_STARTED, dai_data->hwfree_status);
		dev_dbg(dai->dev, "%s: set hwfree_status to started\n",
				__func__);
	}
	return rc;
}

static int msm_dai_q6_mi2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
		dev_get_drvdata(dai->dev);
	struct msm_dai_q6_mi2s_dai_config *mi2s_dai_config =
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
		&mi2s_dai_data->rx_dai : &mi2s_dai_data->tx_dai);
	struct msm_dai_q6_dai_data *dai_data = &mi2s_dai_config->mi2s_dai_data;
	struct afe_param_id_i2s_cfg *i2s = &dai_data->port_config.i2s;

	dai_data->channels = params_channels(params);
	switch (dai_data->channels) {
	case 15:
	case 16:
		switch (mi2s_dai_config->pdata_mi2s_lines) {
		case AFE_PORT_I2S_16CHS:
			dai_data->port_config.i2s.channel_mode
				= AFE_PORT_I2S_16CHS;
			break;
		default:
			goto error_invalid_data;
		};
		break;
	case 13:
	case 14:
		switch (mi2s_dai_config->pdata_mi2s_lines) {
		case AFE_PORT_I2S_14CHS:
		case AFE_PORT_I2S_16CHS:
			dai_data->port_config.i2s.channel_mode
				= AFE_PORT_I2S_14CHS;
			break;
		default:
			goto error_invalid_data;
		};
		break;
	case 11:
	case 12:
		switch (mi2s_dai_config->pdata_mi2s_lines) {
		case AFE_PORT_I2S_12CHS:
		case AFE_PORT_I2S_14CHS:
		case AFE_PORT_I2S_16CHS:
			dai_data->port_config.i2s.channel_mode
				= AFE_PORT_I2S_12CHS;
			break;
		default:
			goto error_invalid_data;
		};
		break;
	case 9:
	case 10:
		switch (mi2s_dai_config->pdata_mi2s_lines) {
		case AFE_PORT_I2S_10CHS:
		case AFE_PORT_I2S_12CHS:
		case AFE_PORT_I2S_14CHS:
		case AFE_PORT_I2S_16CHS:
			dai_data->port_config.i2s.channel_mode
				= AFE_PORT_I2S_10CHS;
			break;
		default:
			goto error_invalid_data;
		};
		break;
	case 8:
	case 7:
		if (mi2s_dai_config->pdata_mi2s_lines < AFE_PORT_I2S_8CHS)
			goto error_invalid_data;
		else
			if (mi2s_dai_config->pdata_mi2s_lines
					== AFE_PORT_I2S_8CHS_2)
				dai_data->port_config.i2s.channel_mode =
						AFE_PORT_I2S_8CHS_2;
			else
				dai_data->port_config.i2s.channel_mode =
						AFE_PORT_I2S_8CHS;
		break;
	case 6:
	case 5:
		if (mi2s_dai_config->pdata_mi2s_lines < AFE_PORT_I2S_6CHS)
			goto error_invalid_data;
		dai_data->port_config.i2s.channel_mode = AFE_PORT_I2S_6CHS;
		break;
	case 4:
	case 3:
		switch (mi2s_dai_config->pdata_mi2s_lines) {
		case AFE_PORT_I2S_SD0:
		case AFE_PORT_I2S_SD1:
		case AFE_PORT_I2S_SD2:
		case AFE_PORT_I2S_SD3:
		case AFE_PORT_I2S_SD4:
		case AFE_PORT_I2S_SD5:
		case AFE_PORT_I2S_SD6:
		case AFE_PORT_I2S_SD7:
			goto error_invalid_data;
			break;
		case AFE_PORT_I2S_QUAD01:
		case AFE_PORT_I2S_QUAD23:
		case AFE_PORT_I2S_QUAD45:
		case AFE_PORT_I2S_QUAD67:
			dai_data->port_config.i2s.channel_mode =
				mi2s_dai_config->pdata_mi2s_lines;
			break;
		case AFE_PORT_I2S_8CHS_2:
			dai_data->port_config.i2s.channel_mode =
					AFE_PORT_I2S_QUAD45;
			break;
		default:
			dai_data->port_config.i2s.channel_mode =
					AFE_PORT_I2S_QUAD01;
			break;
		};
		break;
	case 2:
	case 1:
		if (mi2s_dai_config->pdata_mi2s_lines < AFE_PORT_I2S_SD0)
			goto error_invalid_data;
		switch (mi2s_dai_config->pdata_mi2s_lines) {
		case AFE_PORT_I2S_SD0:
		case AFE_PORT_I2S_SD1:
		case AFE_PORT_I2S_SD2:
		case AFE_PORT_I2S_SD3:
		case AFE_PORT_I2S_SD4:
		case AFE_PORT_I2S_SD5:
		case AFE_PORT_I2S_SD6:
		case AFE_PORT_I2S_SD7:
			dai_data->port_config.i2s.channel_mode =
				mi2s_dai_config->pdata_mi2s_lines;
			break;
		case AFE_PORT_I2S_QUAD01:
		case AFE_PORT_I2S_6CHS:
		case AFE_PORT_I2S_8CHS:
		case AFE_PORT_I2S_10CHS:
		case AFE_PORT_I2S_12CHS:
		case AFE_PORT_I2S_14CHS:
		case AFE_PORT_I2S_16CHS:
			if (dai_data->vi_feed_mono == SPKR_1)
				dai_data->port_config.i2s.channel_mode =
							AFE_PORT_I2S_SD0;
			else
				dai_data->port_config.i2s.channel_mode =
							AFE_PORT_I2S_SD1;
			break;
		case AFE_PORT_I2S_QUAD23:
			dai_data->port_config.i2s.channel_mode =
						AFE_PORT_I2S_SD2;
			break;
		case AFE_PORT_I2S_QUAD45:
			dai_data->port_config.i2s.channel_mode =
						AFE_PORT_I2S_SD4;
			break;
		case AFE_PORT_I2S_QUAD67:
			dai_data->port_config.i2s.channel_mode =
						AFE_PORT_I2S_SD6;
			break;
		}
		if (dai_data->channels == 2)
			dai_data->port_config.i2s.mono_stereo =
						MSM_AFE_CH_STEREO;
		else
			dai_data->port_config.i2s.mono_stereo = MSM_AFE_MONO;
		break;
	default:
		pr_err("%s: default err channels %d\n",
			__func__, dai_data->channels);
		goto error_invalid_data;
	}
	dai_data->rate = params_rate(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_SPECIAL:
		dai_data->port_config.i2s.bit_width = 16;
		dai_data->bitwidth = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_3LE:
		dai_data->port_config.i2s.bit_width = 24;
		dai_data->bitwidth = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		dai_data->port_config.i2s.bit_width = 32;
		dai_data->bitwidth = 32;
		break;
	default:
		pr_err("%s: format %d\n",
			__func__, params_format(params));
		return -EINVAL;
	}

	dai_data->port_config.i2s.i2s_cfg_minor_version =
			AFE_API_VERSION_I2S_CONFIG;
	dai_data->port_config.i2s.sample_rate = dai_data->rate;
	if ((test_bit(STATUS_PORT_STARTED,
	    mi2s_dai_data->rx_dai.mi2s_dai_data.status_mask) &&
	    test_bit(STATUS_PORT_STARTED,
	    mi2s_dai_data->rx_dai.mi2s_dai_data.hwfree_status)) ||
	    (test_bit(STATUS_PORT_STARTED,
	    mi2s_dai_data->tx_dai.mi2s_dai_data.status_mask) &&
	    test_bit(STATUS_PORT_STARTED,
	    mi2s_dai_data->tx_dai.mi2s_dai_data.hwfree_status))) {
		if ((mi2s_dai_data->tx_dai.mi2s_dai_data.rate !=
		    mi2s_dai_data->rx_dai.mi2s_dai_data.rate) ||
		   (mi2s_dai_data->rx_dai.mi2s_dai_data.bitwidth !=
		    mi2s_dai_data->tx_dai.mi2s_dai_data.bitwidth)) {
			dev_err(dai->dev, "%s: Error mismatch in HW params\n"
				"Tx sample_rate = %u bit_width = %hu\n"
				"Rx sample_rate = %u bit_width = %hu\n"
				, __func__,
				mi2s_dai_data->tx_dai.mi2s_dai_data.rate,
				mi2s_dai_data->tx_dai.mi2s_dai_data.bitwidth,
				mi2s_dai_data->rx_dai.mi2s_dai_data.rate,
				mi2s_dai_data->rx_dai.mi2s_dai_data.bitwidth);
			return -EINVAL;
		}
	}
	dev_dbg(dai->dev, "%s: dai id %d dai_data->channels = %d\n"
		"sample_rate = %u i2s_cfg_minor_version = 0x%x\n"
		"bit_width = %hu  channel_mode = 0x%x mono_stereo = %#x\n"
		"ws_src = 0x%x sample_rate = %u data_format = 0x%x\n"
		"reserved = %u\n", __func__, dai->id, dai_data->channels,
		dai_data->rate, i2s->i2s_cfg_minor_version, i2s->bit_width,
		i2s->channel_mode, i2s->mono_stereo, i2s->ws_src,
		i2s->sample_rate, i2s->data_format, i2s->reserved);

	return 0;

error_invalid_data:
	pr_err("%s: dai_data->channels = %d channel_mode = %d\n", __func__,
		 dai_data->channels, dai_data->port_config.i2s.channel_mode);
	return -EINVAL;
}


static int msm_dai_q6_mi2s_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
	dev_get_drvdata(dai->dev);

	if (test_bit(STATUS_PORT_STARTED,
	    mi2s_dai_data->rx_dai.mi2s_dai_data.status_mask) ||
	    test_bit(STATUS_PORT_STARTED,
	    mi2s_dai_data->tx_dai.mi2s_dai_data.status_mask)) {
		dev_err(dai->dev, "%s: err chg i2s mode while dai running",
			__func__);
		return -EPERM;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
	case SND_SOC_DAIFMT_CBM_CFS:
		mi2s_dai_data->rx_dai.mi2s_dai_data.port_config.i2s.ws_src = 1;
		mi2s_dai_data->tx_dai.mi2s_dai_data.port_config.i2s.ws_src = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		mi2s_dai_data->rx_dai.mi2s_dai_data.port_config.i2s.ws_src = 0;
		mi2s_dai_data->tx_dai.mi2s_dai_data.port_config.i2s.ws_src = 0;
		break;
	default:
		pr_err("%s: fmt %d\n",
			__func__, fmt & SND_SOC_DAIFMT_MASTER_MASK);
		return -EINVAL;
	}

	return 0;
}

static int msm_dai_q6_mi2s_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
			dev_get_drvdata(dai->dev);
	struct msm_dai_q6_dai_data *dai_data =
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
		 &mi2s_dai_data->rx_dai.mi2s_dai_data :
		 &mi2s_dai_data->tx_dai.mi2s_dai_data);

	if (test_bit(STATUS_PORT_STARTED, dai_data->hwfree_status)) {
		clear_bit(STATUS_PORT_STARTED, dai_data->hwfree_status);
		dev_dbg(dai->dev, "%s: clear hwfree_status\n", __func__);
	}
	return 0;
}

static void msm_dai_q6_mi2s_shutdown(struct snd_pcm_substream *substream,
				     struct snd_soc_dai *dai)
{
	struct msm_dai_q6_mi2s_dai_data *mi2s_dai_data =
			dev_get_drvdata(dai->dev);
	struct msm_dai_q6_dai_data *dai_data =
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
		 &mi2s_dai_data->rx_dai.mi2s_dai_data :
		 &mi2s_dai_data->tx_dai.mi2s_dai_data);
	 u16 port_id = 0;
	int rc = 0;

	if (msm_mi2s_get_port_id(dai->id, substream->stream,
				 &port_id) != 0) {
		dev_err(dai->dev, "%s: Invalid Port ID 0x%x\n",
				__func__, port_id);
	}

	dev_dbg(dai->dev, "%s: closing afe port id = 0x%x\n",
			__func__, port_id);

	if (test_bit(STATUS_PORT_STARTED, dai_data->status_mask)) {
		rc = afe_close(port_id);
		if (rc < 0)
			dev_err(dai->dev, "fail to close AFE port\n");
		clear_bit(STATUS_PORT_STARTED, dai_data->status_mask);
	}
	if (test_bit(STATUS_PORT_STARTED, dai_data->hwfree_status))
		clear_bit(STATUS_PORT_STARTED, dai_data->hwfree_status);
}

static struct snd_soc_dai_ops msm_dai_q6_mi2s_ops = {
	.startup	= msm_dai_q6_mi2s_startup,
	.prepare	= msm_dai_q6_mi2s_prepare,
	.hw_params	= msm_dai_q6_mi2s_hw_params,
	.hw_free	= msm_dai_q6_mi2s_hw_free,
	.set_fmt	= msm_dai_q6_mi2s_set_fmt,
	.shutdown	= msm_dai_q6_mi2s_shutdown,
};

/* Channel min and max are initialized base on platform data */
static struct snd_soc_dai_driver msm_dai_q6_mi2s_dai[] = {
	{
		.playback = {
			.stream_name = "Primary MI2S Playback",
			.aif_name = "PRI_MI2S_RX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |
				SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
				SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_352800 |
				SNDRV_PCM_RATE_384000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     384000,
		},
		.capture = {
			.stream_name = "Primary MI2S Capture",
			.aif_name = "PRI_MI2S_TX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "Primary MI2S",
		.id = MSM_PRIM_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "Secondary MI2S Playback",
			.aif_name = "SEC_MI2S_RX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.capture = {
			.stream_name = "Secondary MI2S Capture",
			.aif_name = "SEC_MI2S_TX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "Secondary MI2S",
		.id = MSM_SEC_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "Tertiary MI2S Playback",
			.aif_name = "TERT_MI2S_RX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.capture = {
			.stream_name = "Tertiary MI2S Capture",
			.aif_name = "TERT_MI2S_TX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "Tertiary MI2S",
		.id = MSM_TERT_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "Quaternary MI2S Playback",
			.aif_name = "QUAT_MI2S_RX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.capture = {
			.stream_name = "Quaternary MI2S Capture",
			.aif_name = "QUAT_MI2S_TX",
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |
				 SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |
				 SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |
				 SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |
				 SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "Quaternary MI2S",
		.id = MSM_QUAT_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "Quinary MI2S Playback",
			.aif_name = "QUIN_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_96000 |
			SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.capture = {
			.stream_name = "Quinary MI2S Capture",
			.aif_name = "QUIN_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "Quinary MI2S",
		.id = MSM_QUIN_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "Senary MI2S Playback",
			.aif_name = "SEN_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.capture = {
			.stream_name = "Senary MI2S Capture",
			.aif_name = "SENARY_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "Senary MI2S",
		.id = MSM_SENARY_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "Secondary MI2S Playback SD1",
			.aif_name = "SEC_MI2S_RX_SD1",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.id = MSM_SEC_MI2S_SD1,
	},
	{
		.playback = {
			.stream_name = "INT0 MI2S Playback",
			.aif_name = "INT0_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 |
			SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.capture = {
			.stream_name = "INT0 MI2S Capture",
			.aif_name = "INT0_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT0 MI2S",
		.id = MSM_INT0_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "INT1 MI2S Playback",
			.aif_name = "INT1_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.capture = {
			.stream_name = "INT1 MI2S Capture",
			.aif_name = "INT1_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT1 MI2S",
		.id = MSM_INT1_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "INT2 MI2S Playback",
			.aif_name = "INT2_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.capture = {
			.stream_name = "INT2 MI2S Capture",
			.aif_name = "INT2_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT2 MI2S",
		.id = MSM_INT2_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "INT3 MI2S Playback",
			.aif_name = "INT3_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.capture = {
			.stream_name = "INT3 MI2S Capture",
			.aif_name = "INT3_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT3 MI2S",
		.id = MSM_INT3_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "INT4 MI2S Playback",
			.aif_name = "INT4_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_96000 |
			SNDRV_PCM_RATE_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     192000,
		},
		.capture = {
			.stream_name = "INT4 MI2S Capture",
			.aif_name = "INT4_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT4 MI2S",
		.id = MSM_INT4_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "INT5 MI2S Playback",
			.aif_name = "INT5_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.capture = {
			.stream_name = "INT5 MI2S Capture",
			.aif_name = "INT5_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT5 MI2S",
		.id = MSM_INT5_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
	{
		.playback = {
			.stream_name = "INT6 MI2S Playback",
			.aif_name = "INT6_MI2S_RX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				SNDRV_PCM_FMTBIT_S24_LE |
				SNDRV_PCM_FMTBIT_S24_3LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.capture = {
			.stream_name = "INT6 MI2S Capture",
			.aif_name = "INT6_MI2S_TX",
			.rates = SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_8000 |
			SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
			.rate_min =     8000,
			.rate_max =     48000,
		},
		.ops = &msm_dai_q6_mi2s_ops,
		.name = "INT6 MI2S",
		.id = MSM_INT6_MI2S,
		.probe = msm_dai_q6_dai_mi2s_probe,
		.remove = msm_dai_q6_dai_mi2s_remove,
	},
};


static int msm_dai_q6_mi2s_get_lineconfig(u16 sd_lines, u16 *config_ptr,
					  unsigned int *ch_cnt)
{
	u8 num_of_sd_lines;

	num_of_sd_lines = num_of_bits_set(sd_lines);
	switch (num_of_sd_lines) {
	case 0:
		pr_debug("%s: no line is assigned\n", __func__);
		break;
	case 1:
		switch (sd_lines) {
		case MSM_MI2S_SD0:
			*config_ptr = AFE_PORT_I2S_SD0;
			break;
		case MSM_MI2S_SD1:
			*config_ptr = AFE_PORT_I2S_SD1;
			break;
		case MSM_MI2S_SD2:
			*config_ptr = AFE_PORT_I2S_SD2;
			break;
		case MSM_MI2S_SD3:
			*config_ptr = AFE_PORT_I2S_SD3;
			break;
		case MSM_MI2S_SD4:
			*config_ptr = AFE_PORT_I2S_SD4;
			break;
		case MSM_MI2S_SD5:
			*config_ptr = AFE_PORT_I2S_SD5;
			break;
		case MSM_MI2S_SD6:
			*config_ptr = AFE_PORT_I2S_SD6;
			break;
		case MSM_MI2S_SD7:
			*config_ptr = AFE_PORT_I2S_SD7;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 2:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1:
			*config_ptr = AFE_PORT_I2S_QUAD01;
			break;
		case MSM_MI2S_SD2 | MSM_MI2S_SD3:
			*config_ptr = AFE_PORT_I2S_QUAD23;
			break;
		case MSM_MI2S_SD4 | MSM_MI2S_SD5:
			*config_ptr = AFE_PORT_I2S_QUAD45;
			break;
		case MSM_MI2S_SD6 | MSM_MI2S_SD7:
			*config_ptr = AFE_PORT_I2S_QUAD67;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 3:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1 | MSM_MI2S_SD2:
			*config_ptr = AFE_PORT_I2S_6CHS;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 4:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1 | MSM_MI2S_SD2 | MSM_MI2S_SD3:
			*config_ptr = AFE_PORT_I2S_8CHS;
			break;
		case MSM_MI2S_SD4 | MSM_MI2S_SD5 | MSM_MI2S_SD6 | MSM_MI2S_SD7:
			*config_ptr = AFE_PORT_I2S_8CHS_2;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 5:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1 | MSM_MI2S_SD2
		   | MSM_MI2S_SD3 | MSM_MI2S_SD4:
			*config_ptr = AFE_PORT_I2S_10CHS;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 6:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1 | MSM_MI2S_SD2
		   | MSM_MI2S_SD3 | MSM_MI2S_SD4 | MSM_MI2S_SD5:
			*config_ptr = AFE_PORT_I2S_12CHS;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 7:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1 | MSM_MI2S_SD2 | MSM_MI2S_SD3
		   | MSM_MI2S_SD4 | MSM_MI2S_SD5 | MSM_MI2S_SD6:
			*config_ptr = AFE_PORT_I2S_14CHS;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	case 8:
		switch (sd_lines) {
		case MSM_MI2S_SD0 | MSM_MI2S_SD1 | MSM_MI2S_SD2 | MSM_MI2S_SD3
		   | MSM_MI2S_SD4 | MSM_MI2S_SD5 | MSM_MI2S_SD6 | MSM_MI2S_SD7:
			*config_ptr = AFE_PORT_I2S_16CHS;
			break;
		default:
			pr_err("%s: invalid SD lines %d\n",
				   __func__, sd_lines);
			goto error_invalid_data;
		}
		break;
	default:
		pr_err("%s: invalid SD lines %d\n", __func__, num_of_sd_lines);
		goto error_invalid_data;
	}
	*ch_cnt = num_of_sd_lines;
	return 0;

error_invalid_data:
	pr_err("%s: invalid data\n", __func__);
	return -EINVAL;
}

static u16 msm_dai_q6_mi2s_get_num_channels(u16 config)
{
	switch (config) {
	case AFE_PORT_I2S_SD0:
	case AFE_PORT_I2S_SD1:
	case AFE_PORT_I2S_SD2:
	case AFE_PORT_I2S_SD3:
	case AFE_PORT_I2S_SD4:
	case AFE_PORT_I2S_SD5:
	case AFE_PORT_I2S_SD6:
	case AFE_PORT_I2S_SD7:
		return 2;
	case AFE_PORT_I2S_QUAD01:
	case AFE_PORT_I2S_QUAD23:
	case AFE_PORT_I2S_QUAD45:
	case AFE_PORT_I2S_QUAD67:
		return 4;
	case AFE_PORT_I2S_6CHS:
		return 6;
	case AFE_PORT_I2S_8CHS:
	case AFE_PORT_I2S_8CHS_2:
		return 8;
	case AFE_PORT_I2S_10CHS:
		return 10;
	case AFE_PORT_I2S_12CHS:
		return 12;
	case AFE_PORT_I2S_14CHS:
		return 14;
	case AFE_PORT_I2S_16CHS:
		return 16;
	default:
		pr_err("%s: invalid config\n", __func__);
		return 0;
	}
}

static int msm_dai_q6_mi2s_platform_data_validation(
	struct platform_device *pdev, struct snd_soc_dai_driver *dai_driver)
{
	struct msm_dai_q6_mi2s_dai_data *dai_data = dev_get_drvdata(&pdev->dev);
	struct msm_mi2s_pdata *mi2s_pdata =
			(struct msm_mi2s_pdata *) pdev->dev.platform_data;
	unsigned int ch_cnt;
	int rc = 0;
	u16 sd_line;

	if (mi2s_pdata == NULL) {
		pr_err("%s: mi2s_pdata NULL", __func__);
		return -EINVAL;
	}

	rc = msm_dai_q6_mi2s_get_lineconfig(mi2s_pdata->rx_sd_lines,
					    &sd_line, &ch_cnt);
	if (rc < 0) {
		dev_err(&pdev->dev, "invalid MI2S RX sd line config\n");
		goto rtn;
	}

	if (ch_cnt) {
		dai_data->rx_dai.mi2s_dai_data.port_config.i2s.channel_mode =
		sd_line;
		dai_data->rx_dai.pdata_mi2s_lines = sd_line;
		dai_driver->playback.channels_min = 1;
		dai_driver->playback.channels_max = ch_cnt << 1;
	} else {
		dai_driver->playback.channels_min = 0;
		dai_driver->playback.channels_max = 0;
	}
	rc = msm_dai_q6_mi2s_get_lineconfig(mi2s_pdata->tx_sd_lines,
					    &sd_line, &ch_cnt);
	if (rc < 0) {
		dev_err(&pdev->dev, "invalid MI2S TX sd line config\n");
		goto rtn;
	}

	if (ch_cnt) {
		dai_data->tx_dai.mi2s_dai_data.port_config.i2s.channel_mode =
		sd_line;
		dai_data->tx_dai.pdata_mi2s_lines = sd_line;
		dai_driver->capture.channels_min = 1;
		dai_driver->capture.channels_max = ch_cnt << 1;
	} else {
		dai_driver->capture.channels_min = 0;
		dai_driver->capture.channels_max = 0;
	}

	dev_dbg(&pdev->dev, "%s: playback sdline 0x%x capture sdline 0x%x\n",
		__func__, dai_data->rx_dai.pdata_mi2s_lines,
		dai_data->tx_dai.pdata_mi2s_lines);
	dev_dbg(&pdev->dev, "%s: playback ch_max %d capture ch_mx %d\n",
		__func__, dai_driver->playback.channels_max,
		dai_driver->capture.channels_max);
rtn:
	return rc;
}

static const struct snd_soc_component_driver msm_q6_mi2s_dai_component = {
	.name		= "msm-dai-q6-mi2s",
};
static int msm_dai_q6_mi2s_dev_probe(struct platform_device *pdev)
{
	struct msm_dai_q6_mi2s_dai_data *dai_data;
	const char *q6_mi2s_dev_id = "qcom,msm-dai-q6-mi2s-dev-id";
	u32 tx_line = 0;
	u32  rx_line = 0;
	u32 mi2s_intf = 0;
	struct msm_mi2s_pdata *mi2s_pdata;
	int rc;

	rc = of_property_read_u32(pdev->dev.of_node, q6_mi2s_dev_id,
				  &mi2s_intf);
	if (rc) {
		dev_err(&pdev->dev,
			"%s: missing 0x%x in dt node\n", __func__, mi2s_intf);
		goto rtn;
	}

	dev_dbg(&pdev->dev, "dev name %s dev id 0x%x\n", dev_name(&pdev->dev),
		mi2s_intf);

	if ((mi2s_intf < MSM_MI2S_MIN || mi2s_intf > MSM_MI2S_MAX)
		|| (mi2s_intf >= ARRAY_SIZE(msm_dai_q6_mi2s_dai))) {
		dev_err(&pdev->dev,
			"%s: Invalid MI2S ID %u from Device Tree\n",
			__func__, mi2s_intf);
		rc = -ENXIO;
		goto rtn;
	}

	pdev->id = mi2s_intf;

	mi2s_pdata = kzalloc(sizeof(struct msm_mi2s_pdata), GFP_KERNEL);
	if (!mi2s_pdata) {
		rc = -ENOMEM;
		goto rtn;
	}

	rc = of_property_read_u32(pdev->dev.of_node, "qcom,msm-mi2s-rx-lines",
				  &rx_line);
	if (rc) {
		dev_err(&pdev->dev, "%s: Rx line from DT file %s\n", __func__,
			"qcom,msm-mi2s-rx-lines");
		goto free_pdata;
	}

	rc = of_property_read_u32(pdev->dev.of_node, "qcom,msm-mi2s-tx-lines",
				  &tx_line);
	if (rc) {
		dev_err(&pdev->dev, "%s: Tx line from DT file %s\n", __func__,
			"qcom,msm-mi2s-tx-lines");
		goto free_pdata;
	}
	dev_dbg(&pdev->dev, "dev name %s Rx line 0x%x , Tx ine 0x%x\n",
		dev_name(&pdev->dev), rx_line, tx_line);
	mi2s_pdata->rx_sd_lines = rx_line;
	mi2s_pdata->tx_sd_lines = tx_line;
	mi2s_pdata->intf_id = mi2s_intf;

	dai_data = kzalloc(sizeof(struct msm_dai_q6_mi2s_dai_data),
			   GFP_KERNEL);
	if (!dai_data) {
		rc = -ENOMEM;
		goto free_pdata;
	} else
		dev_set_drvdata(&pdev->dev, dai_data);

	rc = of_property_read_u32(pdev->dev.of_node,
				    "qcom,msm-dai-is-island-supported",
				    &dai_data->is_island_dai);
	if (rc)
		dev_dbg(&pdev->dev, "island supported entry not found\n");

	pdev->dev.platform_data = mi2s_pdata;

	rc = msm_dai_q6_mi2s_platform_data_validation(pdev,
			&msm_dai_q6_mi2s_dai[mi2s_intf]);
	if (rc < 0)
		goto free_dai_data;

	rc = snd_soc_register_component(&pdev->dev, &msm_q6_mi2s_dai_component,
	&msm_dai_q6_mi2s_dai[mi2s_intf], 1);
	if (rc < 0)
		goto err_register;
	return 0;

err_register:
	dev_err(&pdev->dev, "fail to msm_dai_q6_mi2s_dev_probe\n");
free_dai_data:
	kfree(dai_data);
free_pdata:
	kfree(mi2s_pdata);
rtn:
	return rc;
}

static int msm_dai_q6_mi2s_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

