/*
 * \brief  Main component for ad9361 contrib code
 * \author Johannes Schlatow
 * \date   2021-10-15
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <libc/component.h>

/* libc includes */
#include <stdlib.h> /* 'exit'   */

#include "platform.h"

/* provided by the application */
extern "C" int main(void);

void Libc::Component::construct(Libc::Env &env)
{
	Libc::with_libc([&] () {
		Ad::platform(&env);
		exit(main());
	});
}
