/**
 * @file AccelDesigner.h
 * @author KERI
 * @date 2018.04.29 created
 * @date 2019.03.25 modified
 * @url https://kerikeri.top/posts/2018-04-29-accel-designer4/
 * @brief 距離の拘束を満たす走行軌道を生成する
 */
#pragma once

#include "AccelCurve.h"

#include <algorithm> //< for std::max, std::min
#include <cmath>     //< for std::sqrt, std::cbrt, std::pow
#include <iostream>  //< for std::cout
#include <ostream>   //< for std::ostream

namespace signal_processing {

/**
 * @class 加減速曲線を生成するクラス
 * @brief 引数の拘束条件に従って速度計画をし，加減速曲線を生成する
 */
class AccelDesigner {
public:
  /**
   * @brief 初期化付きコンストラクタ
   *
   * @param a_max     最大加速度の大きさ [mm/s/s], 正であること
   * @param v_start   始点速度 [mm/s]
   * @param v_sat     飽和速度 [mm/s]
   * @param v_target  目標速度 [mm/s]
   * @param v_end     終点速度 [mm/s]
   * @param distance  移動距離の大きさ [mm], 非負であること
   * @param x_start   始点位置 [mm]
   * @param t_start   始点時刻 [t]
   */
  AccelDesigner(const float a_max, const float v_start, const float v_sat,
                const float v_target, const float distance,
                const float x_start = 0, const float t_start = 0) {
    reset(a_max, v_start, v_sat, v_target, distance, x_start, t_start);
  }
  /**
   * @brief 空のコンストラクタ．あとで reset() により初期化すること．
   */
  AccelDesigner() { t0 = t1 = t2 = t3 = x0 = x3 = 0; }
  /**
   * @brief 引数の拘束条件から曲線を生成する．
   * この関数によって，すべての変数が初期化される．(漏れはない)
   *
   * @param a_max     最大加速度の大きさ [mm/s/s], 正であること
   * @param v_start   始点速度 [mm/s]
   * @param v_sat     飽和速度 [mm/s]
   * @param v_target  目標速度 [mm/s]
   * @param v_end     終点速度 [mm/s]
   * @param distance  移動距離の大きさ [mm], 非負であること
   * @param x_start   始点位置 [mm]
   * @param t_start   始点時刻 [t]
   */
  void reset(const float a_max, const float v_start, const float v_sat,
             const float v_target, float distance, const float x_start = 0,
             const float t_start = 0) {
    /* 最大速度の仮置き */
    float v_max = std::max({v_start, v_sat, v_target});
    float v_end = v_target;
    /* 走行距離が負の場合の例外処理 */
    if (distance <= 0) {
      std::cerr << "Warning: distance < 0" << std::endl;
      v_end = v_max = v_start;
      distance = 0;
    }
    /* 走行距離から終点速度$v_e$を算出 */
    if (distance < AccelCurve::calcMinDistance(a_max, v_start, v_end)) {
      /* 走行距離から終点速度$v_e$を算出 */
      v_end = AccelCurve::calcVelocityEnd(a_max, v_start, v_target, distance);
      v_max = std::max(v_start, v_end);
    }
    /* 曲線を生成 */
    ac.reset(a_max, v_start, v_max); //< 加速
    dc.reset(a_max, v_max, v_end);   //< 減速
    /* 走行距離の拘束を満たさない場合の処理 */
    if (distance < ac.x_end() + dc.x_end()) {
      /* 走行距離から最大速度$v_m$を算出 */
      v_max = AccelCurve::calcVelocityMax(a_max, v_start, v_end, distance);
      v_max = std::min(v_max, v_sat);            //< 飽和速度で飽和
      v_max = std::max({v_max, v_start, v_end}); //< 無駄な減速を避ける
      ac.reset(a_max, v_start, v_max);           //< 加速
      dc.reset(a_max, v_max, v_end);             //< 減速
    }
    if (ac.x_end() + dc.x_end() > distance + 0.1f) {
      std::cerr << "Error: distance constraint: " << distance
                << " result: " << ac.x_end() + dc.x_end() << std::endl;
    }
    /* 各定数の算出 */
    x0 = x_start;
    x3 = x_start + distance;
    t0 = t_start;
    t1 = t0 + ac.t_end(); //< 曲線加速終了の時刻
    t2 = t0 + ac.t_end() +
         (distance - ac.x_end() - dc.x_end()) / v_max; //< 等速走行終了の時刻
    t3 = t0 + ac.t_end() + (distance - ac.x_end() - dc.x_end()) / v_max +
         dc.t_end();        //< 曲線減速終了の時刻
    const float e = 0.001f; //< 数値誤差分
    if (!(t0 <= t1 + e && t1 <= t2 + e && t2 <= t3 + e)) {
      std::cerr << "Error: Time Point" << std::endl;
      /* 入力情報の表示 */
      std::cout << "a_max: " << a_max << "\tv_start: " << v_start
                << "\tv_sat: " << v_sat << "\tv_target: " << v_target
                << "\tdistance: " << distance << std::endl;
      /* 表示 */
      std::cout << "v_start: " << v_start << "\tv_max: " << v_max
                << "\tv_end: " << v_end << std::endl;
      std::cout << "t0: " << t0 << "\tt1: " << t1 << "\tt2: " << t2
                << "\tt3: " << t3 << std::endl;
      std::cout << "x0: " << x0 << "\tx1: " << x0 + ac.x_end()
                << "\tx2: " << x0 + (distance - dc.x_end()) << "\tx3: " << x3
                << std::endl;
    }
  }
  /**
   * @brief 時刻$t$における躍度$j$
   * @param t 時刻[s]
   * @return j 躍度[mm/s/s/s]
   */
  float j(const float t) const {
    if (t < t2)
      return ac.j(t - t0);
    else
      return dc.j(t - t2);
  }
  /**
   * @brief 時刻$t$における加速度$a$
   * @param t 時刻 [s]
   * @return a 加速度 [mm/s/s]
   */
  float a(const float t) const {
    if (t < t2)
      return ac.a(t - t0);
    else
      return dc.a(t - t2);
  }
  /**
   * @brief 時刻$t$における速度$v$
   * @param t 時刻 [s]
   * @return v 速度 [mm/s]
   */
  float v(const float t) const {
    if (t < t2)
      return ac.v(t - t0);
    else
      return dc.v(t - t2);
  }
  /**
   * @brief 時刻$t$における位置$x$
   * @param t 時刻 [s]
   * @return x 位置 [mm]
   */
  float x(const float t) const {
    if (t < t2)
      return x0 + ac.x(t - t0);
    else
      return x3 - dc.x_end() + dc.x(t - t2);
  }
  /**
   * @brief 終端xx
   */
  float t_end() const { return t3; }
  float v_end() const { return dc.v_end(); }
  float x_end() const { return x3; }
  /**
   * @brief stdoutに軌道のcsvを出力する関数．
   */
  void printCsv(const float t_interval = 0.001f) const {
    printCsv(std::cout, t_interval);
  }
  /**
   * @brief std::ofstream に軌道のcsvを出力する関数．
   */
  void printCsv(std::ostream &os, const float t_interval = 0.001f) const {
    for (float t = t0; t < t_end(); t += t_interval) {
      os << t << "," << j(t) << "," << a(t) << "," << v(t) << "," << x(t)
         << std::endl;
    }
  }
  /**
   * @brief 情報の表示
   */
  friend std::ostream &operator<<(std::ostream &os, const AccelDesigner &obj) {
    os << "AccelDesigner ";
    os << "\td: " << obj.x3 - obj.x0;
    os << "\tvs: " << obj.ac.v(0);
    os << "\tvm: " << obj.ac.v_end();
    os << "\tve: " << obj.dc.v_end();
    os << "\tt0: " << obj.t0;
    os << "\tt1: " << obj.t1;
    os << "\tt2: " << obj.t2;
    os << "\tt3: " << obj.t3;
    return os;
  }

private:
  float t0, t1, t2, t3; /**< 境界点の時刻 [s] */
  float x0, x3;         /**< 境界点の位置 [mm] */
  AccelCurve ac, dc;    /**< 曲線加速，曲線減速オブジェクト */
};

} // namespace signal_processing
