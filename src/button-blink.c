#include "pico/stdlib.h"

int main() {
	const uint LED_PIN = 0;
	const uint BUTTON1 = 20;
	const uint BUTTON2 = 21;
	const uint BUTTON3 = 22;

	for (int i=0; i<28; i++) {
		gpio_init(i);

		if (i == 20 || i == 21 || i == 22) {
			gpio_set_dir(i, GPIO_IN);
		} else {
			gpio_set_dir(i, GPIO_OUT);
		}
	}

	int note_num = 0;
	float note_freqs[] = {
		// 440, 493.88, (A4, B4)
		523.25, 587.33, 659.25, 698.46, 783.99, 880,
		987.77, 1046.50
	};

	while (true) {
		if (gpio_get(BUTTON1) == 0) {
			gpio_put(18, 1);
			sleep_us((float)1000000 / note_freqs[note_num]);
			gpio_put(18, 0);
			sleep_us((float)1000000 / note_freqs[note_num]);

			/* 
			for (int i=0; i<19; i++) {
				gpio_put(i, 1);

				if (i > 0) {
					gpio_put(i - 3, 0);
				}
				sleep_ms(100);
			}

			gpio_put(16, 0);
			sleep_ms(100);
			gpio_put(17, 0);
			sleep_ms(100);
			gpio_put(18, 0);
			*/
		} if (gpio_get(BUTTON2) == 0) {
			note_num = (note_num + 1) % 8;

			for (int i=0; i<(100000 / ((float)1000000 / note_freqs[note_num])); i++) {
				gpio_put(18, 1);
				sleep_us((float)1000000 / note_freqs[note_num]);
				gpio_put(18, 0);
				sleep_us((float)1000000 / note_freqs[note_num]);
			}
		} if (gpio_get(BUTTON3) == 0) {
			if (note_num == 0) {
				note_num = 7;
			} else {
				note_num = (note_num - 1) % 8;
			}

			for (int i=0; i<(100000 / ((float)1000000 / note_freqs[note_num])); i++) {
				gpio_put(18, 1);
				sleep_us((float)1000000 / note_freqs[note_num]);
				gpio_put(18, 0);
				sleep_us((float)1000000 / note_freqs[note_num]);
			}
		}
	}
}
