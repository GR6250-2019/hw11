// fms_lmm.h - LIBOR market model
/*
	The LIBOR market model has parameters, (t[j]), (f[j]), (sigma[j]), and alpha,
	where f[j] is the forward over (t[j-1], t[j]], sigma[j] is the corresponding
	at-the-money caplet implied volatility, and alpha is a correlation parameter 
	for two dimensional Brownian motion.

	The repo rates are F_j = phi[j] exp(sigma[j] B_{t_j} - sigma[j]^2 t[j]/2),
	where B_t = B0_t cos(alpha t) + B1_t sin(alpha t) and B0, B1 are independent
	Brownian motions.

	We assume the futures and foward satisfy phi(t) = f(t) + sigma^2 t^2/2.

	The futures quotes at time t are Phi_j(t) = E_t[F_j] = phi[j] exp(sigma[j] B_t - sigma[j]^2 t/2)
*/
#pragma once
#include <random>

namespace fms::lmm {

	inline std::default_random_engine dre;

	// Convert forwards to futures.
	template<class T, class F, class S>
	inline void to_futures(size_t n, const T* t, F* f, const S* sigma)
	{
		for (size_t i = 0; i < n; ++i) {
			f[i] += sigma[i] *sigma[i]* t[i] *t[i] / 2;
		}

	}

	// Convert futures to forwards.
	template<class T, class F, class S>
	inline void to_forwards(size_t n, const T* t, F* phi, const S* sigma)
	{
		for (size_t i = 0; i < n; ++i) {
			phi[i] -= sigma[i] * sigma[i] * t[i] * t[i] / 2;
		}
	}

	// Modify pointers to get the curve starting at time u.
	template<class T, class F, class S>
	inline size_t advance_futures(const T& u, size_t n, T*& t, F*& phi, S*& sigma, const F& alpha)
	{
		std::normal_distribution<T> N;

		// Generate two independent normal random variates.
		auto B0 = N(dre);
		auto B1 = N(dre);

		// Decrement n and increment t, phi, and sigma until t[i-1] <= u < t[i]
		while (n > 0 and *t <= u) {
			--n;
			++t;
			++phi;
			++sigma;
		}

		// Advance the remainin curve to time u: phi[i] *= exp(sigma[i]*B_u - sigma[i]^2 u/2)
		// where B_t = B0_t cos(alpha t) + B1_t sin(alpha t) and B0, B1 are independent
		for (size_t i = 0; i < n; ++i) {
			t[i] -= u;
			auto Bu = B0 * cos(alpha * t[i]) + B1 * sin(alpha * t[i]);
			phi[i] *= exp(sigma[i] * Bu - sigma[i] * sigma[i] * u / 2);
		}

		return n;
	}

	// Random forward curve at time u.
	template<class T, class F, class S>
	inline size_t advance(const T& u, size_t n, T*& t, F*& f, S*& sigma, const F& alpha)
	{
		to_futures(n, t, f, sigma);
		n = advance_futures(u, n, t, f, sigma, alpha);
		to_forwards(n, t, f, sigma);

		return n;
	}

	template<class T, class F>
	inline F par_coupon(size_t n, const T* t, const F* f)
	{
		F D0 = 1, Dn = 1;
		F D = 0; // sum D_j delta t_;
		T t0 = 0;
		for (size_t j = 0; j < n; ++j) {
			auto dt = t[j] - t0;
			Dn *= exp(-f[j] * dt);
			D += Dn * dt;
			t0 = t[j];
		}

		return (D0 - Dn) / D;
	}

}
