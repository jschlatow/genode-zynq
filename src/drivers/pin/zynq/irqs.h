/*
 * \brief  Irq handlers for Zynq pin driver
 * \author Johannes Schlatow
 * \date   2022-09-09
 */

#ifndef _IRQS_H_
#define _IRQS_H_

/* local includes */
#include <common_types.h>

namespace Pin_driver {

	using namespace Genode;

	struct Irq_handlers;
}


struct Pin_driver::Irq_handlers
{
	using Irq = Platform::Device::Irq;

	Irq_handler _irq_handler;

	Irq_handlers(Env &env, Platform::Device &device, Irq_handler::Fn &fn)
	:
		_irq_handler { env, device, Irq::Index { 0 }, fn }
	{ }
};


#endif /* _IRQS_H_ */
