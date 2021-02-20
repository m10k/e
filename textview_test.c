#include <assert.h>
#include "ui.h"

int main(void)
{
	struct window *window;
	struct textview *textview;
	int event;

	assert(window_new(&window) == 0);
	assert(textview_new(&textview) == 0);

	assert(window_set_child(window, (struct widget*)textview) == 0);

	widget_redraw((struct widget*)window);

	while((event = getch())) {
		if(event == 17) {
			break;
		}

		widget_input((struct widget*)window, event);
	}

	widget_free((struct widget*)window);

	return(0);
}
