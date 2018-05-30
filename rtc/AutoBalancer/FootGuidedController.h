/* -*- coding:utf-8-unix mode:c++ -*- */
#ifndef FOOT_H_
#define FOOT_H_
#include <iostream>
#include <cmath>
#include <hrpUtil/Eigen3d.h>
#include "hrpsys/util/Hrpsys.h"

static const double DEFAULT_GRAVITATIONAL_ACCELERATION = 9.80665; // [m/s^2]

template <typename T> int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

class foot_guided_control_base
{
private:
  void calc_u(const std::size_t N, const double ref_dcm, const double ref_zmp);
  void truncate_u();
  void calc_x_k();
protected:
  Eigen::Matrix<double, 2, 2> A; // state matrix
  Eigen::Matrix<double, 2, 1> b; // input matrix
  Eigen::Matrix<double, 2, 2> Phi; // convert matrix : pos, vel => dcm, ccm
  Eigen::Matrix<double, 2, 2> Phi_inv; // convert matrix : dcm, ccm => pos, vel
  Eigen::Matrix<double, 2, 1> x_k; // pos, vel
  Eigen::Matrix<double, 2, 1> w_k; // dcm, ccm
  double u_k; // zmp
  double w_k_offset; // dcm offset
  double mu; // friction coefficient
  double dt, g;
  double dz, xi, h, h_;
  double act_vel_ratio; // how much act_vel is used (0.0 ~ 1.0)
public:
  // constructor
  foot_guided_control_base() {}
  foot_guided_control_base(const double _dt,  const double _dz,
                           const double _g = DEFAULT_GRAVITATIONAL_ACCELERATION)
    : dt(_dt), dz(_dz), g(_g), act_vel_ratio(1.0),
      x_k(Eigen::Matrix<double, 2, 1>::Zero()), u_k(0.0), w_k_offset(0.0), mu(0.5)
  {
    set_mat();
  }
  foot_guided_control_base(const double _dt,  const double _dz, const double init_xk,
                           const double _g = DEFAULT_GRAVITATIONAL_ACCELERATION)
    : dt(_dt), dz(_dz), g(_g), act_vel_ratio(1.0),
      x_k(Eigen::Matrix<double, 2, 1>::Zero()), u_k(0.0), w_k_offset(0.0), mu(0.5)
  {
    set_mat();
    x_k(0) = init_xk;
  }
  // destructor
  ~foot_guided_control_base() {};
  // update function
  void update_control(double& zmp, const std::size_t N, const double ref_dcm, const double ref_zmp);
  void update_state(double& pos);
  void update(double& zmp, double& pos, const std::size_t N, const double ref_dcm, const double ref_zmp);
  // set function
  void set_mat();
  void set_act_x_k(const double pos, const double vel, const bool is_start_or_end_phase)
  {
    x_k(0) = pos;
    if (is_start_or_end_phase) {
      x_k(1) = 0.01 * vel + 0.99 * x_k(1); // too much oscillate in start or end phase
    } else {
      x_k(1) = act_vel_ratio * vel + (1.0 - act_vel_ratio) * x_k(1);
    }
  }
  void set_pos (const double x) { x_k(0) = x; }
  void set_zmp (const double u) { u_k = u; }
  void set_offset (const double offset) { w_k_offset = offset; }
  void set_dz(const double _dz) {
    dz = _dz;
    set_mat();
  };
  void set_act_vel_ratio (const double ratio) { act_vel_ratio = ratio; }
  // get_function
  void get_pos (double& ret) { ret = x_k(0); }
  void get_vel (double& ret) { ret = x_k(1); }
  void get_acc (double& ret) { ret = xi * xi * ( x_k(0) - u_k ); }
  void get_zmp (double& ret) { ret = u_k; }
  double get_xi() { return xi; }
  double get_dz() { return dz; }
};

template <std::size_t dim>
class foot_guided_controller
{
private:
  foot_guided_control_base *controllers;
protected:
public:
  // constructor
  foot_guided_controller(const double _dt,  const double _dz, const hrp::Vector3& init_xk,
                         const double _g = DEFAULT_GRAVITATIONAL_ACCELERATION)
  {
    controllers = new foot_guided_control_base[dim];
    for (size_t i = 0; i < dim; i++) controllers[i] = foot_guided_control_base(_dt, _dz, init_xk[i], _g);
  }
  // destructor
  ~foot_guided_controller()
  {
    delete[] controllers;
  };
  // update function
  void update_control(hrp::Vector3& p_ret, const std::size_t N, const hrp::Vector3& ref_dcm, const hrp::Vector3& ref_zmp)
  {
    for (size_t i = 0; i < dim; i++)
      controllers[i].update_control(p_ret[i], N, ref_dcm[i], ref_zmp[i]);
  }
  void update_state(hrp::Vector3& x_ret)
  {
    for (size_t i = 0; i < dim; i++)
      controllers[i].update_state(x_ret[i]);
  }
  void update(hrp::Vector3& p_ret, hrp::Vector3& x_ret, const std::size_t N, const hrp::Vector3& ref_dcm, const hrp::Vector3& ref_zmp)
  {
    for (size_t i = 0; i < dim; i++)
      controllers[i].update(p_ret[i], x_ret[i], N, ref_dcm[i], ref_zmp[i]);
  }
  // set function
  void set_offset(const hrp::Vector3& offset)
  {
    for (size_t i = 0; i < dim; i++)
      controllers[i].set_offset(offset[i]);
  }
  void set_act_x_k (const hrp::Vector3& pos, const hrp::Vector3& vel, const bool is_start_or_end_phase)
  {
    for (size_t i = 0; i < dim; i++)
      controllers[i].set_act_x_k(pos(i), vel(i), is_start_or_end_phase);
  }
  void set_act_vel_ratio (const double ratio) {
    for (size_t i = 0; i < dim; i++) {
      controllers[i].set_act_vel_ratio(ratio);
    }
  }
  // get function
  double get_dz() { return controllers[0].get_dz(); }
  void get_acc(hrp::Vector3& ret) {
    for (size_t i = 0; i < dim; i++)
      controllers[i].get_acc(ret[i]);
  }
  void set_dz(const double _dz) {
    for (size_t i = 0; i < dim; i++) {
      controllers[i].set_dz(_dz);
    }
  }
};
#endif /*FOOT_H_*/
