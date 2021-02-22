#include <ncurses.h>
#include <errno.h>
#include "ui.h"

int widget_clear(struct widget *widget, const int x, const int y,
		 const int width, const int height)
{
        int cur_y;

        if(!widget) {
                return(-EINVAL);
        }

        for(cur_y = 0; cur_y < height; cur_y++) {
                int cur_x;

                for(cur_x = 0; cur_x < width; cur_x++) {
                        mvwaddch(widget->window,
				 widget->y + y + cur_y,
				 widget->x + x + cur_x,
				 ' ');
                }
        }

        return(0);
}
