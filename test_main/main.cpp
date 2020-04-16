#include "accel_designer.h"

#include "feedback_controller.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

ctrl::AccelDesigner ad;
ctrl::AccelCurve ac;
std::ofstream of("main.csv");

void test(float jm, float am, float vs, float va, float ve, float d, float xs,
          float ts) {
  ad.reset(jm, am, vs, va, ve, d, xs, ts);
  ad.printCsv(of);
  std::cout << ad << std::endl;
}

int main(void) {
  // test(4800 * M_PI, 48 * M_PI, 0, 4 * M_PI, 0, M_PI / 2, 0, 0);
  test(240000, 3600, 720, 720, 0, 90, 0, 0);
  // test(100, 10, ad.v_end(), 10, 1, 10, ad.x_end(), ad.t_end());
  // test(100, 10, ad.v_end(), 10, 1, 0.1, ad.x_end(), ad.t_end());
  // test(100, 100, ad.v_end(), 10, 1, 1, ad.x_end(), ad.t_end());
#if 0
  int n = 100;
  std::mt19937 mt{std::random_device{}()};
  std::uniform_real_distribution<float> j_urd(100000, 500000);
  std::uniform_real_distribution<float> a_urd(1000, 18000);
  std::uniform_real_distribution<float> v_urd(90, 4800);
  std::uniform_real_distribution<float> d_urd(0, 32 * 90);
  auto ts = std::chrono::steady_clock::now();
  for (int i = 0; i < n; ++i) {
    test(j_urd(mt), a_urd(mt), ad.v_end(), v_urd(mt), v_urd(mt), d_urd(mt),
         ad.x_end(), ad.t_end());
  }
  auto te = std::chrono::steady_clock::now();
  auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(te - ts);
  std::cout << "Average Time: " << dur.count() / n << " [ns]" << std::endl;
#endif

#if 0
  /* Feedforward Model and Feedback Gain */
  ctrl::FeedbackController<float>::Model model;
  ctrl::FeedbackController<float>::Gain gain;
  /* Controller */
  ctrl::FeedbackController<float> feedback_controller(model, gain);
  /* Control Period [s] */
  const float Ts = 0.001f;
  /* Control */
  feedback_controller.reset();
  while (1 /* write breaking condition */) {
    const float r = 1.0f;  //< reference of output
    const float y = 0.5f;  //< measurement output
    const float dr = 0.0f; //< reference of output
    const float dy = 0.0f; //< measurement output
    const float u = feedback_controller.update(r, y, dr, dy, Ts);
    /* apply control input u here */
  }
#endif

  return 0;
}
