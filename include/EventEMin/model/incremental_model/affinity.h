#ifndef EVENT_EMIN_INCREMENTAL_AFFINITY_H
#define EVENT_EMIN_INCREMENTAL_AFFINITY_H

#include "EventEMin/model/incremental_model.h"

namespace EventEMin
{
namespace incremental
{
template <typename Scalar>
struct Affinity
{
  enum
  {
    NWt = 1,
    NWp = 1,
    NL = 2,
    NV = 2,
    NVars = NWt + NWp + NL + NV,
    NDims = 2,
    NMatrix = 3
  };

  typedef Scalar T;
  typedef Vector<T, NDims> Point;
  typedef Vector<T, NDims + 1> PointHomogeneous;
  typedef Vector<T, NVars> Vars;
  typedef Matrix<T, NDims, NDims> CMatrix;
  typedef Matrix<T, NDims, NVars> DMatrix;
  typedef Matrix<T, NDims, NVars> PMatrix;
  typedef Matrix<T, NMatrix, NMatrix> GMatrix;
  typedef Matrix<T, NMatrix, NMatrix> TMatrix;

  Affinity(void) = default;

  void
  operator()(const Ref<const Vars>& vars, const Ref<const Point>& c, const T& t,
             Ref<Point> cm) const
  {
    const Vars varst(t * vars);

    GMatrix gMatrix;
    generator(varst, gMatrix);

    PointHomogeneous ch;
    ch << c, T(1.0);

    // transformation
    transformation(gMatrix, ch, cm);
  }

  void
  operator()(const Ref<const Vars>& vars, const Ref<const Point>& c, const T& t,
             Ref<Point> cm, Ref<DMatrix> dcm, Ref<Point> cg,
             Ref<PMatrix> pMatrix) const
  {
    const Vars varst(t * vars);

    GMatrix gMatrix;
    generator(varst, gMatrix);

    PointHomogeneous ch;
    ch << c, T(1.0);

    // transformation
    transformation(gMatrix, ch, cm);

    // generator
    generator(gMatrix, ch, cg);

    // derivatives wrt to vars
    dtransformationdvars(ch, t, dcm);

    // perturbation
    perturbation(c, t, pMatrix);
  }

  void
  operator()(const Ref<const Vars>& vars, const Ref<const Point>& c, const T& t,
             Ref<Point> cm, Ref<DMatrix> dcm, Ref<CMatrix> dc) const
  {
    const Vars varst(t * vars);

    GMatrix gMatrix;
    generator(varst, gMatrix);

    PointHomogeneous ch;
    ch << c, T(1.0);

    // transformation
    TMatrix tMatrix;
    transformation(gMatrix, ch, cm, tMatrix);

    // derivatives wrt to vars
    dtransformationdvars(ch, t, dcm);

    // derivatives wrt to c
    dc = tMatrix.template topLeftCorner<NDims, NDims>();
  }

  void
  generator(const Ref<const GMatrix>& gMatrix,
            const Ref<const PointHomogeneous>& ch, Ref<Point> cg) const
  {
    cg.noalias() = gMatrix.template topRows<NDims>() * ch;
  }

  void
  generator(const Ref<const Vars>& varst, Ref<GMatrix> gMatrix) const
  {
    gMatrix << varst(1) + varst(2), varst(3) - varst(0), varst(4),
        varst(3) + varst(0), varst(1) - varst(2), varst(5), T(0.0), T(0.0),
        T(0.0);
  }

  void
  dgeneratordw(const T& t, Ref<GMatrix> dgMatrix) const
  {
    dgMatrix << T(0.0), -t, T(0.0), t, T(0.0), T(0.0), T(0.0), T(0.0), T(0.0);
  }

  void
  dgeneratords(const T& t, Ref<GMatrix> dgMatrix) const
  {
    dgMatrix << t, T(0.0), T(0.0), T(0.0), t, T(0.0), T(0.0), T(0.0), T(0.0);
  }

  void
  dgeneratordh(const T& t, Ref<GMatrix> dgMatrix) const
  {
    dgMatrix << t, T(0.0), T(0.0), T(0.0), -t, T(0.0), T(0.0), T(0.0), T(0.0);
  }

  void
  dgeneratordr(const T& t, Ref<GMatrix> dgMatrix) const
  {
    dgMatrix << T(0.0), t, T(0.0), t, T(0.0), T(0.0), T(0.0), T(0.0), T(0.0);
  }

  void
  dgeneratordvx(const T& t, Ref<GMatrix> dgMatrix) const
  {
    dgMatrix << T(0.0), T(0.0), t, T(0.0), T(0.0), T(0.0), T(0.0), T(0.0),
        T(0.0);
  }

  void
  dgeneratordvy(const T& t, Ref<GMatrix> dgMatrix) const
  {
    dgMatrix << T(0.0), T(0.0), T(0.0), T(0.0), T(0.0), t, T(0.0), T(0.0),
        T(0.0);
  }

  void
  perturbation(const Ref<const Point>& c, const T& t,
               Ref<PMatrix> pMatrix) const
  {
    const Point ct(t * c);
    pMatrix << -ct(1), ct(0), ct(0), ct(1), t, T(0.0), ct(0), ct(1), -ct(1),
        ct(0), T(0.0), t;
  }

  void
  transformation(const Ref<const GMatrix>& gMatrix,
                 const Ref<const PointHomogeneous>& ch, Ref<Point> cm) const
  {
    TMatrix tMatrix;
    transformation(gMatrix, ch, cm, tMatrix);
  }

  void
  transformation(const Ref<const GMatrix>& gMatrix,
                 const Ref<const PointHomogeneous>& ch, Ref<Point> cm,
                 Ref<TMatrix> tMatrix) const
  {
    transformationG(gMatrix, tMatrix);
    cm.noalias() = tMatrix.template topRows<NDims>() * ch;
  }

  void
  transformation(const Ref<const Vars>& varst, Ref<TMatrix> tMatrix) const
  {
    GMatrix gMatrix;
    generator(varst, gMatrix);
    transformationG(gMatrix, tMatrix);
  }

  void
  transformationG(const Ref<const GMatrix>& gMatrix, Ref<TMatrix> tMatrix) const
  {
    // 1st order Taylor series expansion
    tMatrix.setIdentity();
    tMatrix += gMatrix;
  }

  void
  dtransformationdvars(const Ref<const PointHomogeneous>& ch, const T& t,
                       Ref<DMatrix> dcm) const
  {
    // derivatives wrt to vars
    GMatrix dgMatrix;
    dgeneratordw(t, dgMatrix);
    dtransformationdvars(dgMatrix, ch, dcm.col(0));
    dgeneratords(t, dgMatrix);
    dtransformationdvars(dgMatrix, ch, dcm.col(1));
    dgeneratordh(t, dgMatrix);
    dtransformationdvars(dgMatrix, ch, dcm.col(2));
    dgeneratordr(t, dgMatrix);
    dtransformationdvars(dgMatrix, ch, dcm.col(3));
    dgeneratordvx(t, dgMatrix);
    dtransformationdvars(dgMatrix, ch, dcm.col(4));
    dgeneratordvy(t, dgMatrix);
    dtransformationdvars(dgMatrix, ch, dcm.col(5));
  }

  void
  dtransformationdvars(const Ref<const GMatrix>& dgMatrix,
                       const Ref<const PointHomogeneous>& ch,
                       Ref<Point> dcm) const
  {
    dcm.noalias() = dgMatrix.template topRows<NDims>() * ch;
  }
};
}  // namespace incremental

template <typename T>
using IncrementalAffinity = incremental::Model<incremental::Affinity<T> >;
}  // namespace EventEMin

#endif  // EVENT_EMIN_INCREMENTAL_AFFINITY_H
