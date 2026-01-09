/*
 * Copyright (c) 2019-2020,2024-2025
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* DOC:
 * https://lwn.net/ml/linux-kernel/20240115004847.22369-2-warthog618@gmail.com/
 */

/* Kernel: 4.19.75-v7+
 * BUG_1: using OPEN_DRAIN or OPEN_SOURCE disable setting the line
 *        https://github.com/raspberrypi/linux/commit/410ab742a50348afa389d55f3c7bf03538ce4210#diff-a8583939a10364379827fe5c47f52dbf
 */

#if defined(BITTERS_WITH_GPIO_IRQ) && !defined(BITTERS_WITH_THREADS)
#error BITTERS_WITH_GPIO_IRQ requires BITTERS_WITH_THREADS
#endif

#ifndef _GNU_SOURCE
#error GNU extensions are required, please at -D_GNU_SOURCE to your compiler
#endif

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>

#include <string.h>
#include <errno.h>
#include <linux/gpio.h>

#include "bitters.h"
#include "bitters/gpio.h"
#include "queue.h"

#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
#include <pthread.h>
#include <signal.h>
#endif




/*== Log & Assert helpers ==============================================*/

#if defined(BITTERS_GPIO_WITH_LOG)
#define BITTERS_GPIO_LOG(x, ...) 					\
    BITTERS_LOG("gpio: " x, ##__VA_ARGS__)
#else
#define BITTERS_GPIO_LOG(x, ...)
#endif

#if defined(BITTERS_GPIO_WITH_ASSERT)
#define BITTERS_GPIO_ASSERT(x)						\
    assert(x)
#else
#define BITTERS_GPIO_ASSERT(x)
#endif

#define BITTERS_GPIO_ASSERT_PIN(x)					\
    BITTERS_GPIO_ASSERT(((x) != NULL) &&				\
		((x)->id >= 0) && ((x)->id < 100) &&			\
		((x)->ctrl_devname != NULL))

#define BITTERS_GPIO_ASSERT_DIR(x)					\
    BITTERS_GPIO_ASSERT(((x) == BITTERS_GPIO_DIR_INPUT) ||		\
			((x) == BITTERS_GPIO_DIR_OUTPUT))

#define BITTERS_GPIO_ASSERT_INTERRUPT(x)				\
    BITTERS_GPIO_ASSERT(((x) == BITTERS_GPIO_INTERRUPT_DISABLED    ) ||	\
                        ((x) == BITTERS_GPIO_INTERRUPT_RISING_EDGE ) || \
                        ((x) == BITTERS_GPIO_INTERRUPT_FALLING_EDGE) || \
                        ((x) == BITTERS_GPIO_INTERRUPT_BOTH_EDGE   ))

#define BITTERS_GPIO_ASSERT_PIN_ASSOCIATED(pin)				\
    BITTERS_GPIO_ASSERT((pin != NULL) && (pin->ctrl != NULL))




/*== Macros ============================================================*/

#define BITTERS_GPIO_ENSURE_ASSOCIATED_PIN(pin)				\
    do {								\
        int rc = _bitters_gpio_pin_ensure_associated(pin);		\
	if (rc < 0)							\
	    return rc;							\
    } while(0)

#define BITTERS_GPIO_ENSURE_INTERRUPT_PIN(pin)				\
    do {								\
	if (! (pin->flags & GPIO_PIN_FLAG_INTERRUPT)) {			\
	    return -EINVAL;						\
	}								\
    } while(0)


/*== Structures ========================================================*/

struct bitters_gpio_ctrl {
    LIST_ENTRY(bitters_gpio_ctrl) entries; /* link list entry		*/
    char *name;			   	/* gpio device name		*/
    int   fd;				/* file descriptor on device	*/
    int   refcount;			/* number of pin associated	*/
#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
    pthread_t irq_thread;		// thread for "irq" processing
    int   lines;			// controller pin count
    struct pollfd *fds;
    bitters_gpio_pin_t **pins;
#endif
};




/*== Local variables ===================================================*/

static LIST_HEAD(, bitters_gpio_ctrl) bitters_gpio_ctrls =
    LIST_HEAD_INITIALIZER(bitters_gpio_ctrls);




/*== Internal functions ================================================*/

static void
_bitters_gpio_sigirq(int a) {
    /* Nothing */
}


static int
_bitters_gpio_pin_disassociate_ctrl(bitters_gpio_pin_t *pin)
{
    struct bitters_gpio_ctrl *ctrl = pin->ctrl;

    BITTERS_GPIO_ASSERT(ctrl->refcount > 0);
    ctrl->refcount--;
    pin->ctrl = NULL;
    return 0;
}

#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
static void *
bitters_gpio_irq_processing(void *args) {
    struct bitters_gpio_ctrl *ctrl = args;
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, BITTERS_SIGIRQ);

    while (1) {
	int rc = ppoll(ctrl->fds, ctrl->lines, NULL, &mask);
	/* Check if we got interrupted to perform a reload of the
	 * file descriptors table
	 */
	if (rc < 0) {
	    BITTERS_GPIO_ASSERT(errno == EINTR);
	    continue;
	}
	/* Find and process pin irq
	 */
	for (int i = 0 ; i < ctrl->lines ; i++) {
	    if (ctrl->fds[i].revents) {
		bitters_gpio_pin_t *pin = ctrl->pins[i];
		// Consume event
		BITTERS_GPIO_LOG("got interrupt on pin %d", pin->id);
		bitters_gpio_irq_wait(pin);
		// Perform callback
		BITTERS_GPIO_ASSERT(pin->irq_cb != NULL);
		pin->irq_cb(pin, pin->irq_cb_args);
	    }
	}
    };

    __builtin_unreachable();
}
#endif

static struct bitters_gpio_ctrl *
_bitters_gpio_ctrl_create(const char *devname)
{
    int                       rc      = -EINVAL;
    int                       fd      = -1;
    char                     *name    = NULL;
    char                     *devpath = NULL;
    struct bitters_gpio_ctrl *ctrl    = NULL;

    /* Create a new controller
     */
    // Allocate memory
    ctrl = calloc(1, sizeof(struct bitters_gpio_ctrl));
    if (ctrl == NULL) {
	BITTERS_GPIO_LOG("failed to allocate memory for gpio controller");
	goto failed;
    }

    // Duplicate chip name to avoid dangling pointer
    // if pin is later removed
    name = strdup(devname);
    if (name == NULL) {
	BITTERS_GPIO_LOG("failed to allocate memory for gpio name");
	goto failed;
    }

    // Build device path
    rc = asprintf(&devpath, "/dev/%s", devname);
    if (rc < 0) {
	errno = ENOMEM;
	BITTERS_GPIO_LOG("unable to build path to device name"
			 " (out of memory)");
	goto failed;
    }

    // Open device
    fd = open(devpath, O_RDONLY);
    if (fd < 0) {
	BITTERS_GPIO_LOG("failed to open %s (%s)", devpath, strerror(errno));
	goto failed;
    }
    BITTERS_GPIO_LOG("controller device %s opened (fd=%d)", devpath, fd);

#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
    // Get information about chip
    struct gpiochip_info cinfo;
    rc = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &cinfo);
    if (rc < 0) {
	BITTERS_GPIO_LOG("failed to get information about %s (%s)",
			 devname, strerror(errno));
	goto failed;
    }
    ctrl->lines = cinfo.lines;
    ctrl->fds   = calloc(cinfo.lines, sizeof(struct pollfd));
    ctrl->pins  = calloc(cinfo.lines, sizeof(bitters_gpio_pin_t *));
    if ((ctrl->fds == NULL) || (ctrl->pins == NULL)) {
	BITTERS_GPIO_LOG("failed to allocate memory for interrupt polling");
	goto failed;
    }
    for (int i = 0 ; i < cinfo.lines ; i++) {
	ctrl->fds[i].fd = -1;
    }
    BITTERS_GPIO_LOG("found %u lines for %s (%s)",
		     cinfo.lines, cinfo.name, cinfo.label);

    // Create IRQ processing thread
    rc = pthread_create(&ctrl->irq_thread, NULL,
			bitters_gpio_irq_processing, ctrl);
    if (rc < 0) {
	BITTERS_GPIO_LOG("failed to create irq processing thread");
	goto failed;
    }
#endif

    // Release memory
    free(devpath);

    // Initialise
    ctrl->fd   = fd;
    ctrl->name = name;
    return ctrl;

    // Deal with failures
 failed:
    free(devpath);
    free(name);
#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
    free(ctrl->fds);
    free(ctrl->pins);
#endif
    free(ctrl);
    return NULL;
}


static int
_bitters_gpio_pin_associate_ctrl(bitters_gpio_pin_t *pin)
{
    int                       rc      = -EINVAL;
    struct bitters_gpio_ctrl *ctrl    = NULL;

    /* Lookup for existing controller
     */
    for (ctrl =  LIST_FIRST(&bitters_gpio_ctrls) ; ctrl ; LIST_NEXT(ctrl, entries)) {
	if (! strcmp(ctrl->name, pin->ctrl_devname)) {
	    BITTERS_GPIO_LOG("found instanciated gpio controller (%s)",
			     ctrl->name);
	    goto associate;
	}
    }

    /* Create a new controller
     */
    ctrl = _bitters_gpio_ctrl_create(pin->ctrl_devname);
    if (ctrl == NULL) {
	rc = -errno;
	goto failed;
    }

    // Attach to controler list
    LIST_INSERT_HEAD(&bitters_gpio_ctrls, ctrl, entries);

    // Associate
 associate:
#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
    BITTERS_GPIO_ASSERT(pin->id < ctrl->lines);
#endif
    ctrl->refcount++;
    pin->ctrl = ctrl;
    BITTERS_GPIO_LOG("controller %s associated to pin %d", ctrl->name, pin->id);
    return 0;

    // Deal with failures
 failed:
    return rc;
}



static inline int
_bitters_gpio_pin_ensure_associated(bitters_gpio_pin_t *pin)
{
    if (pin->ctrl != NULL)
	return 0;
    return _bitters_gpio_pin_associate_ctrl(pin);
}



static void
_bitters_gpio_warn_about_hardware_config(void) {
    static int once = 0;
    if (once++                                          ||
	(getenv("BITTERS_SILENCE_WARNING"    ) != NULL) ||
	(getenv("BITTERS_SILENCE_RPI_WARNING") != NULL)) return;

    fprintf(stderr,
	"\n"
	"bitters: Don't forget to configure at boot-time Raspberry PI with\n"
	"       | necessary pull-up / pull-down / no-pull\n"
	"       |   * raspi-gpio  (see: raspi-gpio help)\n"
        "       |   * config.txt (see: config-txt/gpio.md)\n"
	"       |   * device-tree with brcm,pull (see: brcm,bcm2835-gpio.txt)\n"
	"\n");
}

#if !defined(BITTERS_SILENCE_WARNING    ) &&				\
    !defined(BITTERS_SILENCE_RPI_WARNING)
#  define BITTERS_GPIO_WARN_ABOUT_HARDWARE_CONFIG()			\
    _bitters_gpio_warn_about_hardware_config()
#else
#  define BITTERS_GPIO_WARN_ABOUT_HARDWARE_CONFIG()
#endif



/*== Exported functions ================================================*/

int
bitters_gpio_init(void)
{
#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
    /* Install dummy signal handler (to have interrupted system call) */
    struct sigaction sigact = { .sa_handler = _bitters_gpio_sigirq };
    struct sigaction oldsigact;
    int rc = sigaction(BITTERS_SIGIRQ, &sigact, &oldsigact);
    if (rc < 0) {
	BITTERS_LOG("failed to install signal hander for %s",
		    strsignal(BITTERS_SIGIRQ));
	return -errno;
    }
    BITTERS_ASSERT((oldsigact.sa_handler   == NULL) &&
		   (oldsigact.sa_sigaction == NULL));
    if ((oldsigact.sa_handler   != NULL) ||
	(oldsigact.sa_sigaction != NULL)) {
	// Restore original signal handler
	sigaction(BITTERS_SIGIRQ, &oldsigact, NULL);
	// Warn about it
	char *signame = strsignal(BITTERS_SIGIRQ);
	fprintf(stderr,
	"\n"
	"bitters: process is already intercepting %s\n"
	"       | orignal handler has been restored\n"
	"       | you can either:\n"
	"       |   * compile defining BITTERS_SIGIRQ to another signal\n"
	"       |   * change your program so that %s is free to be used\n"
	"       |   * don't use bitters IRQ callback\n"
	"\n", signame, signame);
	return -EBUSY;
    }
#endif

    BITTERS_GPIO_WARN_ABOUT_HARDWARE_CONFIG();
    return 0;
}



int
bitters_gpio_pin_enable(bitters_gpio_pin_t *pin, bitters_gpio_cfg_t *cfg)
{
    BITTERS_GPIO_ASSERT_PIN(pin);
    BITTERS_GPIO_ENSURE_ASSOCIATED_PIN(pin);

    // Already enabled ?
    if (pin->fd >= 0)
	return 0;

    // Build flags
    uint64_t flags = 0;
    switch(cfg->dir) {
    case BITTERS_GPIO_DIR_INPUT:
	flags |= GPIO_V2_LINE_FLAG_INPUT;
	switch(cfg->interrupt) {
	case BITTERS_GPIO_INTERRUPT_DISABLED:
	    break;
	case BITTERS_GPIO_INTERRUPT_RISING_EDGE:
	    flags |= GPIO_V2_LINE_FLAG_EDGE_RISING;
	    break;
	case BITTERS_GPIO_INTERRUPT_FALLING_EDGE:
	    flags |= GPIO_V2_LINE_FLAG_EDGE_FALLING;
	    break;
	case BITTERS_GPIO_INTERRUPT_BOTH_EDGE:
	    flags |= GPIO_V2_LINE_FLAG_EDGE_RISING  |
		GPIO_V2_LINE_FLAG_EDGE_FALLING ;
	    break;
	default:
	    BITTERS_GPIO_LOG("unexepected interrupt value");
	    return -EINVAL;
	}
	switch(cfg->bias) {
	case BITTERS_GPIO_BIAS_DISABLED:
	    flags |= GPIO_V2_LINE_FLAG_BIAS_DISABLED;
	    break;
	case BITTERS_GPIO_BIAS_PULL_UP:
	    flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_UP;
	    break;
	case BITTERS_GPIO_BIAS_PULL_DOWN:
	    flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN;
	    break;
	case BITTERS_GPIO_BIAS_DEFAULT:
	    BITTERS_GPIO_WARN_ABOUT_HARDWARE_CONFIG();
	    break;
	default:
	    BITTERS_GPIO_LOG("unexepected bias value");
	    return -EINVAL;
	}
	break;

    case BITTERS_GPIO_DIR_OUTPUT:
	flags |= GPIO_V2_LINE_FLAG_OUTPUT;
	switch(cfg->mode) {
	case BITTERS_GPIO_MODE_OPEN_DRAIN:
	    flags |= GPIO_V2_LINE_FLAG_OPEN_DRAIN;
	    break;
	case BITTERS_GPIO_MODE_OPEN_SOURCE:
	    flags |= GPIO_V2_LINE_FLAG_OPEN_SOURCE;
	    break;
	case BITTERS_GPIO_MODE_DEFAULT:
	    BITTERS_GPIO_WARN_ABOUT_HARDWARE_CONFIG();
	    break;
	default:
	    BITTERS_GPIO_LOG("unexepected mode value");
	    return -EINVAL;
	}
	break;

    default:
	BITTERS_GPIO_LOG("unexepected direction value");
	return -EINVAL;
    }

    if ((cfg->interrupt != BITTERS_GPIO_INTERRUPT_DISABLED) &&
	(cfg->dir       != BITTERS_GPIO_DIR_INPUT         )) {
	BITTERS_GPIO_LOG("when using interrupt direction must be input");
	return -EINVAL;
    }

    // Create gpio line request
    struct gpio_v2_line_request req = {
	.num_lines        = 1,
	.offsets          = { [0] = pin->id },
	.config.flags     = flags,
	.config.num_attrs = ((cfg->dir     == BITTERS_GPIO_DIR_INPUT) &&
			     (cfg->debounce > 0)) ? 2 : 1,
	.config.attrs     = {
	    { .mask                   = 1 << 0,
	      .attr.id                = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES,
	      .attr.values            = (cfg->defval ? 1 : 0) << 0 },
	    { .mask                   = 1 << 0,
	      .attr.id                = GPIO_V2_LINE_ATTR_ID_DEBOUNCE,
	      .attr.debounce_period_us= cfg->debounce              }
	}
    };
    strncpy(req.consumer, cfg->label, sizeof(req.consumer));

    // Call ioctl
    int rc = ioctl(pin->ctrl->fd, GPIO_V2_GET_LINE_IOCTL, &req);
    if (rc < 0) {
	BITTERS_GPIO_LOG("failed to issue GPIO_V2_GET_LINE IOCTL"
			 " for pin %d (%s)", pin->id, strerror(errno));
	return -errno;
    }

    // Store file descriptor
    //  (can be useful: req.event_buffer_size)
    pin->fd = req.fd;

    // Set interrupt handling status
    if (cfg->interrupt != BITTERS_GPIO_INTERRUPT_DISABLED) {
	pin->flags |= GPIO_PIN_FLAG_INTERRUPT;
    }

    // Job's done
    BITTERS_GPIO_LOG("pin %d (%s) enabled (fd=%d)",
		     pin->id, cfg->label, pin->fd);
    return 0;
}



int
bitters_gpio_pin_disable(bitters_gpio_pin_t *pin)
{
    BITTERS_GPIO_ASSERT_PIN(pin);

    // Already disabled (or never enabled) ?
    if (pin->fd < 0)
	return 0;

    // Disable pin, by closing file descriptor
    int rc = close(pin->fd);
    if (rc < 0) {
	rc = -errno;
	BITTERS_GPIO_LOG("failed to disable pin %s[%d] (%s)",
		 pin->ctrl_devname, pin->id, strerror(errno));
	return rc;
    }

    // Mark as disabled
    pin->flags = 0;
    pin->fd    = -1;

    // Job's done
    return 0;
}



int
bitters_gpio_pin_read(bitters_gpio_pin_t *pin, int *val)
{
    BITTERS_GPIO_ASSERT_PIN(pin);
    BITTERS_GPIO_ASSERT_PIN_ASSOCIATED(pin);

    struct gpio_v2_line_values values = { .mask = 1 << 0 };
    int rc = ioctl(pin->fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &values);
    if (rc < 0) {
	rc = -errno;
	BITTERS_GPIO_LOG("failed to issue GPIOHANDLE_GET_LINE_VALUES"
			 " on pin %d (%s)", pin->id, strerror(errno));
	return rc;
    }

    if (val != NULL)
	*val = (values.bits & (1 << 0)) ? 1 : 0;

    return 0;
}



int
bitters_gpio_pin_write(bitters_gpio_pin_t *pin, int val)
{
    BITTERS_GPIO_ASSERT_PIN(pin);
    BITTERS_GPIO_ASSERT_PIN_ASSOCIATED(pin);

    struct gpio_v2_line_values values = {
	.mask = 1                    << 0,
	.bits = ((val == 0) ? 0 : 1) << 0
    };

    int rc = ioctl(pin->fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &values);
    if (rc < 0) {
	rc = -errno;
	BITTERS_GPIO_LOG("failed to issue GPIO_V2_LINE_SET_VALUES"
			 " on pin %d (%s)", pin->id, strerror(errno));
	return rc;
    }

    return 0;
}



int
bitters_gpio_irq_wait(bitters_gpio_pin_t *pin) {
    BITTERS_GPIO_ASSERT_PIN(pin);
    BITTERS_GPIO_ENSURE_INTERRUPT_PIN(pin);

    // [GPIO_V2_LINES_MAX * 16]
    struct gpio_v2_line_event event = { 0 };
    ssize_t size = read(pin->fd, &event, sizeof(event));

    if (size < 0) {
	BITTERS_GPIO_LOG("failed to read event (%s)", strerror(errno));
	return -errno;
    } else if (size != sizeof(event)) {
	BITTERS_GPIO_LOG("got event of unexpected size");
	return -ERANGE;
    }

    return event.id;
}



int
bitters_gpio_irq_fill_poolfd(bitters_gpio_pin_t *pin, struct pollfd *pfd)
{
    BITTERS_GPIO_ASSERT_PIN(pin);
    BITTERS_GPIO_ENSURE_INTERRUPT_PIN(pin);

    pfd->fd     = pin->fd;
    pfd->events = BITTERS_GPIO_POLL_EVENTS;

    return 0;
}



int
bitters_gpio_irq_callback(bitters_gpio_pin_t *pin,
			  bitters_gpio_irq_cb_t cb, void *args)
{
    BITTERS_GPIO_ASSERT_PIN(pin);
    BITTERS_GPIO_ENSURE_INTERRUPT_PIN(pin);

#if defined(BITTERS_WITH_GPIO_IRQ) && defined(BITTERS_WITH_THREADS)
    /* Save callback information */
    pin->irq_cb      = cb;
    pin->irq_cb_args = args;

    /* Initialize polling structure (if callback is null disable) */
    struct pollfd *pfd = &pin->ctrl->fds[pin->id];
    pfd->fd     = (cb != NULL) ? pin->fd : -1;
    pfd->events = BITTERS_GPIO_POLL_EVENTS;

    /* Save pin in controller table */
    pin->ctrl->pins[pin->id] = pin;

    /* Notify irq processing thread of changes */
    int rc = pthread_kill(pin->ctrl->irq_thread, BITTERS_SIGIRQ);
    if (rc < 0) {
	return -errno;
    }

    /* Job's done */
    return 0;
#else
    /* Not supported */
    return -ENOSYS;
#endif
}



/*
 * Local Variables:
 * c-basic-offset: 4
 * End:
 */
