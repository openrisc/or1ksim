/* gpio.h -- GPIO code simulation

   Copyright (C) 2001 Erez Volk, erez@mailandnews.comopencores.org
   Copyright (C) 2008 Embecosm Limited

   Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>

   This file is part of Or1ksim, the OpenRISC 1000 Architectural Simulator.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This program is commented throughout in a fashion suitable for processing
   with Doxygen. */


/* Autoconf and/or portability configuration */
#include "config.h"
#include "port.h"

/* System includes */
#include <stdlib.h>

/* Package includes */
#include "sim-config.h"
#include "arch.h"
#include "vapi.h"
#include "sched.h"
#include "pic.h"
#include "abstract.h"
#include "toplevel-support.h"
#include "sim-cmd.h"
#include "gpio.h"


/*
 * The various VAPI IDs each GPIO device has
 */
enum
{ GPIO_VAPI_DATA = 0,
  GPIO_VAPI_AUX,
  GPIO_VAPI_CLOCK,
  GPIO_VAPI_RGPIO_OE,
  GPIO_VAPI_RGPIO_INTE,
  GPIO_VAPI_RGPIO_PTRIG,
  GPIO_VAPI_RGPIO_AUX,
  GPIO_VAPI_RGPIO_CTRL,
  GPIO_NUM_VAPI_IDS
};

/*
 * Implementatino of GPIO Code Registers and State
 */
struct gpio_device
{
  /* Is peripheral enabled */
  int enabled;

  /* Base address in memory */
  oraddr_t baseaddr;

  /* Which IRQ to generate */
  int irq;

  /* Which GPIO is this? */
  unsigned gpio_number;

  /* VAPI IDs */
  unsigned long base_vapi_id;

  /* Auxiliary inputs */
  unsigned long auxiliary_inputs;

  /* Visible registers */
  struct
  {
    unsigned long in;
    unsigned long out;
    unsigned long oe;
    unsigned long inte;
    unsigned long ptrig;
    unsigned long aux;
    unsigned long ctrl;
    unsigned long ints;

    int external_clock;
  } curr, next;
};

static void gpio_vapi_read (unsigned long id, unsigned long data, void *dat);
static void gpio_external_clock (unsigned long value,
				 struct gpio_device *gpio);
static void gpio_device_clock (struct gpio_device *gpio);
static void gpio_clock (void *dat);

/* Initialize all parameters and state */
static void
gpio_reset (void *dat)
{
  struct gpio_device *gpio = dat;

  if (gpio->baseaddr != 0)
    {
      /* Possibly connect to VAPI */
      if (gpio->base_vapi_id)
	{
	  vapi_install_multi_handler (gpio->base_vapi_id, GPIO_NUM_VAPI_IDS,
				      gpio_vapi_read, dat);
	}
    }
  SCHED_ADD (gpio_clock, dat, 1);
}


/* Dump status */
static void
gpio_status (void *dat)
{
  struct gpio_device *gpio = dat;

  if (gpio->baseaddr == 0)
    return;

  PRINTF ("\nGPIO at 0x%" PRIxADDR ":\n", gpio->baseaddr);
  PRINTF ("RGPIO_IN     : 0x%08lX\n", gpio->curr.in);
  PRINTF ("RGPIO_OUT    : 0x%08lX\n", gpio->curr.out);
  PRINTF ("RGPIO_OE     : 0x%08lX\n", gpio->curr.oe);
  PRINTF ("RGPIO_INTE   : 0x%08lX\n", gpio->curr.inte);
  PRINTF ("RGPIO_PTRIG  : 0x%08lX\n", gpio->curr.ptrig);
  PRINTF ("RGPIO_AUX    : 0x%08lX\n", gpio->curr.aux);
  PRINTF ("RGPIO_CTRL   : 0x%08lX\n", gpio->curr.ctrl);
  PRINTF ("RGPIO_INTS   : 0x%08lX\n", gpio->curr.ints);
}


/* Wishbone read */
static uint32_t
gpio_read32 (oraddr_t addr, void *dat)
{
  struct gpio_device *gpio = dat;

  switch (addr)
    {
    case RGPIO_IN:
      return gpio->curr.in | gpio->curr.out;
    case RGPIO_OUT:
      return gpio->curr.out;
    case RGPIO_OE:
      return gpio->curr.oe;
    case RGPIO_INTE:
      return gpio->curr.inte;
    case RGPIO_PTRIG:
      return gpio->curr.ptrig;
    case RGPIO_AUX:
      return gpio->curr.aux;
    case RGPIO_CTRL:
      return gpio->curr.ctrl;
    case RGPIO_INTS:
      return gpio->curr.ints;
    }

  return 0;
}


/* Wishbone write */
static void
gpio_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  struct gpio_device *gpio = dat;

  switch (addr)
    {
    case RGPIO_IN:
      break;
    case RGPIO_OUT:
      gpio->next.out = value;
      break;
    case RGPIO_OE:
      gpio->next.oe = value;
      break;
    case RGPIO_INTE:
      gpio->next.inte = value;
      break;
    case RGPIO_PTRIG:
      gpio->next.ptrig = value;
      break;
    case RGPIO_AUX:
      gpio->next.aux = value;
      break;
    case RGPIO_CTRL:
      gpio->next.ctrl = value;
      break;
    case RGPIO_INTS:
      if (gpio->next.ints && !value)
	clear_interrupt (gpio->irq);
      gpio->next.ints = value;
      break;
    }
}


/* Input from "outside world" */
static void
gpio_vapi_read (unsigned long id, unsigned long data, void *dat)
{
  unsigned which;
  struct gpio_device *gpio = dat;

  which = id - gpio->base_vapi_id;

  switch (which)
    {
    case GPIO_VAPI_DATA:
      gpio->next.in = data;
      break;
    case GPIO_VAPI_AUX:
      gpio->auxiliary_inputs = data;
      break;
    case GPIO_VAPI_RGPIO_OE:
      gpio->next.oe = data;
      break;
    case GPIO_VAPI_RGPIO_INTE:
      gpio->next.inte = data;
      break;
    case GPIO_VAPI_RGPIO_PTRIG:
      gpio->next.ptrig = data;
      break;
    case GPIO_VAPI_RGPIO_AUX:
      gpio->next.aux = data;
      break;
    case GPIO_VAPI_RGPIO_CTRL:
      gpio->next.ctrl = data;
      break;
    case GPIO_VAPI_CLOCK:
      gpio_external_clock (data, gpio);
      break;
    }
}

/* System Clock. */
static void
gpio_clock (void *dat)
{
  struct gpio_device *gpio = dat;

  /* Clock the device */
  if (!(gpio->curr.ctrl & RGPIO_CTRL_ECLK))
    gpio_device_clock (gpio);
  SCHED_ADD (gpio_clock, dat, 1);
}

/* External Clock. */
static void
gpio_external_clock (unsigned long value, struct gpio_device *gpio)
{
  int use_external_clock =
    ((gpio->curr.ctrl & RGPIO_CTRL_ECLK) == RGPIO_CTRL_ECLK);
  int negative_edge = ((gpio->curr.ctrl & RGPIO_CTRL_NEC) == RGPIO_CTRL_NEC);

  /* "Normalize" clock value */
  value = (value != 0);

  gpio->next.external_clock = value;

  if (use_external_clock
      && (gpio->next.external_clock != gpio->curr.external_clock)
      && (value != negative_edge))
    /* Make sure that in vapi_read, we don't clock the device */
    if (gpio->curr.ctrl & RGPIO_CTRL_ECLK)
      gpio_device_clock (gpio);
}

/* Report an interrupt to the sim */
static void
gpio_do_int (void *dat)
{
  struct gpio_device *gpio = dat;

  report_interrupt (gpio->irq);
}

/* Clock as handld by one device. */
static void
gpio_device_clock (struct gpio_device *gpio)
{
  /* Calculate new inputs and outputs */
  gpio->next.in &= ~gpio->next.oe;	/* Only input bits */
  /* Replace requested output bits with aux input */
  gpio->next.out =
    (gpio->next.out & ~gpio->next.aux) | (gpio->auxiliary_inputs & gpio->next.
					  aux);
  gpio->next.out &= gpio->next.oe;	/* Only output-enabled bits */

  /* If any outputs changed, notify the world (i.e. vapi) */
  if (gpio->next.out != gpio->curr.out)
    {
      if (gpio->base_vapi_id)
	vapi_send (gpio->base_vapi_id + GPIO_VAPI_DATA, gpio->next.out);
    }

  /* If any inputs changed and interrupt enabled, generate interrupt */
  if (gpio->next.in != gpio->curr.in)
    {
      if (gpio->next.ctrl & RGPIO_CTRL_INTE)
	{
	  unsigned changed_bits = gpio->next.in ^ gpio->curr.in;	/* inputs that have changed */
	  unsigned set_bits = changed_bits & gpio->next.in;	/* inputs that have been set */
	  unsigned cleared_bits = changed_bits & gpio->curr.in;	/* inputs that have been cleared */
	  unsigned relevant_bits =
	    (gpio->next.ptrig & set_bits) | (~gpio->next.
					     ptrig & cleared_bits);

	  if (relevant_bits & gpio->next.inte)
	    {
	      gpio->next.ctrl |= RGPIO_CTRL_INTS;
	      gpio->next.ints |= relevant_bits & gpio->next.inte;
	      /* Since we can't report an interrupt during a readmem/writemem
	       * schedule the scheduler to do it.  Read the comment above
	       * report_interrupt in pic/pic.c */
	      SCHED_ADD (gpio_do_int, gpio, 1);
	    }
	}
    }

  /* Switch to values for next clock */
  memcpy (&(gpio->curr), &(gpio->next), sizeof (gpio->curr));
}

/*---------------------------------------------------[ GPIO configuration ]---*/
static void
gpio_baseaddr (union param_val val, void *dat)
{
  struct gpio_device *gpio = dat;
  gpio->baseaddr = val.addr_val;
}

static void
gpio_irq (union param_val val, void *dat)
{
  struct gpio_device *gpio = dat;
  gpio->irq = val.int_val;
}

static void
gpio_base_vapi_id (union param_val val, void *dat)
{
  struct gpio_device *gpio = dat;
  gpio->base_vapi_id = val.int_val;
}

static void
gpio_enabled (union param_val val, void *dat)
{
  struct gpio_device *gpio = dat;
  gpio->enabled = val.int_val;
}

static void *
gpio_sec_start (void)
{
  struct gpio_device *new = malloc (sizeof (struct gpio_device));

  if (!new)
    {
      fprintf (stderr, "Peripheral gpio: Run out of memory\n");
      exit (-1);
    }

  new->auxiliary_inputs = 0;
  memset (&new->curr, 0, sizeof (new->curr));
  memset (&new->next, 0, sizeof (new->next));

  new->enabled = 1;
  new->baseaddr = 0;
  new->irq = 0;
  new->base_vapi_id = 0;

  return new;
}

static void
gpio_sec_end (void *dat)
{
  struct gpio_device *gpio = dat;
  struct mem_ops ops;

  if (!gpio->enabled)
    {
      free (dat);
      return;
    }

  memset (&ops, 0, sizeof (struct mem_ops));

  ops.readfunc32 = gpio_read32;
  ops.writefunc32 = gpio_write32;
  ops.write_dat32 = dat;
  ops.read_dat32 = dat;

  /* FIXME: Correct delays? */
  ops.delayr = 2;
  ops.delayw = 2;

  /* Register memory range */
  reg_mem_area (gpio->baseaddr, GPIO_ADDR_SPACE, 0, &ops);

  reg_sim_reset (gpio_reset, dat);
  reg_sim_stat (gpio_status, dat);
}

void
reg_gpio_sec (void)
{
  struct config_section *sec =
    reg_config_sec ("gpio", gpio_sec_start, gpio_sec_end);

  reg_config_param (sec, "enabled",      PARAMT_INT, gpio_enabled);
  reg_config_param (sec, "baseaddr",     PARAMT_ADDR, gpio_baseaddr);
  reg_config_param (sec, "irq",          PARAMT_INT, gpio_irq);
  reg_config_param (sec, "vapi_id",      PARAMT_INT, gpio_base_vapi_id);
  reg_config_param (sec, "base_vapi_id", PARAMT_INT, gpio_base_vapi_id);
}
