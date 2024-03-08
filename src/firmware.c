#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <string.h>
#include <math.h>

#include "core/system.h"

#define PWM_PORT    (GPIOA)
#define PWM_CH1_PIN (GPIO2)
#define PWM_CH2_PIN (GPIO3)
#define PWM_CH1_OC  (TIM_OC3)
#define PWM_CH2_OC  (TIM_OC4)

// PA2 + PA3 = TIM2 PWM (channels 3+4)
// TIM3 = OC change timer

#define MOD_INC(x, max) (x = (x + 1) % (max))

typedef struct Vec3 {
  float x;
  float y;
  float z;
} Vec3;

#define PI                (3.141592f)
#define NUM_LERP_POINTS   (8)
#define CUBE_POINTS       (8)
#define NUM_EDGES         (18)

static Vec3 cube[CUBE_POINTS] = {
  { .x = -0.5f, .y = -0.5f, .z = -0.5f},
  { .x = -0.5f, .y = -0.5f, .z =  0.5f},
  { .x = -0.5f, .y =  0.5f, .z = -0.5f},
  { .x = -0.5f, .y =  0.5f, .z =  0.5f},
  { .x =  0.5f, .y = -0.5f, .z = -0.5f},
  { .x =  0.5f, .y = -0.5f, .z =  0.5f},
  { .x =  0.5f, .y =  0.5f, .z = -0.5f},
  { .x =  0.5f, .y =  0.5f, .z =  0.5f}
};

static const float lerp_points[NUM_LERP_POINTS] = {0.0, 0.14285714285714285, 0.2857142857142857, 0.42857142857142855, 0.5714285714285714, 0.7142857142857143, 0.8571428571428571, 1.0};
static const uint8_t edges[NUM_EDGES] = {0, 1, 3, 2, 0, 4, 5, 7, 6, 4, 0, 1, 5, 7, 3, 2, 6, 4};
static uint8_t edge_index = 0;
static uint8_t lerp_index = 0;
static Vec3 v1;
static Vec3 v2;

static void rotate_cube(float ax, float ay) {
  float x, y, z;

  float sin_x = sinf(ax);
  float cos_x = cosf(ax);
  float sin_y = sinf(ay);
  float cos_y = cosf(ay);

  for (uint8_t i = 0; i < CUBE_POINTS; i++) {
    x = cube[i].x;
    y = cube[i].y;
    z = cube[i].z;

    cube[i].x = x * cos_x - z * sin_x;
    cube[i].z = z * cos_x + x * sin_x;

    z = cube[i].z;
    cube[i].y = y * cos_y - z * sin_y;
    cube[i].z = z * cos_y + y * sin_y;
  }
}

static Vec3 translate_and_scale(Vec3 v) {
  const float s = 128.0f - 10.0f;
  const float t = 1.0f;
  Vec3 nv = { .x = (v.x + t) * s, .y = (v.y + t) * s, .z = (v.z + t) * s };
  return nv;
}

static float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

void tim3_isr(void) {
  float x = lerp(v1.x, v2.x, lerp_points[lerp_index]);
  float y = lerp(v1.y, v2.y, lerp_points[lerp_index]);
  MOD_INC(lerp_index, NUM_LERP_POINTS);

  timer_set_oc_value(TIM2, PWM_CH1_OC, (uint32_t)x);
  timer_set_oc_value(TIM2, PWM_CH2_OC, (uint32_t)y);

  if (lerp_index == 0) {
    MOD_INC(edge_index, NUM_EDGES);
    if (edge_index == 0) {
      // We've drawn all the edges, rotate the cube and start anew
      rotate_cube(PI/90.0f, PI/360.0f);
      v1 = translate_and_scale(cube[edges[0]]);
      v2 = translate_and_scale(cube[edges[1]]);
    } else {
      v1 = v2;
      v2 = translate_and_scale(cube[edges[edge_index]]);
    }
  }

  timer_clear_flag(TIM3, TIM_SR_CC1IF);
}

static void timer_setup(void) {
  // Setup the two PWM channels
  rcc_periph_clock_enable(RCC_TIM2);

  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  timer_set_oc_mode(TIM2, PWM_CH1_OC, TIM_OCM_PWM1);
  timer_set_oc_mode(TIM2, PWM_CH2_OC, TIM_OCM_PWM1);

  // Freq = (25600Hz ~ 25.6kHz)
  timer_set_prescaler(TIM2, 13-1);
  timer_set_period(TIM2, 256-1);

  timer_enable_counter(TIM2);
  timer_enable_oc_output(TIM2, TIM_OC3);
  timer_enable_oc_output(TIM2, TIM_OC4);

  // Setup level timer
  rcc_periph_clock_enable(RCC_TIM3);
  nvic_enable_irq(NVIC_TIM3_IRQ);
  timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  // Freq = (500Hz ~ 0.5kHz)
  timer_set_prescaler(TIM3, 112-1);
  timer_set_period(TIM3, 1000-1);

  timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_ACTIVE);
  timer_enable_irq(TIM3, TIM_DIER_CC1IE);
}

static void timer_start(void) {
  timer_enable_counter(TIM3);
  timer_enable_oc_output(TIM3, TIM_OC1);
}

static void gpio_setup(void) {
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_mode_setup(PWM_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, PWM_CH1_PIN | PWM_CH2_PIN);
  gpio_set_output_options(PWM_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, PWM_CH1_PIN | PWM_CH2_PIN);
  gpio_set_af(PWM_PORT, GPIO_AF1, PWM_CH1_PIN | PWM_CH2_PIN);
}

int main(void) {
  system_setup();
  gpio_setup();
  timer_setup();

  // Get the cube into a nice initial rotation where we can see all the points
  rotate_cube(PI / 16.0f, atanf(sqrtf(2.0f)));

  // Compute the first points to lerp between
  v1 = translate_and_scale(cube[edges[0]]);
  v2 = translate_and_scale(cube[edges[1]]);

  timer_start();

  while(1){}

  return 0;
}
