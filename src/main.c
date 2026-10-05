/*
 * LD1 (grün) blinkt. Jeder Druck auf den User-Button (B1)
 * wechselt zwischen langsamem und schnellem Blinken.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int main(void)
{
	bool fast = false;
	bool was_pressed = false;
	int elapsed_ms = 0;

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button, GPIO_INPUT);

	while (1) {
		/* Button alle 10 ms abfragen, nur beim Drücken (Flanke) umschalten */
		bool pressed = gpio_pin_get_dt(&button) > 0;

		if (pressed && !was_pressed) {
			fast = !fast;
		}
		was_pressed = pressed;

		elapsed_ms += 10;
		if (elapsed_ms >= (fast ? 100 : 500)) {
			gpio_pin_toggle_dt(&led);
			elapsed_ms = 0;
		}

		k_msleep(10);
	}

	return 0;
}
