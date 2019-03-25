/**
 * @file AccelCurve.h
 * @author KERI
 * @date 2018.04.29 created
 * @date 2019.03.25 modified
 * @url https://kerikeri.top/posts/2018-04-29-accel-designer4/
 * @brief 躍度0次，加速度1次，速度2次，位置3次
 * 関数により，滑らかな加速を実現する
 */
#pragma once

#include <cmath>    //< for std::sqrt, std::cbrt, std::pow
#include <complex>  //< for std::complex
#include <iostream> //< for std::cout

namespace signal_processing {

/**
 * @class 加速曲線を生成するクラス
 * @brief 引数に従って加速曲線を生成する
 */
class AccelCurve {
public:
  /**
   * @brief 最大躍度の定数 [mm/s/s/s]
   */
  static constexpr const float j_max = 500000;

public:
  /**
   * @brief 初期化付きのコンストラクタ．
   * @param a_max   最大加速度 [mm/s/s]
   * @param v_start 始点速度   [mm/s]
   * @param v_end   終点速度   [mm/s]
   */
  AccelCurve(const float a_max, const float v_start, const float v_end) {
    reset(a_max, v_start, v_end);
  }
  /**
   * @brief 空のコンストラクタ．あとで reset() により初期化すること．
   */
  AccelCurve() {
    jm = am = t0 = t1 = t2 = t3 = v0 = v1 = v2 = v3 = x0 = x1 = x2 = x3 = tc =
        tm = 0;
  }
  /**
   * @brief 引数の拘束条件から曲線を生成する．
   * この関数によって，すべての変数が初期化される．(漏れはない)
   * @param a_max   最大加速度の大きさ [mm/s/s]
   * @param v_start 始点速度 [mm/s]
   * @param v_end   終点速度 [mm/s]
   */
  void reset(const float a_max, const float v_start, const float v_end) {
    tc = calcTimeCurve(a_max); //< 速度が曲線である時間を取得
    am = (v_end - v_start > 0) ? a_max : -a_max; //< 最大加速度の符号を決定
    jm = (v_end - v_start > 0) ? j_max : -j_max; //< 最大躍度の符号を決定
    v0 = v_start;                                //< 代入
    v3 = v_end;                                  //< 代入
    t0 = 0;                   //< ここでは初期値をゼロとする
    x0 = 0;                   //< ここでは初期値はゼロとする
    tm = (v3 - v0) / am - tc; //< 等加速度直線運動の時間を決定
    /* 等加速度直線運動の有無で分岐 */
    if (tm > 0) {
      /* 速度: 曲線 -> 直線 -> 曲線 */
      t1 = t0 + tc;
      t2 = t1 + tm;
      t3 = t2 + tc;
    } else {
      /* 速度: 曲線 -> 曲線 */
      t1 = t0 + std::sqrt(tc / am * (v3 - v0)); //< 速度差から算出
      t2 = t1;             //< 加速度一定の時間はないので同じ
      t3 = t2 + (t1 - t0); //< 対称性
    }
    v1 = v(t1);
    v2 = v(t2); //< 式から求めることができる
    x1 = x(t1);
    x2 = x(t2); //< 式から求めることができる
    x3 = x0 + (v0 + v3) / 2 * (t3 - t0); //< 速度グラフの面積により
  }
  /**
   * @brief 時刻$t$における躍度$j$
   * @param t 時刻[s]
   * @return 躍度[mm/s/s/s]
   */
  float j(const float t) const {
    if (t <= t0)
      return 0;
    else if (t <= t1)
      return j_max;
    else if (t <= t2)
      return 0;
    else if (t <= t3)
      return -j_max;
    else
      return 0;
  }
  /**
   * @brief 時刻$t$における加速度$a$
   * @param t 時刻[s]
   * @return 加速度[mm/s/s]
   */
  float a(const float t) const {
    if (t <= t0)
      return 0;
    else if (t <= t1)
      return jm * (t - t0);
    else if (t <= t2)
      return am;
    else if (t <= t3)
      return -jm * (t - t3);
    else
      return 0;
  }
  /**
   * @brief 時刻$t$における速度$v$
   * @param t 時刻[s]
   * @return 速度[mm/s]
   */
  float v(const float t) const {
    if (t <= t0)
      return v0;
    else if (t <= t1)
      return v0 + 0.50f * jm * (t - t0) * (t - t0);
    else if (t <= t2)
      return v1 + am * (t - t1);
    else if (t <= t3)
      return v3 - 0.50f * jm * (t - t3) * (t - t3);
    else
      return v3;
  }
  /**
   * @brief 時刻$t$における位置$x$
   * @param t 時刻[s]
   * @return 位置[mm]
   */
  float x(const float t) const {
    if (t <= t0)
      return x0 + v0 * (t - t0);
    else if (t <= t1)
      return x0 + v0 * (t - t0) + jm / 6 * (t - t0) * (t - t0) * (t - t0);
    else if (t <= t2)
      return x1 + v1 * (t - t1) + am / 2 * (t - t1) * (t - t1);
    else if (t <= t3)
      return x3 + v3 * (t - t3) + jm / 6 * (t - t3) * (t - t3) * (t - t3);
    else
      return x3 + v3 * (t - t3);
  }
  /**
   * @brief 終端定数
   */
  float t_end() const { return t3; }
  float v_end() const { return v3; }
  float x_end() const { return x3; }
  /**
   * @brief 曲線加速部分の時間を決定する関数
   * @param am 最大加速度の大きさ
   */
  static float calcTimeCurve(const float am) {
    const float tc = std::abs(am) / j_max; //< 時間を算出
    return tc;
  }
  /**
   * @brief 走行距離から達しうる終点速度を算出する関数
   * @param a_max 最大加速度の大きさ [mm/s/s]
   * @param vs 始点速度 [mm/s]
   * @param vt 目標速度 [mm/s]
   * @param d 走行距離 [mm]
   * @return ve 終点速度 [mm/s]
   */
  static float calcVelocityEnd(float a_max, const float vs, const float vt,
                               const float d) {
    /* 速度が曲線となる部分の時間を決定 */
    const float tc = AccelCurve::calcTimeCurve(a_max);
    /* 最大加速度の符号を決定 */
    const float am = (vt - vs > 0) ? a_max : -a_max;
    /* 等加速度直線運動の有無で分岐 */
    if (d > (2 * vs + am / tc * tc * tc) * tc) {
      /* 曲線・直線・曲線 */
      /* 2次方程式の解の公式を解く */
      const float amtc = am * tc;
      const float D = amtc * amtc - 4 * (amtc * vs - vs * vs - 2 * am * d);
      return (-amtc + std::sqrt(D)) / 2;
    }
    /* 曲線・曲線 */
    /* 3次方程式を解いて，終点速度を算出 */
    float ve; //< 変数を用意
    const float a = vs;
    const float b = am * d * d / tc;
    const float aaa = a * a * a;
    const float c0 = 27 * (32 * aaa * b + 27 * b * b);
    const float c1 = 16 * aaa + 27 * b;
    if (c0 >= 0) {
      /* ルートの中が非負のとき，つまり，b >= 0 のとき */
      const float c2 = std::cbrt((std::sqrt(c0) + c1) / 2);
      ve = (c2 + 4 * a * a / c2 - a) / 3; //< 3次方程式の解
    } else {
      /* ルートの中が負のとき，つまり，b < 0 のとき */
      const auto c2 =
          std::pow(std::complex<float>(c1 / 2, std::sqrt(-c0) / 2), 1.0f / 3);
      ve = (c2.real() * 2 - a) / 3; //< 3次方程式の解
    }
    return ve;
  }
  /** @function calcVelocityMax
   *   @brief 走行距離から最大速度を算出する関数
   *   @param am 最大加速度の大きさ [mm/s/s]
   *   @param vs 始点速度 [mm/s]
   *   @param va 飽和速度 [mm/s]
   *   @param ve 終点速度 [mm/s]
   *   @param d 走行距離 [mm]
   *   @return vm 最大速度 [mm/s]
   */
  static float calcVelocityMax(const float am, const float vs, const float ve,
                               const float d) {
    /* 速度が曲線となる部分の時間を決定 */
    const float tc = AccelCurve::calcTimeCurve(am);
    /* 2次方程式の解の公式を解く */
    const float amtc = am * tc;
    const float D = amtc * amtc - 2 * (vs + ve) * amtc + 4 * am * d +
                    2 * (vs * vs + ve * ve);
    if (D < 0) {
      /* なんかおかしい */
      std::cerr << "Error: AccelCurve::calcVelocityMax()" << std::endl;
      return vs;
    }
    return (-amtc + std::sqrt(D)) / 2; //< 2次方程式の解
  }
  /**
   * @brief 速度差から変位を算出する関数
   *
   * @param a_max   最大加速度の大きさ [mm/s/s]
   * @param v_start 始点速度 [mm/s]
   * @param v_end   終点速度 [mm/s]
   * @return float d 変位 [mm/s]
   */
  static float calcMinDistance(const float a_max, const float v_start,
                               const float v_end) {
    AccelCurve ac(a_max, v_start, v_end);
    return ac.x_end();
  }

private:
  float jm;             //< 最大躍度 [m/s/s]
  float am;             //< 最大加速度 [m/s/s]
  float t0, t1, t2, t3; //< 境界点の時刻 [s]
  float v0, v1, v2, v3; //< 境界点の速度 [m/s]
  float x0, x1, x2, x3; //< 境界点の位置 [m]
  float tc;             //< 曲線加速の時間 [s]
  float tm;             //< 最大加速度の時間 [s]
};
} // namespace signal_processing
