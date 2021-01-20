// SPDX-License-Identifier: GPL-2.0-only
/*
 * Rockchip OTP Driver
 *
 * Copyright (c) 2018 Rockchip Electronics Co. Ltd.
 * Author: Finley Xiao <finley.xiao@rock-chips.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

/* OTP Register Offsets */
#define OTPC_SBPI_CTRL			0x0020
#define OTPC_SBPI_CMD_VALID_PRE		0x0024
#define OTPC_SBPI_CS_VALID_PRE		0x0028
#define OTPC_SBPI_STATUS		0x002C
#define OTPC_USER_CTRL			0x0100
#define OTPC_USER_ADDR			0x0104
#define OTPC_USER_ENABLE		0x0108
#define OTPC_USER_Q			0x0124
#define OTPC_INT_STATUS			0x0304
#define OTPC_SBPI_CMD0_OFFSET		0x1000
#define OTPC_SBPI_CMD1_OFFSET		0x1004

/* OTP Register bits and masks */
#define OTPC_USER_ADDR_MASK		GENMASK(31, 16)
#define OTPC_USE_USER			BIT(0)
#define OTPC_USE_USER_MASK		GENMASK(16, 16)
#define OTPC_USER_FSM_ENABLE		BIT(0)
#define OTPC_USER_FSM_ENABLE_MASK	GENMASK(16, 16)
#define OTPC_SBPI_DONE			BIT(1)
#define OTPC_USER_DONE			BIT(2)

#define SBPI_DAP_ADDR			0x02
#define SBPI_DAP_ADDR_SHIFT		8
#define SBPI_DAP_ADDR_MASK		GENMASK(31, 24)
#define SBPI_CMD_VALID_MASK		GENMASK(31, 16)
#define SBPI_DAP_CMD_WRF		0xC0
#define SBPI_DAP_REG_ECC		0x3A
#define SBPI_ECC_ENABLE			0x00
#define SBPI_ECC_DISABLE		0x09
#define SBPI_ENABLE			BIT(0)
#define SBPI_ENABLE_MASK		GENMASK(16, 16)

#define OTPC_TIMEOUT			10000

#define RV1126_OTP_NVM_CEB		0x00
#define RV1126_OTP_NVM_RSTB		0x04
#define RV1126_OTP_NVM_ST		0x18
#define RV1126_OTP_NVM_RADDR		0x1C
#define RV1126_OTP_NVM_RSTART		0x20
#define RV1126_OTP_NVM_RDATA		0x24
#define RV1126_OTP_READ_ST		0x30

struct rockchip_otp {
	struct device *dev;
	void __iomem *base;
	struct clk_bulk_data *clks;
	int num_clks;
	struct reset_control *rst;
};

struct rockchip_data {
	int size;
	const char * const *clocks;
	int num_clks;
	nvmem_reg_read_t reg_read;
	int (*init)(struct rockchip_otp *otp);
};

static int rockchip_otp_reset(struct rockchip_otp *otp)
{
	int ret;

	ret = reset_control_assert(otp->rst);
	if (ret) {
		dev_err(otp->dev, "failed to assert otp phy %d\n", ret);
		return ret;
	}

	udelay(2);

	ret = reset_control_deassert(otp->rst);
	if (ret) {
		dev_err(otp->dev, "failed to deassert otp phy %d\n", ret);
		return ret;
	}

	return 0;
}

static int px30_otp_wait_status(struct rockchip_otp *otp, u32 flag)
{
	u32 status = 0;
	int ret;

	ret = readl_poll_timeout_atomic(otp->base + OTPC_INT_STATUS, status,
					(status & flag), 1, OTPC_TIMEOUT);
	if (ret)
		return ret;

	/* clean int status */
	writel(flag, otp->base + OTPC_INT_STATUS);

	return 0;
}

static int px30_otp_ecc_enable(struct rockchip_otp *otp, bool enable)
{
	int ret = 0;

	writel(SBPI_DAP_ADDR_MASK | (SBPI_DAP_ADDR << SBPI_DAP_ADDR_SHIFT),
	       otp->base + OTPC_SBPI_CTRL);

	writel(SBPI_CMD_VALID_MASK | 0x1, otp->base + OTPC_SBPI_CMD_VALID_PRE);
	writel(SBPI_DAP_CMD_WRF | SBPI_DAP_REG_ECC,
	       otp->base + OTPC_SBPI_CMD0_OFFSET);
	if (enable)
		writel(SBPI_ECC_ENABLE, otp->base + OTPC_SBPI_CMD1_OFFSET);
	else
		writel(SBPI_ECC_DISABLE, otp->base + OTPC_SBPI_CMD1_OFFSET);

	writel(SBPI_ENABLE_MASK | SBPI_ENABLE, otp->base + OTPC_SBPI_CTRL);

	ret = px30_otp_wait_status(otp, OTPC_SBPI_DONE);
	if (ret < 0)
		dev_err(otp->dev, "timeout during ecc_enable\n");

	return ret;
}

static int px30_otp_read(void *context, unsigned int offset, void *val,
			 size_t bytes)
{
	struct rockchip_otp *otp = context;
	u8 *buf = val;
	int ret = 0;

	ret = clk_bulk_prepare_enable(otp->num_clks, otp->clks);
	if (ret < 0) {
		dev_err(otp->dev, "failed to prepare/enable clks\n");
		return ret;
	}

	ret = rockchip_otp_reset(otp);
	if (ret) {
		dev_err(otp->dev, "failed to reset otp phy\n");
		goto disable_clks;
	}

	ret = px30_otp_ecc_enable(otp, false);
	if (ret < 0) {
		dev_err(otp->dev, "rockchip_otp_ecc_enable err\n");
		goto disable_clks;
	}

	writel(OTPC_USE_USER | OTPC_USE_USER_MASK, otp->base + OTPC_USER_CTRL);
	udelay(5);
	while (bytes--) {
		writel(offset++ | OTPC_USER_ADDR_MASK,
		       otp->base + OTPC_USER_ADDR);
		writel(OTPC_USER_FSM_ENABLE | OTPC_USER_FSM_ENABLE_MASK,
		       otp->base + OTPC_USER_ENABLE);
		ret = px30_otp_wait_status(otp, OTPC_USER_DONE);
		if (ret < 0) {
			dev_err(otp->dev, "timeout during read setup\n");
			goto read_end;
		}
		*buf++ = readb(otp->base + OTPC_USER_Q);
	}

read_end:
	writel(0x0 | OTPC_USE_USER_MASK, otp->base + OTPC_USER_CTRL);
disable_clks:
	clk_bulk_disable_unprepare(otp->num_clks, otp->clks);

	return ret;
}

static int rv1126_otp_init(struct rockchip_otp *otp)
{
	u32 status = 0;
	int ret;

	writel(0x0, otp->base + RV1126_OTP_NVM_CEB);
	ret = readl_poll_timeout_atomic(otp->base + RV1126_OTP_NVM_ST, status,
					status & 0x1, 1, OTPC_TIMEOUT);
	if (ret < 0) {
		dev_err(otp->dev, "timeout during set ceb\n");
		return ret;
	}

	writel(0x1, otp->base + RV1126_OTP_NVM_RSTB);
	ret = readl_poll_timeout_atomic(otp->base + RV1126_OTP_NVM_ST, status,
					status & 0x4, 1, OTPC_TIMEOUT);
	if (ret < 0) {
		dev_err(otp->dev, "timeout during set rstb\n");
		return ret;
	}

	return 0;
}

static int rv1126_otp_read(void *context, unsigned int offset, void *val,
			   size_t bytes)
{
	struct rockchip_otp *otp = context;
	u32 status = 0;
	u8 *buf = val;
	int ret = 0;

	while (bytes--) {
		writel(offset++, otp->base + RV1126_OTP_NVM_RADDR);
		writel(0x1, otp->base + RV1126_OTP_NVM_RSTART);
		ret = readl_poll_timeout_atomic(otp->base + RV1126_OTP_READ_ST,
						status, status == 0, 1,
						OTPC_TIMEOUT);
		if (ret < 0) {
			dev_err(otp->dev, "timeout during read setup\n");
			return ret;
		}

		*buf++ = readb(otp->base + RV1126_OTP_NVM_RDATA);
	}

	return 0;
}

static struct nvmem_config otp_config = {
	.name = "rockchip-otp",
	.owner = THIS_MODULE,
	.read_only = true,
	.stride = 1,
	.word_size = 1,
};

static const char * const px30_otp_clocks[] = {
	"otp", "apb_pclk", "phy",
};

static const struct rockchip_data px30_data = {
	.size = 0x40,
	.clocks = px30_otp_clocks,
	.num_clks = ARRAY_SIZE(px30_otp_clocks),
	.reg_read = px30_otp_read,
};

static const char * const rv1126_otp_clocks[] = {
	"otp", "apb_pclk",
};

static const struct rockchip_data rv1126_data = {
	.size = 0x200,
	.clocks = rv1126_otp_clocks,
	.num_clks = ARRAY_SIZE(rv1126_otp_clocks),
	.init = rv1126_otp_init,
	.reg_read = rv1126_otp_read,
};

static const struct of_device_id rockchip_otp_match[] = {
	{
		.compatible = "rockchip,px30-otp",
		.data = (void *)&px30_data,
	},
	{
		.compatible = "rockchip,rk3308-otp",
		.data = (void *)&px30_data,
	},
	{
		.compatible = "rockchip,rv1126-otp",
		.data = (void *)&rv1126_data,
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, rockchip_otp_match);

static int __init rockchip_otp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rockchip_otp *otp;
	const struct rockchip_data *data;
	struct nvmem_device *nvmem;
	int ret, i;

	data = of_device_get_match_data(dev);
	if (!data) {
		dev_err(dev, "failed to get match data\n");
		return -EINVAL;
	}

	otp = devm_kzalloc(&pdev->dev, sizeof(struct rockchip_otp),
			   GFP_KERNEL);
	if (!otp)
		return -ENOMEM;

	otp->dev = dev;
	otp->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(otp->base))
		return PTR_ERR(otp->base);

	otp->num_clks = data->num_clks;
	otp->clks = devm_kcalloc(dev, otp->num_clks,
				     sizeof(*otp->clks), GFP_KERNEL);
	if (!otp->clks)
		return -ENOMEM;

	for (i = 0; i < otp->num_clks; ++i)
		otp->clks[i].id = data->clocks[i];

	ret = devm_clk_bulk_get(dev, otp->num_clks, otp->clks);
	if (ret)
		return ret;

	otp->rst = devm_reset_control_array_get_optional_exclusive(dev);
	if (IS_ERR(otp->rst))
		return PTR_ERR(otp->rst);

	if (data->init) {
		ret = data->init(otp);
		if (ret)
			return ret;
	}

	otp_config.size = data->size;
	otp_config.reg_read = data->reg_read;
	otp_config.priv = otp;
	otp_config.dev = dev;
	nvmem = devm_nvmem_register(dev, &otp_config);

	return PTR_ERR_OR_ZERO(nvmem);
}

static struct platform_driver rockchip_otp_driver = {
	.driver = {
		.name = "rockchip-otp",
		.of_match_table = rockchip_otp_match,
	},
};

static int __init rockchip_otp_module_init(void)
{
	return platform_driver_probe(&rockchip_otp_driver,
				     rockchip_otp_probe);
}

subsys_initcall(rockchip_otp_module_init);

MODULE_DESCRIPTION("Rockchip OTP driver");
MODULE_LICENSE("GPL v2");
