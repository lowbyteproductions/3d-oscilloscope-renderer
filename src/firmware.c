#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include "core/system.h"

#define PWM_PORT    (GPIOA)
#define PWM_CH1_PIN (GPIO2)
#define PWM_CH2_PIN (GPIO3)
#define PWM_CH1_OC  (TIM_OC3)
#define PWM_CH2_OC  (TIM_OC4)


#define NUM_POINTS (126)
#define XS_LENGTH   (126)
uint8_t xs[XS_LENGTH] = {
    73, 88, 104, 120, 135, 151, 167, 182, 187, 193, 198, 203, 208, 213, 219, 219,
    219, 219, 219, 219, 219, 219, 203, 187, 172, 156, 141, 125, 109, 104, 99, 94,
    88, 83, 78, 73, 73, 73, 73, 73, 73, 73, 73, 78, 83, 88, 94, 99,
    104, 109, 125, 141, 156, 172, 187, 203, 219, 213, 208, 203, 198, 193, 187, 182,
    182, 182, 182, 182, 182, 182, 182, 187, 193, 198, 203, 208, 213, 219, 203, 187,
    172, 156, 141, 125, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
    109, 109, 109, 104, 99, 94, 88, 83, 78, 73, 88, 104, 120, 135, 151, 167,
    182, 182, 182, 182, 182, 182, 182, 182, 167, 151, 135, 120, 104, 88,
};
#define YS_LENGTH   (126)
uint8_t ys[YS_LENGTH] = {
    255, 255, 255, 255, 255, 255, 255, 255, 250, 245, 239, 234, 229, 224, 219, 203,
    187, 172, 156, 141, 125, 109, 109, 109, 109, 109, 109, 109, 109, 114, 120, 125,
    130, 135, 141, 146, 161, 177, 193, 208, 224, 239, 255, 250, 245, 239, 234, 229,
    224, 219, 219, 219, 219, 219, 219, 219, 219, 224, 229, 234, 239, 245, 250, 255,
    239, 224, 208, 193, 177, 161, 146, 141, 135, 130, 125, 120, 114, 109, 109, 109,
    109, 109, 109, 109, 109, 125, 141, 156, 172, 187, 203, 219, 203, 187, 172, 156,
    141, 125, 109, 114, 120, 125, 130, 135, 141, 146, 146, 146, 146, 146, 146, 146,
    146, 161, 177, 193, 208, 224, 239, 255, 255, 255, 255, 255, 255, 255,
};

uint8_t point_index = 0;

void tim3_isr(void) {
  timer_set_oc_value(TIM2, PWM_CH1_OC, xs[point_index]);
  timer_set_oc_value(TIM2, PWM_CH2_OC, ys[point_index]);

  point_index = (point_index + 1) % NUM_POINTS;

  timer_clear_flag(TIM3, TIM_SR_CC1IF);
}

static void timer_setup(void) {
  rcc_periph_clock_enable(RCC_TIM2);
  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_oc_mode(TIM2, PWM_CH1_OC, TIM_OCM_PWM1);
  timer_set_oc_mode(TIM2, PWM_CH2_OC, TIM_OCM_PWM1);

  // 25.6kHz ~= cf / (prescaler * period)
  timer_set_prescaler(TIM2, 13-1);
  timer_set_period(TIM2, 256-1);

  timer_enable_counter(TIM2);
  timer_enable_oc_output(TIM2, PWM_CH1_OC);
  timer_enable_oc_output(TIM2, PWM_CH2_OC);

  // Set the interrupt
  rcc_periph_clock_enable(RCC_TIM3);
  timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_oc_mode(TIM3, PWM_CH1_OC, TIM_OCM_ACTIVE);

  // 750Hz
  timer_set_prescaler(TIM3, 1000-1);
  timer_set_period(TIM3, 112-1);

  timer_enable_counter(TIM3);
  timer_enable_irq(TIM3, TIM_DIER_CC1IE);
  nvic_enable_irq(NVIC_TIM3_IRQ);
}

static void gpio_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_mode_setup(PWM_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH1_PIN | PWM_CH2_PIN);
  gpio_set_af(PWM_PORT, GPIO_AF1, PWM_CH1_PIN | PWM_CH2_PIN);
}

int main(void) {
  system_setup();
  gpio_setup();
  timer_setup();

  while(1){}

  return 0;
}
