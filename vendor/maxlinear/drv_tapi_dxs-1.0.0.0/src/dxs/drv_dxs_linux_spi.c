#ifdef LINUX

#include <linux/spi/spi.h>

#ifdef __LINUX_SPI_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>

/* Modified in 2026 for standard reset GPIO bindings and PEF compatibles. */

#include "drv_dxs_api.h"
#include "drv_dxs_linux.h"

/* ========================================================================== */
/*                           Local macros                                     */
/* ========================================================================== */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0))
   #define SPI_RETURN_TYPE  void
   #define SPI_RETURN_VALUE /* nothing */
#else
   #define SPI_RETURN_TYPE  int
   #define SPI_RETURN_VALUE 0
#endif

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

IFX_void_t DXS_SPI_drvRegister(DXS_DEVICE_t *pDev);
IFX_void_t DXS_SPI_drvUnregister(DXS_DEVICE_t *pDev);

static int DXS_SPI_Probe(struct spi_device *pSpiDev);
static SPI_RETURN_TYPE DXS_SPI_Remove(struct spi_device *pSpiDev);

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

struct DXS_SPI_DEV {
   struct spi_device *spi;
   struct list_head  device_entry;
   #ifdef TAPI_LINUX_KERNEL_SPACE
      struct gpio_desc  *reset_gpio;
      unsigned int       reset_interval;
   #endif /* TAPI_LINUX_KERNEL_SPACE */
} __PACKED__;

/* ========================================================================== */
/*                             Local variables                                */
/* ========================================================================== */

static IFX_uint32_t nSpiUsage = 0;

static LIST_HEAD(device_list);
static TAPI_OS_mutex_t device_list_lock;

static const struct of_device_id dxs_match_table[] =
   {
      {.compatible = "maxlinear,pef32001"},
      {.compatible = "maxlinear,pef32002"},
      {.compatible = "lantiq,duslicxs"},
      {}
   };

MODULE_DEVICE_TABLE(of, dxs_match_table);

static struct spi_driver sDxsSpiDriver = {
   .driver = {
      .owner = THIS_MODULE,
      .name = "duslicxs",
      .of_match_table = of_match_ptr(dxs_match_table),
   },
   .probe = DXS_SPI_Probe,
   .remove = DXS_SPI_Remove,
};

/**
   SPI device driver setup and teardown

   Binds this driver to the spi device. Drivers can verify
   that the device is actually present, and may need to configure
   characteristics (such as bits_per_word) which weren't needed for
   the initial configuration done during system setup.

   \param  pSpiDev      Pointer to the SPI device.

   \return
   <  0 if incorrect spi device used
   >= 0 if successful
 */
/*lint -save -esym(515,strcmp) -esym(516,strcmp) -e746 */
static int DXS_SPI_Probe(struct spi_device *pSpiDev)
{
   /* Allocate spi device manager data */
   struct DXS_SPI_DEV *spidev = TAPI_OS_Malloc(sizeof(*spidev));
   if (!spidev)
      return -ENOMEM;

   /* Initialize spi device manager data */
   memset(spidev, 0, sizeof(*spidev));
   spidev->spi = pSpiDev;

   INIT_LIST_HEAD(&spidev->device_entry);

#ifdef TAPI_LINUX_KERNEL_SPACE
   /* Get GPIO from device tree */
   /*
    * New bindings use the standard reset-gpios property. Keep the original
    * Intel-prefixed property as a compatibility fallback for old board DTS.
    */
   spidev->reset_gpio = gpiod_get_optional(&pSpiDev->dev, "reset",
                                           GPIOD_OUT_HIGH);
   if (!spidev->reset_gpio)
      spidev->reset_gpio = gpiod_get_optional(&pSpiDev->dev, "intel,reset",
                                              GPIOD_OUT_HIGH);

   if (IS_ERR_OR_NULL(spidev->reset_gpio))
   {
      dev_info(&pSpiDev->dev,
         "reset-gpios missing or invalid, reset GPIO will not be allocated from DT\n");
   }
   else
   {
      int ret = of_property_read_u32(pSpiDev->dev.of_node,
                                     "reset-interval-ms",
                                     &spidev->reset_interval);
      if (ret)
         ret = of_property_read_u32(pSpiDev->dev.of_node,
                                    "intel,reset-interval",
                                    &spidev->reset_interval);
      if (ret != 0 ||
          spidev->reset_interval < GPIO_RESET_INTERVAL_MIN_VALUE ||
          spidev->reset_interval > GPIO_RESET_INTERVAL_MAX_VALUE)
      {
         dev_err(&pSpiDev->dev,
            "reset-interval-ms missing or invalid. Please provide value in range of %d and %dms. "
            "Using default 10ms value.",
            GPIO_RESET_INTERVAL_MIN_VALUE,
            GPIO_RESET_INTERVAL_MAX_VALUE);

         spidev->reset_interval = 10;
      }

#ifdef DEBUG
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0))
      /* If debug is enabled and we are in kernel space, export GPIO to sysfs */
      if (gpiod_export(spidev->reset_gpio, false) != 0)
      {
         dev_info(&pSpiDev->dev, "GPIO export failed\n");
      }
#else
   #warning "Linux kernel version is too old. GPIO export is not supported"
#endif
#endif /* DEBUG */
   }
#endif /* TAPI_LINUX_KERNEL_SPACE */

   /* Add spi device to the common list */
   TAPI_OS_MutexGet(&device_list_lock);
   list_add_tail(&spidev->device_entry, &device_list);
   TAPI_OS_MutexRelease(&device_list_lock);

   spi_set_drvdata(pSpiDev, spidev);

   return 0;
}
/*lint -restore */

/**
   SPI device driver setup and teardown

   Unbinds this driver from the spi device.

   \param  pSpiDev      Pointer to the SPI device.

   \return
   <  0 if incorrect spi device used
   >= 0 if successful
 */
static SPI_RETURN_TYPE DXS_SPI_Remove(struct spi_device *pSpiDev)
{
   struct DXS_SPI_DEV *spidev = IFX_NULL;

   TAPI_OS_MutexGet(&device_list_lock);

   spidev = (struct DXS_SPI_DEV *) spi_get_drvdata(pSpiDev);

   if (IFX_NULL != spidev)
   {
#ifdef TAPI_LINUX_KERNEL_SPACE
      if (!IS_ERR_OR_NULL(spidev->reset_gpio))
      {
#ifdef DEBUG
      #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0))
         gpiod_unexport(spidev->reset_gpio);
      #else
         #warning "Linux kernel version is too old. GPIO export is not supported"
      #endif
#endif /* DEBUG */

         gpiod_put(spidev->reset_gpio);
         spidev->reset_gpio = IFX_NULL;
      }
#endif /* TAPI_LINUX_KERNEL_SPACE */

      /* make sure ops can abort cleanly */
      spi_set_drvdata(spidev->spi, IFX_NULL);
      spidev->spi = IFX_NULL;

      /* prevent new opens */
      list_del(&spidev->device_entry);
      TAPI_OS_Free(spidev);
   }

   TAPI_OS_MutexRelease(&device_list_lock);

   return SPI_RETURN_VALUE;
}

/**
   Register SPI driver

   \param  pDev      Pointer to device structure.
*/
/*lint -save -esym(413, spidev) */
IFX_void_t DXS_SPI_drvRegister(DXS_DEVICE_t *pDev)
{
   IFX_uint32_t dev_num = 0;
   struct DXS_SPI_DEV *spidev = IFX_NULL;

   /* for first usage do SPI driver initialization */
   if (0 == nSpiUsage)
   {
      TAPI_OS_MutexInit(&device_list_lock);

      /* register driver to capture SPI devices */
      spi_register_driver (&sDxsSpiDriver);
   }

   nSpiUsage ++;

   TAPI_OS_MutexGet (&device_list_lock);
   /* mark as used spi device */
   list_for_each_entry(spidev, &device_list, device_entry)
   {
      if (pDev->nDevNr == dev_num)
      {
         pDev->pSpiDev = spidev->spi;
#ifdef TAPI_LINUX_KERNEL_SPACE
         pDev->pResetGpio = spidev->reset_gpio;
         pDev->nResetInterval = spidev->reset_interval;
#endif /* TAPI_LINUX_KERNEL_SPACE */
      }
      dev_num++;
   }
   TAPI_OS_MutexRelease (&device_list_lock);
}
/*lint -restore */


/**
   Unregister SPI driver

   \param  pDev      Pointer to device structure.
*/
IFX_void_t DXS_SPI_drvUnregister(DXS_DEVICE_t *pDev)
{
   struct DXS_SPI_DEV *spidev = IFX_NULL;
   IFX_boolean_t isRegistered = IFX_FALSE;

   TAPI_OS_MutexGet (&device_list_lock);

   /* mark as unused spi device */
   list_for_each_entry(spidev, &device_list, device_entry)
   {
      if (pDev->pSpiDev == spidev->spi)
      {
         isRegistered = IFX_TRUE;
         pDev->pSpiDev = IFX_NULL;
         nSpiUsage--;
      }
   }

   TAPI_OS_MutexRelease (&device_list_lock);

   /* unregister SPI driver after last usage */
   if (isRegistered && (0 == nSpiUsage))
   {
      /* unregister driver if no any users */
      spi_unregister_driver (&sDxsSpiDriver);

      TAPI_OS_MutexDelete (&device_list_lock);
   }
}

#endif /* __LINUX_SPI_H */
#endif /* LINUX */
