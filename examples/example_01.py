"""Replicate Example 1 of the paper.

TODO: Alex, you must modify this to generate the examples of the file.
"""


import jax.numpy as jnp
from jax.random import PRNGKey
import numpy as np
import matplotlib.pyplot as plt

from pift import *
from problems import Diffusion1D
from options import make_standard_option_parser

parser = make_standard_option_parser()
parser.add_option(
    "--beta",
    dest="beta",
    help="the beta you want to run the simulation on",
    type="float",
    default=0.0
)
parser.add_option(
    "--figure-format",
    dest="figure_format",
    help="the figure format",
    type="str",
    default="png"
)
(options, args) = parser.parse_args()

example = Diffusion1D()

num_terms = options.num_terms

V = FunctionParameterization.from_basis(
    "psi",
    Fourier1DBasis(example.b, options.num_terms)
)

problem = example.make_pift_problem(V)

rng_key = PRNGKey(123456)

mcmc = MCMCSampler(
    problem.pyro_model,
    rng_key=rng_key,
    num_warmup=options.num_warmup,
    num_samples=options.num_samples,
    thinning=options.thinning,
    progress_bar=True
)

beta = options.beta

samples = mcmc.sample(
    theta=[example.kappa * beta, beta]
)

xs = np.linspace(example.a, example.b, 200)
ws = samples["w"]
samples_out = f"example_01_beta={beta:1.1f}" + "_ws.csv"
np.savetxt(samples_out, ws)

ys_pift = problem.vwphi(xs, ws)
ys_out = f"example_01_beta={beta:1.1f}" + "_ys.csv"
np.savetxt(ys_out, ys_pift)

f = example.solve()
ys_true = f(xs)

# TODO - Make the figures compatible with paper
fig, ax = plt.subplots()
ax.plot(xs, ys_pift.T, "r", lw=0.1)
ax.plot(xs, ys_true, "b--")
ax.plot(example.a, example.yb[0], "ko")
ax.plot(example.b, example.yb[1], "ko")
ax.set_xlabel("$x$")
ax.set_ylabel(r"$\phi(x)$")
ax.set_title(f"$\\beta={beta:1.1f}$")

out_file = f"example_01_beta={beta:1.1f}" + "." + options.figure_format
print(f"> writing: {out_file}")
fig.savefig(out_file)
