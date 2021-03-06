#pragma once
/** @file LaplaceCartesian.hpp
 * @brief Implements the Laplace kernel with cartesian expansions.
 *
 * K(t,s) = 1 / |s-t|        // Laplace potential
 * K(t,s) = (s-t) / |s-t|^3  // Laplace force
 */

#include "Vec.hpp"

#include <type_traits>


typedef double real;

constexpr unsigned cart_index(unsigned nx, unsigned ny, unsigned nz) {
	return (nx+ny+nz)*(nx+ny+nz+1)*(nx+ny+nz+2)/6 + (ny+nz)*(ny+nz+1)/2 + nz;
}
constexpr unsigned factorial(unsigned k) {
	return k <= 1 ? 1 : k * factorial(k-1);
}
constexpr unsigned cart_value(unsigned nx, unsigned ny, unsigned nz) {
	return factorial(nx) * factorial(ny) * factorial(nz);
}

// Guarantee that compile time values are compile time
template <unsigned V>
struct u_constant : public std::integral_constant<unsigned, V> {};

template <int nx, int ny, int nz>
struct Index {
	static constexpr unsigned I = u_constant<cart_index(nx,ny,nz)>::value;
	static constexpr real     F = u_constant<cart_value(nx,ny,nz)>::value;
};



template<int n, int kx, int ky , int kz, int d>
struct DerivativeTerm {
  static constexpr int coef = 1 - 2 * n;
  template <typename Lset, typename vect>
  static inline real kernel(const Lset& C, const vect& dist) {
    return coef * dist[d] * C[Index<kx,ky,kz>::I];
  }
};
template<int n, int kx, int ky , int kz>
struct DerivativeTerm<n,kx,ky,kz,-1> {
  static constexpr int coef = 1 - n;
  template <typename Lset, typename vect>
      static inline real kernel(const Lset& C, const vect&) {
    return coef * C[Index<kx,ky,kz>::I];
  }
};


template<int nx, int ny, int nz, int kx=nx, int ky=ny, int kz=nz, int flag=5>
struct DerivativeSum {
  static constexpr int nextflag = 5 - (kz < nz || kz == 1);
  static constexpr int dim = kz == (nz-1) ? -1 : 2;
  static constexpr int n = nx + ny + nz;
  template <typename Lset, typename vect>
  static inline real loop(const Lset& C, const vect& dist) {
    return DerivativeSum<nx,ny,nz,nx,ny,kz-1,nextflag>::loop(C,dist)
        + DerivativeTerm<n,nx,ny,kz-1,dim>::kernel(C,dist);
  }
};
template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,4> {
  static constexpr int nextflag = 3 - (ny == 0);
  template <typename Lset, typename vect>
  static inline real loop(const Lset& C, const vect& dist) {
    return DerivativeSum<nx,ny,nz,nx,ny,nz,nextflag>::loop(C,dist);
  }
};
template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,3> {
  static constexpr int nextflag = 3 - (ky < ny || ky == 1);
  static constexpr int dim = ky == (ny-1) ? -1 : 1;
  static constexpr int n = nx + ny + nz;
  template <typename Lset, typename vect>
  static inline real loop(const Lset& C, const vect& dist) {
    return DerivativeSum<nx,ny,nz,nx,ky-1,nz,nextflag>::loop(C,dist)
        + DerivativeTerm<n,nx,ky-1,nz,dim>::kernel(C,dist);
  }
};
template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,2> {
  static constexpr int nextflag = 1 - (nx == 0);
  template <typename Lset, typename vect>
  static inline real loop(const Lset& C, const vect& dist) {
    return DerivativeSum<nx,ny,nz,nx,ny,nz,nextflag>::loop(C,dist);
  }
};
template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,1> {
  static constexpr int nextflag = 1 - (kx < nx || kx == 1);
  static constexpr int dim = kx == (nx-1) ? -1 : 0;
  static constexpr int n = nx + ny + nz;
  template <typename Lset, typename vect>
  static inline real loop(const Lset& C, const vect& dist) {
    return DerivativeSum<nx,ny,nz,kx-1,ny,nz,nextflag>::loop(C,dist)
        + DerivativeTerm<n,kx-1,ny,nz,dim>::kernel(C,dist);
  }
};
template<int nx, int ny, int nz, int kx, int ky, int kz>
struct DerivativeSum<nx,ny,nz,kx,ky,kz,0> {
  template <typename Lset, typename vect>
  static inline real loop(const Lset&, const vect&) {
    return 0;
  }
};
template<int nx, int ny, int nz, int kx, int ky>
struct DerivativeSum<nx,ny,nz,kx,ky,0,5> {
  template <typename Lset, typename vect>
  static inline real loop(const Lset& C, const vect& dist) {
    return DerivativeSum<nx,ny,nz,nx,ny,0,4>::loop(C,dist);
  }
};


template<int nx, int ny, int nz>
struct Terms {
  template <typename Lset, typename vect>
  static inline void power(Lset& C, const vect& dist) {
    Terms<nx,ny+1,nz-1>::power(C,dist);
    C[Index<nx,ny,nz>::I] = C[Index<nx,ny,nz-1>::I] * dist[2] / nz;
  }
  template <typename Lset, typename vect>
  static inline void derivative(Lset& C, const vect& dist, const real& invR2) {
    static constexpr int n = nx + ny + nz;
    Terms<nx,ny+1,nz-1>::derivative(C,dist,invR2);
    C[Index<nx,ny,nz>::I] = DerivativeSum<nx,ny,nz>::loop(C,dist) / n * invR2;
  }
  template <typename Lset>
  static inline void scale(Lset& C) {
    Terms<nx,ny+1,nz-1>::scale(C);
    C[Index<nx,ny,nz>::I] *= Index<nx,ny,nz>::F;
  }
};
template<int nx, int ny>
struct Terms<nx,ny,0> {
  template <typename Lset, typename vect>
  static inline void power(Lset& C, const vect& dist) {
    Terms<nx+1,0,ny-1>::power(C,dist);
    C[Index<nx,ny,0>::I] = C[Index<nx,ny-1,0>::I] * dist[1] / ny;
  }
  template <typename Lset, typename vect>
  static inline void derivative(Lset& C, const vect& dist, const real& invR2) {
    static constexpr int n = nx + ny;
    Terms<nx+1,0,ny-1>::derivative(C,dist,invR2);
    C[Index<nx,ny,0>::I] = DerivativeSum<nx,ny,0>::loop(C,dist) / n * invR2;
  }
  template <typename Lset>
  static inline void scale(Lset& C) {
    Terms<nx+1,0,ny-1>::scale(C);
    C[Index<nx,ny,0>::I] *= Index<nx,ny,0>::F;
  }
};
template<int nx>
struct Terms<nx,0,0> {
  template <typename Lset, typename vect>
  static inline void power(Lset& C, const vect& dist) {
    Terms<0,0,nx-1>::power(C,dist);
    C[Index<nx,0,0>::I] = C[Index<nx-1,0,0>::I] * dist[0] / nx;
  }
  template <typename Lset, typename vect>
  static inline void derivative(Lset& C, const vect& dist, const real& invR2) {
    static constexpr int n = nx;
    Terms<0,0,nx-1>::derivative(C,dist,invR2);
    C[Index<nx,0,0>::I] = DerivativeSum<nx,0,0>::loop(C,dist) / n * invR2;
  }
  template <typename Lset>
  static inline void scale(Lset& C) {
    Terms<0,0,nx-1>::scale(C);
    C[Index<nx,0,0>::I] *= Index<nx,0,0>::F;
  }
};
template<>
struct Terms<0,0,0> {
  template <typename Lset, typename vect>
  static inline void power(Lset&, const vect&) {}
  template <typename Lset, typename vect>
  static inline void derivative(Lset&, const vect&, const real&) {}
  template <typename Lset>
  static inline void scale(Lset&) {}
};


template<int nx, int ny, int nz, int kx, int ky, int kz>
struct M2MSum {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset& C, const Mset& M) {
    return M2MSum<nx,ny,nz,kx,ky,kz-1>::kernel(C,M)
        + C[Index<nx-kx,ny-ky,nz-kz>::I]*M[Index<kx,ky,kz>::I];
  }
};
template<int nx, int ny, int nz, int kx, int ky>
struct M2MSum<nx,ny,nz,kx,ky,0> {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset& C, const Mset& M) {
    return M2MSum<nx,ny,nz,kx,ky-1,nz>::kernel(C,M)
        + C[Index<nx-kx,ny-ky,nz>::I]*M[Index<kx,ky,0>::I];
  }
};
template<int nx, int ny, int nz, int kx>
struct M2MSum<nx,ny,nz,kx,0,0> {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset& C, const Mset& M) {
    return M2MSum<nx,ny,nz,kx-1,ny,nz>::kernel(C,M)
        + C[Index<nx-kx,ny,nz>::I]*M[Index<kx,0,0>::I];
  }
};
template<int nx, int ny, int nz>
struct M2MSum<nx,ny,nz,0,0,0> {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset&, const Mset&) { return 0; }
};


template<int nx, int ny, int nz, int kx, int ky, int kz>
struct M2LSum {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset& L, const Mset& M) {
    return M2LSum<nx,ny,nz,kx,ky+1,kz-1>::kernel(L,M)
        + M[Index<kx,ky,kz>::I] * L[Index<nx+kx,ny+ky,nz+kz>::I];
  }
};
template<int nx, int ny, int nz, int kx, int ky>
struct M2LSum<nx,ny,nz,kx,ky,0> {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset& L, const Mset& M) {
    return M2LSum<nx,ny,nz,kx+1,0,ky-1>::kernel(L,M)
        + M[Index<kx,ky,0>::I] * L[Index<nx+kx,ny+ky,nz>::I];
  }
};
template<int nx, int ny, int nz, int kx>
struct M2LSum<nx,ny,nz,kx,0,0> {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset& L, const Mset& M) {
    return M2LSum<nx,ny,nz,0,0,kx-1>::kernel(L,M)
        + M[Index<kx,0,0>::I] * L[Index<nx+kx,ny,nz>::I];
  }
};
template<int nx, int ny, int nz>
struct M2LSum<nx,ny,nz,0,0,0> {
  template <typename Lset, typename Mset>
  static inline real kernel(const Lset&, const Mset&) { return 0; }
};



template<int nx, int ny, int nz, int kx, int ky, int kz>
struct LocalSum {
  template <typename Lset>
  static inline real kernel(const Lset& C, const Lset& L) {
    return LocalSum<nx,ny,nz,kx,ky+1,kz-1>::kernel(C,L)
        + C[Index<kx,ky,kz>::I] * L[Index<nx+kx,ny+ky,nz+kz>::I];
  }
};
template<int nx, int ny, int nz, int kx, int ky>
struct LocalSum<nx,ny,nz,kx,ky,0> {
  template <typename Lset>
  static inline real kernel(const Lset& C, const Lset& L) {
    return LocalSum<nx,ny,nz,kx+1,0,ky-1>::kernel(C,L)
        + C[Index<kx,ky,0>::I] * L[Index<nx+kx,ny+ky,nz>::I];
  }
};
template<int nx, int ny, int nz, int kx>
struct LocalSum<nx,ny,nz,kx,0,0> {
  template <typename Lset>
  static inline real kernel(const Lset& C, const Lset& L) {
    return LocalSum<nx,ny,nz,0,0,kx-1>::kernel(C,L)
        + C[Index<kx,0,0>::I] * L[Index<nx+kx,ny,nz>::I];
  }
};
template<int nx, int ny, int nz>
struct LocalSum<nx,ny,nz,0,0,0> {
  template <typename Lset>
  static inline real kernel(const Lset&, const Lset&) { return 0; }
};


template<int nx, int ny, int nz>
struct Upward {
  template <typename Mset, typename Lset>
  static inline void M2M(Mset& MI, const Lset& C, const Mset& MJ) {
    Upward<nx,ny+1,nz-1>::M2M(MI,C,MJ);
    MI[Index<nx,ny,nz>::I] += M2MSum<nx,ny,nz,nx,ny,nz>::kernel(C,MJ);
  }
};
template<int nx, int ny>
struct Upward<nx,ny,0> {
  template <typename Mset, typename Lset>
  static inline void M2M(Mset& MI, const Lset& C, const Mset& MJ) {
    Upward<nx+1,0,ny-1>::M2M(MI,C,MJ);
    MI[Index<nx,ny,0>::I] += M2MSum<nx,ny,0,nx,ny,0>::kernel(C,MJ);
  }
};
template<int nx>
struct Upward<nx,0,0> {
  template <typename Mset, typename Lset>
  static inline void M2M(Mset& MI, const Lset& C, const Mset& MJ) {
    Upward<0,0,nx-1>::M2M(MI,C,MJ);
    MI[Index<nx,0,0>::I] += M2MSum<nx,0,0,nx,0,0>::kernel(C,MJ);
  }
};
template<>
struct Upward<0,0,0> {
  template <typename Mset, typename Lset>
  static inline void M2M(Mset&, const Lset&, const Mset&) {}
};




template<int P, int nx, int ny, int nz>
struct Downward {
  template <typename Mset, typename Lset>
  static inline void M2L(Lset& L, const Lset& C, const Mset& M) {
    Downward<P,nx,ny+1,nz-1>::M2L(L,C,M);
    L[Index<nx,ny,nz>::I] += M2LSum<nx,ny,nz,0,0,P-nx-ny-nz>::kernel(C,M);
  }
  template <typename Result, typename Mset, typename Lset>
  static inline void M2P(Result& B, const Lset& C, const Mset& M) {
    Downward<P,nx,ny+1,nz-1>::M2P(B,C,M);
    B[Index<nx,ny,nz>::I] += M2LSum<nx,ny,nz,0,0,P-nx-ny-nz>::kernel(C,M);
  }
  template <typename Lset>
  static inline void L2L(Lset& LI, const Lset& C, const Lset& LJ) {
    Downward<P,nx,ny+1,nz-1>::L2L(LI,C,LJ);
    LI[Index<nx,ny,nz>::I] += LocalSum<nx,ny,nz,0,0,P-nx-ny-nz>::kernel(C,LJ);
  }
  template <typename Result, typename Lset>
  static inline void L2P(Result& B, const Lset& C, const Lset& L) {
    Downward<P,nx,ny+1,nz-1>::L2P(B,C,L);
    B[Index<nx,ny,nz>::I] += LocalSum<nx,ny,nz,0,0,P-nx-ny-nz>::kernel(C,L);
  }
};

template<int P, int nx, int ny>
struct Downward<P,nx,ny,0> {
  template <typename Mset, typename Lset>
  static inline void M2L(Lset& L, const Lset& C, const Mset& M) {
    Downward<P,nx+1,0,ny-1>::M2L(L,C,M);
    L[Index<nx,ny,0>::I] += M2LSum<nx,ny,0,0,0,P-nx-ny>::kernel(C,M);
  }
  template <typename Result, typename Mset, typename Lset>
  static inline void M2P(Result& B, const Lset& C, const Mset& M) {
    Downward<P,nx+1,0,ny-1>::M2P(B,C,M);
    B[Index<nx,ny,0>::I] += M2LSum<nx,ny,0,0,0,P-nx-ny>::kernel(C,M);
  }
  template <typename Lset>
  static inline void L2L(Lset& LI, const Lset& C, const Lset& LJ) {
    Downward<P,nx+1,0,ny-1>::L2L(LI,C,LJ);
    LI[Index<nx,ny,0>::I] += LocalSum<nx,ny,0,0,0,P-nx-ny>::kernel(C,LJ);
  }
  template <typename Result, typename Lset>
  static inline void L2P(Result& B, const Lset& C, const Lset& L) {
    Downward<P,nx+1,0,ny-1>::L2P(B,C,L);
    B[Index<nx,ny,0>::I] += LocalSum<nx,ny,0,0,0,P-nx-ny>::kernel(C,L);
  }
};

template<int P, int nx>
struct Downward<P,nx,0,0> {
  template <typename Mset, typename Lset>
  static inline void M2L(Lset& L, const Lset& C, const Mset& M) {
    Downward<P,0,0,nx-1>::M2L(L,C,M);
    L[Index<nx,0,0>::I] += M2LSum<nx,0,0,0,0,P-nx>::kernel(C,M);
  }
  template <typename Result, typename Mset, typename Lset>
  static inline void M2P(Result& B, const Lset& C, const Mset& M) {
    Downward<P,0,0,nx-1>::M2P(B,C,M);
    B[Index<nx,0,0>::I] += M2LSum<nx,0,0,0,0,P-nx>::kernel(C,M);
  }
  template <typename Lset>
  static inline void L2L(Lset& LI, const Lset& C, const Lset& LJ) {
    Downward<P,0,0,nx-1>::L2L(LI,C,LJ);
    LI[Index<nx,0,0>::I] += LocalSum<nx,0,0,0,0,P-nx>::kernel(C,LJ);
  }
  template <typename Result, typename Lset>
  static inline void L2P(Result& B, const Lset& C, const Lset& L) {
    Downward<P,0,0,nx-1>::L2P(B,C,L);
    B[Index<nx,0,0>::I] += LocalSum<nx,0,0,0,0,P-nx>::kernel(C,L);
  }
};

template<int P>
struct Downward<P,0,0,0> {
  template <typename Mset, typename Lset>
  static inline void M2L(Lset&, const Lset&, const Mset&) {}
  template <typename Result, typename Mset, typename Lset>
  static inline void M2P(Result&, const Lset&, const Mset&) {}
  template <typename Lset>
  static inline void L2L(Lset&, const Lset&, const Lset&) {}
  template <typename Result, typename Lset>
  static inline void L2P(Result&, const Lset& , const Lset& ) {}
};

template<int P, typename Lset, typename vect>
inline void getCoef(Lset& C, const vect& dist, real& invR2, const real& invR) {
  C[0] = invR;
  Terms<0,0,P>::derivative(C,dist,invR2);
  Terms<0,0,P>::scale(C);
}

/*
  template<>
  inline void getCoef<1>(Lset& C, const vect& dist, real& invR2, const real& invR) {
  C[0] = invR;
  invR2 = -invR2;
  real x = dist[0], y = dist[1], z = dist[2];
  real invR3 = invR * invR2;
  C[1] = x * invR3;
  C[2] = y * invR3;
  C[3] = z * invR3;
  }

  template<>
  inline void getCoef<2>(Lset& C, const vect& dist, real& invR2, const real& invR) {
  getCoef<1>(C,dist,invR2,invR);
  real x = dist[0], y = dist[1], z = dist[2];
  real invR3 = invR * invR2;
  real invR5 = 3 * invR3 * invR2;
  real t = x * invR5;
  C[4] = x * t + invR3;
  C[5] = y * t;
  C[6] = z * t;
  t = y * invR5;
  C[7] = y * t + invR3;
  C[8] = z * t;
  C[9] = z * z * invR5 + invR3;
  }

  template<>
  inline void getCoef<3>(Lset& C, const vect& dist, real& invR2, const real& invR) {
  getCoef<2>(C,dist,invR2,invR);
  real x = dist[0], y = dist[1], z = dist[2];
  real invR3 = invR * invR2;
  real invR5 = 3 * invR3 * invR2;
  real invR7 = 5 * invR5 * invR2;
  real t = x * x * invR7;
  C[10] = x * (t + 3 * invR5);
  C[11] = y * (t +     invR5);
  C[12] = z * (t +     invR5);
  t = y * y * invR7;
  C[13] = x * (t +     invR5);
  C[16] = y * (t + 3 * invR5);
  C[17] = z * (t +     invR5);
  t = z * z * invR7;
  C[15] = x * (t +     invR5);
  C[18] = y * (t +     invR5);
  C[19] = z * (t + 3 * invR5);
  C[14] = x * y * z * invR7;
  }

  template<>
  inline void getCoef<4>(Lset& C, const vect& dist, real& invR2, const real& invR) {
  getCoef<3>(C,dist,invR2,invR);
  real x = dist[0], y = dist[1], z = dist[2];
  real invR3 = invR * invR2;
  real invR5 = 3 * invR3 * invR2;
  real invR7 = 5 * invR5 * invR2;
  real invR9 = 7 * invR7 * invR2;
  real t = x * x * invR9;
  C[20] = x * x * (t + 6 * invR7) + 3 * invR5;
  C[21] = x * y * (t + 3 * invR7);
  C[22] = x * z * (t + 3 * invR7);
  C[23] = y * y * (t +     invR7) + x * x * invR7 + invR5;
  C[24] = y * z * (t +     invR7);
  C[25] = z * z * (t +     invR7) + x * x * invR7 + invR5;
  t = y * y * invR9;
  C[26] = x * y * (t + 3 * invR7);
  C[27] = x * z * (t +     invR7);
  C[30] = y * y * (t + 6 * invR7) + 3 * invR5;
  C[31] = y * z * (t + 3 * invR7);
  C[32] = z * z * (t +     invR7) + y * y * invR7 + invR5;
  t = z * z * invR9;
  C[28] = x * y * (t +     invR7);
  C[29] = x * z * (t + 3 * invR7);
  C[33] = y * z * (t + 3 * invR7);
  C[34] = z * z * (t + 6 * invR7) + 3 * invR5;
  }

  template<>
  inline void getCoef<5>(Lset& C, const vect& dist, real& invR2, const real& invR) {
  getCoef<4>(C,dist,invR2,invR);
  real x = dist[0], y = dist[1], z = dist[2];
  real invR3 = invR * invR2;
  real invR5 = 3 * invR3 * invR2;
  real invR7 = 5 * invR5 * invR2;
  real invR9 = 7 * invR7 * invR2;
  real invR11 = 9 * invR9 * invR2;
  real t = x * x * invR11;
  C[35] = x * x * x * (t + 10 * invR9) + 15 * x * invR7;
  C[36] = x * x * y * (t +  6 * invR9) +  3 * y * invR7;
  C[37] = x * x * z * (t +  6 * invR9) +  3 * z * invR7;
  C[38] = x * y * y * (t +  3 * invR9) + x * x * x * invR9 + 3 * x * invR7;
  C[39] = x * y * z * (t +  3 * invR9);
  C[40] = x * z * z * (t +  3 * invR9) + x * x * x * invR9 + 3 * x * invR7;
  C[41] = y * y * y * (t +      invR9) + 3 * x * x * y * invR9 + 3 * y * invR7;
  C[42] = y * y * z * (t +      invR9) + x * x * z * invR9 + z * invR7;
  C[43] = y * z * z * (t +      invR9) + x * x * y * invR9 + y * invR7;
  C[44] = z * z * z * (t +      invR9) + 3 * x * x * z * invR9 + 3 * z * invR7;
  t = y * y * invR11;
  C[45] = x * y * y * (t +  6 * invR9) +  3 * x * invR7;
  C[46] = x * y * z * (t +  3 * invR9);
  C[47] = x * z * z * (t +      invR9) + x * y * y * invR9 + x * invR7;
  C[50] = y * y * y * (t + 10 * invR9) + 15 * y * invR7;
  C[51] = y * y * z * (t +  6 * invR9) + 3 * z * invR7;
  C[52] = y * z * z * (t +  3 * invR9) + y * y * y * invR9 + 3 * y * invR7;
  C[53] = z * z * z * (t +      invR9) + 3 * y * y * z * invR9 + 3 * z * invR7;
  t = z * z * invR11;
  C[48] = x * y * z * (t +  3 * invR9);
  C[49] = x * z * z * (t +  6 * invR9) +  3 * x * invR7;
  C[54] = y * z * z * (t +  6 * invR9) +  3 * y * invR7;
  C[55] = z * z * z * (t + 10 * invR9) + 15 * z * invR7;
  }

  template<>
  inline void getCoef<6>(Lset& C, const vect& dist, real& invR2, const real& invR) {
  getCoef<5>(C,dist,invR2,invR);
  real x = dist[0], y = dist[1], z = dist[2];
  real invR3 = invR * invR2;
  real invR5 = 3 * invR3 * invR2;
  real invR7 = 5 * invR5 * invR2;
  real invR9 = 7 * invR7 * invR2;
  real invR11 = 9 * invR9 * invR2;
  real invR13 = 11 * invR11 * invR2;
  real t = x * x * invR13;
  C[56] = x * x * x * x * (t + 15 * invR11) + 45 * x * x * invR9 + 15 * invR7;
  C[57] = x * x * x * y * (t + 10 * invR11) + 15 * x * y * invR9;
  C[58] = x * x * x * z * (t + 10 * invR11) + 15 * x * z * invR9;
  C[59] = x * x * y * y * (t +  6 * invR11) + x * x * x * x * invR11 + (6 * x * x + 3 * y * y) * invR9 + 3 * invR7;
  C[60] = x * x * y * z * (t +  6 * invR11) + 3 * y * z * invR9;
  C[61] = x * x * z * z * (t +  6 * invR11) + x * x * x * x * invR11 + (6 * x * x + 3 * z * z) * invR9 + 3 * invR7;
  C[62] = x * y * y * y * (t +  3 * invR11) + 3 * x * x * x * y * invR11 + 9 * x * y * invR9;
  C[63] = x * y * y * z * (t +  3 * invR11) + x * x * x * z * invR11 + 3 * x * z * invR9;
  C[64] = x * y * z * z * (t +  3 * invR11) + x * x * x * y * invR11 + 3 * x * y * invR9;
  C[65] = x * z * z * z * (t +  3 * invR11) + 3 * x * x * x * z * invR11 + 9 * x * z * invR9;
  C[66] = y * y * y * y * (t +      invR11) + 6 * x * x * y * y * invR11 + (3 * x * x + 6 * y * y) * invR9 + 3 * invR7;
  C[67] = y * y * y * z * (t +      invR11) + 3 * x * x * y * z * invR11 + 3 * y * z * invR9;
  C[68] = y * y * z * z * (t +      invR11) + (x * x * y * y + x * x * z * z) * invR11 + (x * x + y * y + z * z) * invR9 + invR7;
  C[69] = y * z * z * z * (t +      invR11) + 3 * x * x * y * z * invR11 + 3 * y * z * invR9;
  C[70] = z * z * z * z * (t +      invR11) + 6 * x * x * z * z * invR11 + (3 * x * x + 6 * z * z) * invR9 + 3 * invR7;
  t = y * y * invR13;
  C[71] = x * y * y * y * (t + 10 * invR11) + 15 * x * y * invR9;
  C[72] = x * y * y * z * (t +  6 * invR11) + 3 * x * z * invR9;
  C[73] = x * y * z * z * (t +  3 * invR11) + x * y * y * y * invR11 + 3 * x * y * invR9;
  C[74] = x * z * z * z * (t +      invR11) + 3 * x * y * y * z * invR11 + 3 * x * z * invR9;
  C[77] = y * y * y * y * (t + 15 * invR11) + 45 * y * y * invR9 + 15 * invR7;
  C[78] = y * y * y * z * (t + 10 * invR11) + 15 * y * z * invR9;
  C[79] = y * y * z * z * (t +  6 * invR11) + y * y * y * y * invR11 + (6 * y * y + 3 * z * z) * invR9 + 3 * invR7;
  C[80] = y * z * z * z * (t +  3 * invR11) + 3 * y * y * y * z * invR11 + 9 * y * z * invR9;
  C[81] = z * z * z * z * (t +      invR11) + 6 * y * y * z * z * invR11 + (3 * y * y + 6 * z * z) * invR9 + 3 * invR7;
  t = z * z * invR13;
  C[75] = x * y * z * z * (t +  6 * invR11) + 3 * x * y * invR9;
  C[76] = x * z * z * z * (t + 10 * invR11) + 15 * x * z * invR9;
  C[82] = y * z * z * z * (t + 10 * invR11) + 15 * y * z * invR9;
  C[83] = z * z * z * z * (t + 15 * invR11) + 45 * z * z * invR9 + 15 * invR7;
  }
*/

template<int P, typename Lset, typename Mset>
inline void sumM2L(Lset& L, const Lset& C, const Mset& M) {
  L += C;
  for (int i = 1; i < P*(P+1)*(P+2)/6; ++i)
    L[0] += M[i] * C[i];
  Downward<P,0,0,P-1>::M2L(L,C,M);
}

/*
  template<>
  inline void sumM2L<1>(Lset& L, const Lset& C, const Mset&) {
  L += C;
  }

  template<>
  inline void sumM2L<2>(Lset& L, const Lset& C, const Mset& M) {
  sumM2L<1>(L,C,M);
  L[0] += M[1]*C[1]+M[2]*C[2]+M[3]*C[3];
  L[1] += M[1]*C[4]+M[2]*C[5]+M[3]*C[6];
  L[2] += M[1]*C[5]+M[2]*C[7]+M[3]*C[8];
  L[3] += M[1]*C[6]+M[2]*C[8]+M[3]*C[9];
  }

  template<>
  inline void sumM2L<3>(Lset& L, const Lset& C, const Mset& M) {
  sumM2L<2>(L,C,M);
  for( int i=4; i<10; ++i ) L[0] += M[i]*C[i];
  L[1] += M[4]*C[10]+M[5]*C[11]+M[6]*C[12]+M[7]*C[13]+M[8]*C[14]+M[9]*C[15];
  L[2] += M[4]*C[11]+M[5]*C[13]+M[6]*C[14]+M[7]*C[16]+M[8]*C[17]+M[9]*C[18];
  L[3] += M[4]*C[12]+M[5]*C[14]+M[6]*C[15]+M[7]*C[17]+M[8]*C[18]+M[9]*C[19];
  L[4] += M[1]*C[10]+M[2]*C[11]+M[3]*C[12];
  L[5] += M[1]*C[11]+M[2]*C[13]+M[3]*C[14];
  L[6] += M[1]*C[12]+M[2]*C[14]+M[3]*C[15];
  L[7] += M[1]*C[13]+M[2]*C[16]+M[3]*C[17];
  L[8] += M[1]*C[14]+M[2]*C[17]+M[3]*C[18];
  L[9] += M[1]*C[15]+M[2]*C[18]+M[3]*C[19];
  }

  template<>
  inline void sumM2L<4>(Lset& L, const Lset& C, const Mset& M) {
  sumM2L<3>(L,C,M);
  for( int i=10; i<20; ++i ) L[0] += M[i]*C[i];
  L[1] += M[10]*C[20]+M[11]*C[21]+M[12]*C[22]+M[13]*C[23]+M[14]*C[24]+M[15]*C[25]+M[16]*C[26]+M[17]*C[27]+M[18]*C[28]+M[19]*C[29];
  L[2] += M[10]*C[21]+M[11]*C[23]+M[12]*C[24]+M[13]*C[26]+M[14]*C[27]+M[15]*C[28]+M[16]*C[30]+M[17]*C[31]+M[18]*C[32]+M[19]*C[33];
  L[3] += M[10]*C[22]+M[11]*C[24]+M[12]*C[25]+M[13]*C[27]+M[14]*C[28]+M[15]*C[29]+M[16]*C[31]+M[17]*C[32]+M[18]*C[33]+M[19]*C[34];
  L[4] += M[4]*C[20]+M[5]*C[21]+M[6]*C[22]+M[7]*C[23]+M[8]*C[24]+M[9]*C[25];
  L[5] += M[4]*C[21]+M[5]*C[23]+M[6]*C[24]+M[7]*C[26]+M[8]*C[27]+M[9]*C[28];
  L[6] += M[4]*C[22]+M[5]*C[24]+M[6]*C[25]+M[7]*C[27]+M[8]*C[28]+M[9]*C[29];
  L[7] += M[4]*C[23]+M[5]*C[26]+M[6]*C[27]+M[7]*C[30]+M[8]*C[31]+M[9]*C[32];
  L[8] += M[4]*C[24]+M[5]*C[27]+M[6]*C[28]+M[7]*C[31]+M[8]*C[32]+M[9]*C[33];
  L[9] += M[4]*C[25]+M[5]*C[28]+M[6]*C[29]+M[7]*C[32]+M[8]*C[33]+M[9]*C[34];
  L[10] += M[1]*C[20]+M[2]*C[21]+M[3]*C[22];
  L[11] += M[1]*C[21]+M[2]*C[23]+M[3]*C[24];
  L[12] += M[1]*C[22]+M[2]*C[24]+M[3]*C[25];
  L[13] += M[1]*C[23]+M[2]*C[26]+M[3]*C[27];
  L[14] += M[1]*C[24]+M[2]*C[27]+M[3]*C[28];
  L[15] += M[1]*C[25]+M[2]*C[28]+M[3]*C[29];
  L[16] += M[1]*C[26]+M[2]*C[30]+M[3]*C[31];
  L[17] += M[1]*C[27]+M[2]*C[31]+M[3]*C[32];
  L[18] += M[1]*C[28]+M[2]*C[32]+M[3]*C[33];
  L[19] += M[1]*C[29]+M[2]*C[33]+M[3]*C[34];
  }

  template<>
  inline void sumM2L<5>(Lset& L, const Lset& C, const Mset& M) {
  sumM2L<4>(L,C,M);
  for( int i=20; i<35; ++i ) L[0] += M[i]*C[i];
  L[1] += M[20]*C[35]+M[21]*C[36]+M[22]*C[37]+M[23]*C[38]+M[24]*C[39]+M[25]*C[40]+M[26]*C[41]+M[27]*C[42]+M[28]*C[43]+M[29]*C[44]+M[30]*C[45]+M[31]*C[46]+M[32]*C[47]+M[33]*C[48]+M[34]*C[49];
  L[2] += M[20]*C[36]+M[21]*C[38]+M[22]*C[39]+M[23]*C[41]+M[24]*C[42]+M[25]*C[43]+M[26]*C[45]+M[27]*C[46]+M[28]*C[47]+M[29]*C[48]+M[30]*C[50]+M[31]*C[51]+M[32]*C[52]+M[33]*C[53]+M[34]*C[54];
  L[3] += M[20]*C[37]+M[21]*C[39]+M[22]*C[40]+M[23]*C[42]+M[24]*C[43]+M[25]*C[44]+M[26]*C[46]+M[27]*C[47]+M[28]*C[48]+M[29]*C[49]+M[30]*C[51]+M[31]*C[52]+M[32]*C[53]+M[33]*C[54]+M[34]*C[55];
  L[4] += M[10]*C[35]+M[11]*C[36]+M[12]*C[37]+M[13]*C[38]+M[14]*C[39]+M[15]*C[40]+M[16]*C[41]+M[17]*C[42]+M[18]*C[43]+M[19]*C[44];
  L[5] += M[10]*C[36]+M[11]*C[38]+M[12]*C[39]+M[13]*C[41]+M[14]*C[42]+M[15]*C[43]+M[16]*C[45]+M[17]*C[46]+M[18]*C[47]+M[19]*C[48];
  L[6] += M[10]*C[37]+M[11]*C[39]+M[12]*C[40]+M[13]*C[42]+M[14]*C[43]+M[15]*C[44]+M[16]*C[46]+M[17]*C[47]+M[18]*C[48]+M[19]*C[49];
  L[7] += M[10]*C[38]+M[11]*C[41]+M[12]*C[42]+M[13]*C[45]+M[14]*C[46]+M[15]*C[47]+M[16]*C[50]+M[17]*C[51]+M[18]*C[52]+M[19]*C[53];
  L[8] += M[10]*C[39]+M[11]*C[42]+M[12]*C[43]+M[13]*C[46]+M[14]*C[47]+M[15]*C[48]+M[16]*C[51]+M[17]*C[52]+M[18]*C[53]+M[19]*C[54];
  L[9] += M[10]*C[40]+M[11]*C[43]+M[12]*C[44]+M[13]*C[47]+M[14]*C[48]+M[15]*C[49]+M[16]*C[52]+M[17]*C[53]+M[18]*C[54]+M[19]*C[55];
  L[10] += M[4]*C[35]+M[5]*C[36]+M[6]*C[37]+M[7]*C[38]+M[8]*C[39]+M[9]*C[40];
  L[11] += M[4]*C[36]+M[5]*C[38]+M[6]*C[39]+M[7]*C[41]+M[8]*C[42]+M[9]*C[43];
  L[12] += M[4]*C[37]+M[5]*C[39]+M[6]*C[40]+M[7]*C[42]+M[8]*C[43]+M[9]*C[44];
  L[13] += M[4]*C[38]+M[5]*C[41]+M[6]*C[42]+M[7]*C[45]+M[8]*C[46]+M[9]*C[47];
  L[14] += M[4]*C[39]+M[5]*C[42]+M[6]*C[43]+M[7]*C[46]+M[8]*C[47]+M[9]*C[48];
  L[15] += M[4]*C[40]+M[5]*C[43]+M[6]*C[44]+M[7]*C[47]+M[8]*C[48]+M[9]*C[49];
  L[16] += M[4]*C[41]+M[5]*C[45]+M[6]*C[46]+M[7]*C[50]+M[8]*C[51]+M[9]*C[52];
  L[17] += M[4]*C[42]+M[5]*C[46]+M[6]*C[47]+M[7]*C[51]+M[8]*C[52]+M[9]*C[53];
  L[18] += M[4]*C[43]+M[5]*C[47]+M[6]*C[48]+M[7]*C[52]+M[8]*C[53]+M[9]*C[54];
  L[19] += M[4]*C[44]+M[5]*C[48]+M[6]*C[49]+M[7]*C[53]+M[8]*C[54]+M[9]*C[55];
  L[20] += M[1]*C[35]+M[2]*C[36]+M[3]*C[37];
  L[21] += M[1]*C[36]+M[2]*C[38]+M[3]*C[39];
  L[22] += M[1]*C[37]+M[2]*C[39]+M[3]*C[40];
  L[23] += M[1]*C[38]+M[2]*C[41]+M[3]*C[42];
  L[24] += M[1]*C[39]+M[2]*C[42]+M[3]*C[43];
  L[25] += M[1]*C[40]+M[2]*C[43]+M[3]*C[44];
  L[26] += M[1]*C[41]+M[2]*C[45]+M[3]*C[46];
  L[27] += M[1]*C[42]+M[2]*C[46]+M[3]*C[47];
  L[28] += M[1]*C[43]+M[2]*C[47]+M[3]*C[48];
  L[29] += M[1]*C[44]+M[2]*C[48]+M[3]*C[49];
  L[30] += M[1]*C[45]+M[2]*C[50]+M[3]*C[51];
  L[31] += M[1]*C[46]+M[2]*C[51]+M[3]*C[52];
  L[32] += M[1]*C[47]+M[2]*C[52]+M[3]*C[53];
  L[33] += M[1]*C[48]+M[2]*C[53]+M[3]*C[54];
  L[34] += M[1]*C[49]+M[2]*C[54]+M[3]*C[55];
  }

  template<>
  inline void sumM2L<6>(Lset& L, const Lset& C, const Mset& M) {
  sumM2L<5>(L,C,M);
  for( int i=35; i<56; ++i ) L[0] += M[i]*C[i];
  L[1] += M[35]*C[56]+M[36]*C[57]+M[37]*C[58]+M[38]*C[59]+M[39]*C[60]+M[40]*C[61]+M[41]*C[62]+M[42]*C[63]+M[43]*C[64]+M[44]*C[65]+M[45]*C[66]+M[46]*C[67]+M[47]*C[68]+M[48]*C[69]+M[49]*C[70]+M[50]*C[71]+M[51]*C[72]+M[52]*C[73]+M[53]*C[74]+M[54]*C[75]+M[55]*C[76];
  L[2] += M[35]*C[57]+M[36]*C[59]+M[37]*C[60]+M[38]*C[62]+M[39]*C[63]+M[40]*C[64]+M[41]*C[66]+M[42]*C[67]+M[43]*C[68]+M[44]*C[69]+M[45]*C[71]+M[46]*C[72]+M[47]*C[73]+M[48]*C[74]+M[49]*C[75]+M[50]*C[77]+M[51]*C[78]+M[52]*C[79]+M[53]*C[80]+M[54]*C[81]+M[55]*C[82];
  L[3] += M[35]*C[58]+M[36]*C[60]+M[37]*C[61]+M[38]*C[63]+M[39]*C[64]+M[40]*C[65]+M[41]*C[67]+M[42]*C[68]+M[43]*C[69]+M[44]*C[70]+M[45]*C[72]+M[46]*C[73]+M[47]*C[74]+M[48]*C[75]+M[49]*C[76]+M[50]*C[78]+M[51]*C[79]+M[52]*C[80]+M[53]*C[81]+M[54]*C[82]+M[55]*C[83];
  L[4] += M[20]*C[56]+M[21]*C[57]+M[22]*C[58]+M[23]*C[59]+M[24]*C[60]+M[25]*C[61]+M[26]*C[62]+M[27]*C[63]+M[28]*C[64]+M[29]*C[65]+M[30]*C[66]+M[31]*C[67]+M[32]*C[68]+M[33]*C[69]+M[34]*C[70];
  L[5] += M[20]*C[57]+M[21]*C[59]+M[22]*C[60]+M[23]*C[62]+M[24]*C[63]+M[25]*C[64]+M[26]*C[66]+M[27]*C[67]+M[28]*C[68]+M[29]*C[69]+M[30]*C[71]+M[31]*C[72]+M[32]*C[73]+M[33]*C[74]+M[34]*C[75];
  L[6] += M[20]*C[58]+M[21]*C[60]+M[22]*C[61]+M[23]*C[63]+M[24]*C[64]+M[25]*C[65]+M[26]*C[67]+M[27]*C[68]+M[28]*C[69]+M[29]*C[70]+M[30]*C[72]+M[31]*C[73]+M[32]*C[74]+M[33]*C[75]+M[34]*C[76];
  L[7] += M[20]*C[59]+M[21]*C[62]+M[22]*C[63]+M[23]*C[66]+M[24]*C[67]+M[25]*C[68]+M[26]*C[71]+M[27]*C[72]+M[28]*C[73]+M[29]*C[74]+M[30]*C[77]+M[31]*C[78]+M[32]*C[79]+M[33]*C[80]+M[34]*C[81];
  L[8] += M[20]*C[60]+M[21]*C[63]+M[22]*C[64]+M[23]*C[67]+M[24]*C[68]+M[25]*C[69]+M[26]*C[72]+M[27]*C[73]+M[28]*C[74]+M[29]*C[75]+M[30]*C[78]+M[31]*C[79]+M[32]*C[80]+M[33]*C[81]+M[34]*C[82];
  L[9] += M[20]*C[61]+M[21]*C[64]+M[22]*C[65]+M[23]*C[68]+M[24]*C[69]+M[25]*C[70]+M[26]*C[73]+M[27]*C[74]+M[28]*C[75]+M[29]*C[76]+M[30]*C[79]+M[31]*C[80]+M[32]*C[81]+M[33]*C[82]+M[34]*C[83];
  L[10] += M[10]*C[56]+M[11]*C[57]+M[12]*C[58]+M[13]*C[59]+M[14]*C[60]+M[15]*C[61]+M[16]*C[62]+M[17]*C[63]+M[18]*C[64]+M[19]*C[65];
  L[11] += M[10]*C[57]+M[11]*C[59]+M[12]*C[60]+M[13]*C[62]+M[14]*C[63]+M[15]*C[64]+M[16]*C[66]+M[17]*C[67]+M[18]*C[68]+M[19]*C[69];
  L[12] += M[10]*C[58]+M[11]*C[60]+M[12]*C[61]+M[13]*C[63]+M[14]*C[64]+M[15]*C[65]+M[16]*C[67]+M[17]*C[68]+M[18]*C[69]+M[19]*C[70];
  L[13] += M[10]*C[59]+M[11]*C[62]+M[12]*C[63]+M[13]*C[66]+M[14]*C[67]+M[15]*C[68]+M[16]*C[71]+M[17]*C[72]+M[18]*C[73]+M[19]*C[74];
  L[14] += M[10]*C[60]+M[11]*C[63]+M[12]*C[64]+M[13]*C[67]+M[14]*C[68]+M[15]*C[69]+M[16]*C[72]+M[17]*C[73]+M[18]*C[74]+M[19]*C[75];
  L[15] += M[10]*C[61]+M[11]*C[64]+M[12]*C[65]+M[13]*C[68]+M[14]*C[69]+M[15]*C[70]+M[16]*C[73]+M[17]*C[74]+M[18]*C[75]+M[19]*C[76];
  L[16] += M[10]*C[62]+M[11]*C[66]+M[12]*C[67]+M[13]*C[71]+M[14]*C[72]+M[15]*C[73]+M[16]*C[77]+M[17]*C[78]+M[18]*C[79]+M[19]*C[80];
  L[17] += M[10]*C[63]+M[11]*C[67]+M[12]*C[68]+M[13]*C[72]+M[14]*C[73]+M[15]*C[74]+M[16]*C[78]+M[17]*C[79]+M[18]*C[80]+M[19]*C[81];
  L[18] += M[10]*C[64]+M[11]*C[68]+M[12]*C[69]+M[13]*C[73]+M[14]*C[74]+M[15]*C[75]+M[16]*C[79]+M[17]*C[80]+M[18]*C[81]+M[19]*C[82];
  L[19] += M[10]*C[65]+M[11]*C[69]+M[12]*C[70]+M[13]*C[74]+M[14]*C[75]+M[15]*C[76]+M[16]*C[80]+M[17]*C[81]+M[18]*C[82]+M[19]*C[83];
  L[20] += M[4]*C[56]+M[5]*C[57]+M[6]*C[58]+M[7]*C[59]+M[8]*C[60]+M[9]*C[61];
  L[21] += M[4]*C[57]+M[5]*C[59]+M[6]*C[60]+M[7]*C[62]+M[8]*C[63]+M[9]*C[64];
  L[22] += M[4]*C[58]+M[5]*C[60]+M[6]*C[61]+M[7]*C[63]+M[8]*C[64]+M[9]*C[65];
  L[23] += M[4]*C[59]+M[5]*C[62]+M[6]*C[63]+M[7]*C[66]+M[8]*C[67]+M[9]*C[68];
  L[24] += M[4]*C[60]+M[5]*C[63]+M[6]*C[64]+M[7]*C[67]+M[8]*C[68]+M[9]*C[69];
  L[25] += M[4]*C[61]+M[5]*C[64]+M[6]*C[65]+M[7]*C[68]+M[8]*C[69]+M[9]*C[70];
  L[26] += M[4]*C[62]+M[5]*C[66]+M[6]*C[67]+M[7]*C[71]+M[8]*C[72]+M[9]*C[73];
  L[27] += M[4]*C[63]+M[5]*C[67]+M[6]*C[68]+M[7]*C[72]+M[8]*C[73]+M[9]*C[74];
  L[28] += M[4]*C[64]+M[5]*C[68]+M[6]*C[69]+M[7]*C[73]+M[8]*C[74]+M[9]*C[75];
  L[29] += M[4]*C[65]+M[5]*C[69]+M[6]*C[70]+M[7]*C[74]+M[8]*C[75]+M[9]*C[76];
  L[30] += M[4]*C[66]+M[5]*C[71]+M[6]*C[72]+M[7]*C[77]+M[8]*C[78]+M[9]*C[79];
  L[31] += M[4]*C[67]+M[5]*C[72]+M[6]*C[73]+M[7]*C[78]+M[8]*C[79]+M[9]*C[80];
  L[32] += M[4]*C[68]+M[5]*C[73]+M[6]*C[74]+M[7]*C[79]+M[8]*C[80]+M[9]*C[81];
  L[33] += M[4]*C[69]+M[5]*C[74]+M[6]*C[75]+M[7]*C[80]+M[8]*C[81]+M[9]*C[82];
  L[34] += M[4]*C[70]+M[5]*C[75]+M[6]*C[76]+M[7]*C[81]+M[8]*C[82]+M[9]*C[83];
  L[35] += M[1]*C[56]+M[2]*C[57]+M[3]*C[58];
  L[36] += M[1]*C[57]+M[2]*C[59]+M[3]*C[60];
  L[37] += M[1]*C[58]+M[2]*C[60]+M[3]*C[61];
  L[38] += M[1]*C[59]+M[2]*C[62]+M[3]*C[63];
  L[39] += M[1]*C[60]+M[2]*C[63]+M[3]*C[64];
  L[40] += M[1]*C[61]+M[2]*C[64]+M[3]*C[65];
  L[41] += M[1]*C[62]+M[2]*C[66]+M[3]*C[67];
  L[42] += M[1]*C[63]+M[2]*C[67]+M[3]*C[68];
  L[43] += M[1]*C[64]+M[2]*C[68]+M[3]*C[69];
  L[44] += M[1]*C[65]+M[2]*C[69]+M[3]*C[70];
  L[45] += M[1]*C[66]+M[2]*C[71]+M[3]*C[72];
  L[46] += M[1]*C[67]+M[2]*C[72]+M[3]*C[73];
  L[47] += M[1]*C[68]+M[2]*C[73]+M[3]*C[74];
  L[48] += M[1]*C[69]+M[2]*C[74]+M[3]*C[75];
  L[49] += M[1]*C[70]+M[2]*C[75]+M[3]*C[76];
  L[50] += M[1]*C[71]+M[2]*C[77]+M[3]*C[78];
  L[51] += M[1]*C[72]+M[2]*C[78]+M[3]*C[79];
  L[52] += M[1]*C[73]+M[2]*C[79]+M[3]*C[80];
  L[53] += M[1]*C[74]+M[2]*C[80]+M[3]*C[81];
  L[54] += M[1]*C[75]+M[2]*C[81]+M[3]*C[82];
  L[55] += M[1]*C[76]+M[2]*C[82]+M[3]*C[83];
  }
*/

template <int P, typename Result, typename Lset, typename Mset>
inline void sumM2P(Result& B, const Lset& C, const Mset& M) {
  B[0] += C[0];
  B[1] += C[1];
  B[2] += C[2];
  B[3] += C[3];
  for (int i = 1; i<P*(P+1)*(P+2)/6; ++i)
    B[0] += M[i] * C[i];
  Downward<P,0,0,1>::M2P(B,C,M);
}


template <unsigned P>
class LaplaceCartesian
{
  //! Number of Cartesian mutlipole terms
  static constexpr int MTERM = P*(P+1)*(P+2)/6;
  //! Number of Cartesian local terms
  static constexpr int LTERM = (P+1)*(P+2)*(P+3)/6;

 public:
  //! The dimension of the Kernel
  static constexpr unsigned dimension = 3;
  //! Point type
  typedef Vec<dimension,real> point_type;
  //! Source type
  typedef point_type source_type;
  //! Target type
  typedef point_type target_type;
  //! Charge type
  typedef real charge_type;
  //! The return type of a kernel evaluation
  typedef Vec<4,real> kernel_value_type;
  //! The product of the kernel_value_type and the charge_type
  typedef Vec<4,real> result_type;

  //! Multipole expansion type
  typedef Vec<MTERM,real> multipole_type;
  //! Local expansion type
  typedef Vec<LTERM,real> local_type;

  /** Kernel evaluation
   * K(t,s)
   *
   * @param[in] t,s The target and source points to evaluate the kernel
   * @result The Laplace potential and force 4-vector on t from s:
   * Potential: 1/|s-t|  Force: (s-t)/|s-t|^3
   */
  kernel_value_type operator()(const point_type& t,
                               const point_type& s) const {
    point_type dist = s - t;         //   Vector from target to source
    real R2 = normSq(dist);          //   R^2
    real invR2 = 1.0 / R2;           //   1 / R^2
    if (R2 < 1e-8) invR2 = 0;        //   Exclude self interaction
    real invR = sqrt(invR2);         //   potential
    dist *= invR2 * invR;            //   force
    return kernel_value_type(invR, dist[0], dist[1], dist[2]);
  }

  /** Kernel P2M operation
   * M += Op(s) * c where M is the multipole and s is the source
   *
   * @param[in] source The source to accumulate into the multipole
   * @param[in] charge The source's corresponding charge
   * @param[in] center The center of the box containing the multipole expansion
   * @param[in,out] M The multipole expansion to accumulate into
   */
  void P2M(const source_type& source, const charge_type& charge,
           const point_type& center, multipole_type& M) const {
    point_type dist = center - source;
    multipole_type C;
    C[0] = charge;
    Terms<0,0,P-1>::power(C, dist);
    M += C;
  }

  /** Kernel M2M operator
   * M_t += Op(M_s) where M_t is the target and M_s is the source
   *
   * @param[in] source The multipole source at the child level
   * @param[in,out] target The multipole target to accumulate into
   * @param[in] translation The vector from source to target
   * @pre Msource includes the influence of all points within its box
   */
  void M2M(const multipole_type& Msource,
           multipole_type& Mtarget,
           const point_type& translation) const {
    multipole_type C;
    C[0] = 1;
    Terms<0,0,P-1>::power(C, translation);
    for (int i = 0; i < MTERM; ++i)
      Mtarget[i] += C[i] * Msource[0];
    Upward<0,0,P-1>::M2M(Mtarget, C, Msource);
  }

  /** Kernel M2L operation
   * L += Op(M)
   *
   * @param[in] M The multpole expansion source
   * @param[in,out] L The local expansion target
   * @param[in] translation The vector from source to target
   * @pre translation obeys the multipole-acceptance criteria
   * @pre Msource includes the influence of all points within its box
   */
  void M2L(const multipole_type& M,
           local_type& L,
           const point_type& translation) const {
    real invR2 = 1.0 / normSq(translation);
    real invR  = M[0] * sqrt(invR2);
    local_type C;
    getCoef<P>(C, translation, invR2, invR);
    sumM2L<P>(L, C, M);
  }

  /** Kernel M2P operation
   * r += Op(t, M) where M is the multipole and r is the result
   *
   * @param[in] M The multpole expansion
   * @param[in] center The center of the box with the multipole expansion
   * @param[in] target The target to evaluate the multipole at
   * @param[in,out] result The target's corresponding result to accumulate into
   * @pre M includes the influence of all sources within its box
   */
  void M2P(const multipole_type& M, const point_type& center,
           const target_type& target, result_type& result) const {
    point_type dist = target - center;
    real invR2 = 1.0 / normSq(dist);
    real invR  = M[0] * sqrt(invR2);
    local_type C;
    getCoef<P>(C, dist, invR2, invR);
    sumM2P<P>(result, C, M);
  }

  /** Kernel L2L operator
   * L_t += Op(L_s) where L_t is the target and L_s is the source
   *
   * @param[in] source The local source at the parent level
   * @param[in,out] target The local target to accumulate into
   * @param[in] translation The vector from source to target
   * @pre Lsource includes the influence of all points outside its box
   */
  void L2L(const local_type& Lsource,
           local_type& Ltarget,
           const point_type& translation) const {
    local_type C;
    C[0] = 1;
    Terms<0,0,P>::power(C, translation);
    Ltarget += Lsource;
    for (int i = 1; i < LTERM; ++i)
      Ltarget[0] += C[i] * Lsource[i];
    Downward<P,0,0,P-1>::L2L(Ltarget, C, Lsource);
  }

  /** Kernel L2P operation
   * r += Op(t, L) where L is the local expansion and r is the result
   *
   * @param[in] L The local expansion
   * @param[in] center The center of the box with the local expansion
   * @param[in] target The target of this L2P operation
   * @param[in] result The result to accumulate into
   * @pre L includes the influence of all sources outside its box
   */
  void L2P(const local_type& L, const point_type& center,
           const target_type& target, result_type& result) const {
    point_type dist = target - center;
    local_type C;
    C[0] = 1;
    Terms<0,0,P>::power(C, dist);
    result[0] += L[0];
    result[1] += L[1];
    result[2] += L[2];
    result[3] += L[3];
    for (int i = 1; i < LTERM; ++i)
      result[0] += C[i] * L[i];
    Downward<P,0,0,1>::L2P(result, C, L);
  }
};
