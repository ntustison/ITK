/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkGaussianDerivativeOperator_hxx
#define itkGaussianDerivativeOperator_hxx

#include "itkCompensatedSummation.h"
#include "itkOutputWindow.h"
#include "itkMacro.h"
#include <numeric>

namespace itk
{

template <typename TPixel, unsigned int VDimension, typename TAllocator>
auto
GaussianDerivativeOperator<TPixel, VDimension, TAllocator>::GenerateCoefficients() -> CoefficientVector
{

  // compute gaussian kernel of 0-order
  CoefficientVector coeff = this->GenerateGaussianCoefficients();

  if (m_Order == 0)
  {
    return coeff;
  }


  // Calculate scale-space normalization factor for derivatives
  double norm = 1.0;
  if (m_NormalizeAcrossScale)
  {
    norm = std::pow(m_Variance, m_Order / 2.0);
  }

  // additional normalization for spacing
  norm /= std::pow(m_Spacing, static_cast<int>(m_Order));

  DerivativeOperatorType derivOp;
  derivOp.SetDirection(this->GetDirection());
  derivOp.SetOrder(m_Order);
  derivOp.CreateDirectional();

  // The input gaussian kernel needs to be padded with a clamped
  // boundary condition. If N is the radius of the derivative
  // operator, then the output kernel needs to be padded by N-1. For
  // these values to be computed the input kernel needs to be padded
  // by 2N-1 on both sides.
  const unsigned int N = (derivOp.Size() - 1) / 2;

  // copy the gaussian operator adding clamped boundary condition
  CoefficientVector paddedCoeff(coeff.size() + 4 * N - 2);

  // copy the whole gaussian operator in coeff to paddedCoef
  // starting after the padding
  std::copy(coeff.begin(), coeff.end(), paddedCoeff.begin() + 2 * N - 1);

  // pad paddedCoeff with 2*N-1 number of boundary conditions
  std::fill(paddedCoeff.begin(), paddedCoeff.begin() + 2 * N, coeff.front());
  std::fill(paddedCoeff.rbegin(), paddedCoeff.rbegin() + 2 * N, coeff.back());

  // clear for output kernel/coeffs
  coeff = CoefficientVector();

  // Now perform convolution between derivative operators and padded gaussian
  for (unsigned int i = N; i < paddedCoeff.size() - N; ++i)
  {
    CompensatedSummation<double> conv;

    // current index in derivative op
    for (unsigned int j = 0; j < derivOp.Size(); ++j)
    {
      const unsigned int k = i + j - derivOp.Size() / 2;
      conv += paddedCoeff[k] * derivOp[derivOp.Size() - 1 - j];
    }

    // normalize for scale-space and spacing
    coeff.push_back(norm * conv.GetSum());
  }

  return coeff;
}

template <typename TPixel, unsigned int VDimension, typename TAllocator>
auto
GaussianDerivativeOperator<TPixel, VDimension, TAllocator>::GenerateGaussianCoefficients() const -> CoefficientVector
{
  // Use image spacing to modify variance
  const double pixelVariance = m_Variance / (m_Spacing * m_Spacing);

  // Now create coefficients as if they were zero order coeffs
  const double et = std::exp(-pixelVariance);
  const double cap = 1.0 - m_MaximumError;


  // Create the kernel coefficients as a std::vector
  CoefficientVector coeff;
  coeff.push_back(et * ModifiedBesselI0(pixelVariance));

  CompensatedSummation<double> sum = coeff[0];
  coeff.push_back(et * ModifiedBesselI1(pixelVariance));
  sum += coeff[1] * 2.0;

  for (int i = 2; sum.GetSum() < cap; ++i)
  {
    coeff.push_back(et * ModifiedBesselI(i, pixelVariance));
    sum += coeff[i] * 2.0;
    if (coeff[i] < sum.GetSum() * NumericTraits<double>::epsilon())
    {
      // if the coeff is less then this value then the value of cap
      // will not change, and it will not contribute to the operator
      itkWarningMacro("Kernel failed to accumulate to approximately one with current remainder "
                      << cap - sum.GetSum() << " and current coefficient " << coeff[i] << '.');

      break;
    }
    if (coeff.size() > m_MaximumKernelWidth)
    {
      itkWarningMacro("Kernel size has exceeded the specified maximum width of "
                      << m_MaximumKernelWidth << " and has been truncated to "
                      << static_cast<unsigned long>(coeff.size())
                      << " elements.  You can raise "
                         "the maximum width using the SetMaximumKernelWidth method.");
      break;
    }
  }

  // re-accumulate from smallest number to largest for maximum precision
  sum = std::accumulate(coeff.rbegin(), coeff.rend() - 1, 0.0);
  sum *= 2.0;
  sum += coeff[0]; // the first is only needed once

  // Normalize the coefficients so they sum one
  for (auto it = coeff.begin(); it != coeff.end(); ++it)
  {
    *it /= sum.GetSum();
  }

  // Make symmetric
  const size_t s = coeff.size() - 1;
  coeff.insert(coeff.begin(), s, 0);
  std::copy_n(coeff.rbegin(), s, coeff.begin());

  return coeff;
}

template <typename TPixel, unsigned int VDimension, typename TAllocator>
double
GaussianDerivativeOperator<TPixel, VDimension, TAllocator>::ModifiedBesselI0(double y)
{
  double accumulator = NAN;

  const double d = itk::Math::abs(y);
  if (d < 3.75)
  {
    double m = y / 3.75;
    m *= m;
    accumulator =
      1.0 + m * (3.5156229 + m * (3.0899424 + m * (1.2067492 + m * (0.2659732 + m * (0.360768e-1 + m * 0.45813e-2)))));
  }
  else
  {
    const double m = 3.75 / d;
    accumulator =
      (std::exp(d) / std::sqrt(d)) *
      (0.39894228 +
       m * (0.1328592e-1 +
            m * (0.225319e-2 +
                 m * (-0.157565e-2 +
                      m * (0.916281e-2 +
                           m * (-0.2057706e-1 + m * (0.2635537e-1 + m * (-0.1647633e-1 + m * 0.392377e-2))))))));
  }
  return accumulator;
}

template <typename TPixel, unsigned int VDimension, typename TAllocator>
double
GaussianDerivativeOperator<TPixel, VDimension, TAllocator>::ModifiedBesselI1(double y)
{
  double       accumulator = NAN;
  const double d = itk::Math::abs(y);
  if (d < 3.75)
  {
    double m = y / 3.75;
    m *= m;
    accumulator =
      d * (0.5 + m * (0.87890594 +
                      m * (0.51498869 + m * (0.15084934 + m * (0.2658733e-1 + m * (0.301532e-2 + m * 0.32411e-3))))));
  }
  else
  {
    const double m = 3.75 / d;
    accumulator = 0.2282967e-1 + m * (-0.2895312e-1 + m * (0.1787654e-1 - m * 0.420059e-2));
    accumulator =
      0.39894228 + m * (-0.3988024e-1 + m * (-0.362018e-2 + m * (0.163801e-2 + m * (-0.1031555e-1 + m * accumulator))));

    accumulator *= (std::exp(d) / std::sqrt(d));
  }

  if (y < 0.0)
  {
    return -accumulator;
  }

  return accumulator;
}

template <typename TPixel, unsigned int VDimension, typename TAllocator>
double
GaussianDerivativeOperator<TPixel, VDimension, TAllocator>::ModifiedBesselI(int n, double y)
{
  constexpr double DIGITS = 10.0;

  if (n < 2)
  {
    throw ExceptionObject(__FILE__, __LINE__, "Order of modified bessel is > 2.", ITK_LOCATION); //
                                                                                                 // placeholder
  }

  double accumulator = NAN;
  if (y == 0.0)
  {
    return 0.0;
  }

  const double toy = 2.0 / itk::Math::abs(y);
  double       qip = accumulator = 0.0;
  double       qi = 1.0;
  for (int j = 2 * (n + static_cast<int>(DIGITS * std::sqrt(static_cast<double>(n)))); j > 0; j--)
  {
    const double qim = qip + j * toy * qi;
    qip = qi;
    qi = qim;
    if (itk::Math::abs(qi) > 1.0e10)
    {
      accumulator *= 1.0e-10;
      qi *= 1.0e-10;
      qip *= 1.0e-10;
    }
    if (j == n)
    {
      accumulator = qip;
    }
  }
  accumulator *= ModifiedBesselI0(y) / qi;
  if (y < 0.0 && (n & 1))
  {
    return -accumulator;
  }
  else
  {
    return accumulator;
  }
}

template <typename TPixel, unsigned int VDimension, typename TAllocator>
void
GaussianDerivativeOperator<TPixel, VDimension, TAllocator>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "NormalizeAcrossScale: " << m_NormalizeAcrossScale << std::endl;
  os << indent << "Variance: " << m_Variance << std::endl;
  os << indent << "MaximumError: " << m_MaximumError << std::endl;
  os << indent << "MaximumKernelWidth: " << m_MaximumKernelWidth << std::endl;
  os << indent << "Order: " << m_Order << std::endl;
  os << indent << "Spacing: " << m_Spacing << std::endl;
}

} // end namespace itk

#endif
