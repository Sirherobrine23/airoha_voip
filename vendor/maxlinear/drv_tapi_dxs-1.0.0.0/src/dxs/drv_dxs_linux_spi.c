/* SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause) */
/* Linux SPI binding for the MaxLinear DXS TAPI backend. */
#ifdef LINUX
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include "drv_dxs_api.h"
#include "drv_dxs_linux.h"

struct dxs_spi_binding {
   DXS_DEVICE_t *dev;
   struct gpio_desc *reset;
};

static int dxs_spi_probe(struct spi_device *spi)
{
   struct dxs_spi_binding *binding;
   DXS_DEVICE_t *dev;
   u32 index = spi_get_chipselect(spi, 0), interval = 10;
   int ret;

   of_property_read_u32(spi->dev.of_node, "airoha,tapi-device", &index);
   if (index >= DXS_MAX_DEVICES ||
       DXS_GetDevice(index, &dev) != DXS_statusOk)
      return -EINVAL;
   if (dev->pSpiDev)
      return -EBUSY;
   binding = devm_kzalloc(&spi->dev, sizeof(*binding), GFP_KERNEL);
   if (!binding)
      return -ENOMEM;
   spi->mode = SPI_MODE_3;
   spi->bits_per_word = 8;
   ret = spi_setup(spi);
   if (ret)
      return ret;
   binding->reset = devm_gpiod_get_optional(&spi->dev, "reset", GPIOD_OUT_LOW);
   if (!binding->reset)
      binding->reset = devm_gpiod_get_optional(&spi->dev, "intel,reset", GPIOD_OUT_LOW);
   if (IS_ERR(binding->reset))
      return dev_err_probe(&spi->dev, PTR_ERR(binding->reset), "cannot get DXS reset\n");
   if (of_property_read_u32(spi->dev.of_node, "reset-interval-ms", &interval))
      of_property_read_u32(spi->dev.of_node, "intel,reset-interval", &interval);
   if (interval < GPIO_RESET_INTERVAL_MIN_VALUE || interval > GPIO_RESET_INTERVAL_MAX_VALUE)
      return -EINVAL;
   binding->dev = dev;
   dev->pSpiDev = spi;
   dev->pResetGpio = binding->reset;
   dev->nResetInterval = interval;
   spi_set_drvdata(spi, binding);
   dev_info(&spi->dev, "DXS TAPI device %u bound (reset %s, IRQ %d)\n",
            index, binding->reset ? "GPIO" : "external", spi->irq);
   return 0;
}

static void dxs_spi_remove(struct spi_device *spi)
{
   struct dxs_spi_binding *binding = spi_get_drvdata(spi);
   /* Stateful device links remove the voice consumer before its supplier. */
   binding->dev->pSpiDev = NULL;
   binding->dev->pResetGpio = NULL;
   spi_set_drvdata(spi, NULL);
}

static const struct of_device_id dxs_spi_of_match[] = {
   { .compatible = "maxlinear,pef32001" },
   { .compatible = "maxlinear,pef32002" },
   { .compatible = "lantiq,duslicxs" },
   { }
};
MODULE_DEVICE_TABLE(of, dxs_spi_of_match);
static const struct spi_device_id dxs_spi_ids[] = {
   { "pef32001", 0 }, { "pef32002", 0 }, { "duslicxs", 0 }, { }
};
MODULE_DEVICE_TABLE(spi, dxs_spi_ids);
static struct spi_driver dxs_spi_driver = {
   .driver = { .name = "duslicxs", .of_match_table = dxs_spi_of_match },
   .id_table = dxs_spi_ids,
   .probe = dxs_spi_probe,
   .remove = dxs_spi_remove,
};

int DXS_SPI_ModuleInit(void)
{
   return spi_register_driver(&dxs_spi_driver);
}

void DXS_SPI_ModuleExit(void)
{
   spi_unregister_driver(&dxs_spi_driver);
}

/* Binding happens in probe, not in every BASICDEV_INIT call. Reinitializing
 * a device must not increment a shared registration reference count. */
IFX_void_t DXS_SPI_drvRegister(DXS_DEVICE_t *dev) { TAPI_UNUSED(dev); }
IFX_void_t DXS_SPI_drvUnregister(DXS_DEVICE_t *dev) { TAPI_UNUSED(dev); }
#endif
