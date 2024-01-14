#include <errno.h>
#include "ui.h"

int _container_add(struct container *container, struct widget *widget)
{
	return(-ENOSYS);
}

int container_init(struct container *container)
{
	int err;

	if(!container) {
		return(-EINVAL);
	}

	err = widget_init((struct widget*)container);

	if(!err) {
		container->add = _container_add;
	}

	return(err);
}
