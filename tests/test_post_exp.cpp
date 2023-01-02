// Tests the unbiased estimator of the posterior expectation of the integral of
// grad theta of the Hamiltonian.
//
// Authors:
//  Ilias Bilionis
//
// Date:
//  12/28/2022

#include <cmath>
#include <random>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "pift.hpp"
#include "example02.hpp"

#include "options.hpp"
#include "postprocessing.hpp"

// Definition of some types
using RNG = std::mt19937;
using F = float;
using Domain = pift::UniformRectangularDomain<F, RNG>;
// Type for parameterized field
using FField = pift::Fourier1DField<F, Domain>;
// Type for constrained parameterized field
using CFField = pift::Constrained1DField<F, FField, Domain>;
// Type for Hamiltonian
using H = Example02Hamiltonian<F>;
// Type for likelihood
using L = pift::GaussianLikelihood<F, CFField>;
// Type for the unbiased estimator of the integral of the gradient of the
// Hamiltonian with respect to w
using UEGradWH = pift::UEIntegralGradWH<F, H, CFField, Domain>;
// Type for unbiased estimator for minus the grad w of the log likelihood
using UEGradWL = pift::UEGradWL<F, L, RNG>;
// Type for unbiased estimator for minus the grad w of the log posterior
using UEGradWP = pift::UEGradWPost<F, UEGradWH, UEGradWL>;
// Type for the unbiased estimator of the integral of the gradient of the
// Hamiltonian with respect to theta
using UEGradThetaH = pift::UEIntegralGradThetaH<F, H, CFField, Domain>;
// Type for unbiased estimator for the posterior expectation of grad theta of the
// Hamiltonian
using UEGradThetaPost = pift::UEGradThetaHF<F, UEGradWP, UEGradThetaH, RNG>;

int main(int argc, char* argv[]) {
  const F gamma = 1.0;
  char parsed_gamma[256];
  snprintf(parsed_gamma, 256, "gamma=%.2e", gamma);

  // Open the configuration file to read the rest of the parameters
  std::string config_file = "test_config.yml";
  //std::filesystem::path config_file_full_path(config_file);
  if (not std::filesystem::exists(config_file)) {
    std::cerr << "Configuration file `" << config_file << "` was not found."
              << std::endl;
    exit(2);
  }
    
  YAML::Node yaml = YAML::LoadFile(config_file); 
  Configuration02<F> config(yaml);

  // The output prefix
  std::string prefix = config.output.prefix + "_" + std::string(parsed_gamma);

  // A random number generator
  RNG rng;

  Domain domain(config.domain.bounds, rng);

  // Make the spatial domain
  FField psi(domain, config.field.num_terms);

  // Constrain the field to satisfy the boundary conditions
  CFField phi(psi, domain, config.field.boundary_values);

  // The Hamiltonian
  H h(gamma);

  // Initialize the parameters
  std::normal_distribution<F> norm(0, 1);
  F theta[h.get_num_params()];
  for(int i=0; i<h.get_num_params(); i++)
    theta[i] = config.parameters.init_mean[0] +
               config.parameters.init_std[0] * norm(rng);

  // The unbiased estimator of the integral of the gradient of the
  // Hamiltonian with respect to theta
  UEGradThetaH ue_int_grad_theta_H(h, phi, domain,
      config.parameters.prior.num_collocation);

  // Unbiased estimator used to take expectations over posterior
  UEGradWH ue_grad_w_h(
      h,
      phi,
      domain,
      config.parameters.prior.num_collocation,
      theta
  );
  const F sigma = 0.01;
  auto x_obs = 
    pift::loadtxtvec<F>("../examples/example02_n=100_sigma=1.00e-04_0_x_obs.csv");
  auto y_obs = 
    pift::loadtxtvec<F>("../examples/example02_n=100_sigma=1.00e-04_0_y_obs.csv");
  assert(x_obs.size() == y_obs.size());
  L l(phi, x_obs.size(), x_obs.data(), y_obs.data(), sigma);
  UEGradWL ue_grad_w_l(l, theta, config.parameters.post.batch_size, rng);
  UEGradWP ue_grad_w_post(ue_grad_w_h, ue_grad_w_l);

  // Unbiased estimator of the posterior expectation of the integral of
  // grad theta of the Hamiltonian
  auto theta_params = config.parameters.post.get_theta_params();
  theta_params.sgld_params.out_file = prefix + "_post_ws.csv";
  UEGradThetaPost ue_post_exp_int_grad_theta_H(
       ue_grad_w_post,
       ue_int_grad_theta_H,
       rng,
       theta_params
  );

  F grad_theta[h.get_num_params()];
  theta[0] = std::log(10000.0);
  ue_post_exp_int_grad_theta_H(theta, grad_theta);
  // theta[0] = std::log(10000.0);
  // ue_prior_exp_int_grad_theta_H(theta, grad_theta);

  // Postprocess the results
  const int n = config.postprocess.num_points_per_dim[0];
  postprocess<F>(
     phi, domain, config.postprocess.num_points_per_dim[0],
     theta_params.sgld_params.out_file,
     prefix
  );

  return 0;
} // main
