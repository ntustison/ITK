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
#ifndef itkVectorLinearInterpolateImageFunction_hxx
#define itkVectorLinearInterpolateImageFunction_hxx


#include "itkMath.h"
#include <algorithm> // For min and max.

namespace itk
{

template <typename TInputImage, typename TCoordinate>
const unsigned long VectorLinearInterpolateImageFunction<TInputImage, TCoordinate>::m_Neighbors =
  1 << TInputImage::ImageDimension;


template <typename TInputImage, typename TCoordinate>
auto
VectorLinearInterpolateImageFunction<TInputImage, TCoordinate>::EvaluateAtContinuousIndex(
  const ContinuousIndexType & index) const -> OutputType
{
  //
  // Compute base index = closet index below point
  // Compute distance from point to base index
  //
  IndexType                 baseIndex;
  InternalComputationType   distance[ImageDimension];
  const TInputImage * const inputImgPtr = this->GetInputImage();
  for (unsigned int dim = 0; dim < ImageDimension; ++dim)
  {
    baseIndex[dim] = Math::Floor<IndexValueType>(index[dim]);
    distance[dim] = index[dim] - static_cast<InternalComputationType>(baseIndex[dim]);
  }

  /**
   * Interpolated value is the weighted sum of each of the surrounding
   * neighbors. The weight for each neighbor is the fraction overlap
   * of the neighbor pixel with respect to a pixel centered on point.
   */
  OutputType output{};

  using ScalarRealType = typename NumericTraits<PixelType>::ScalarRealType;
  ScalarRealType totalOverlap{};

  for (unsigned int counter = 0; counter < m_Neighbors; ++counter)
  {
    InternalComputationType overlap = 1.0;   // fraction overlap
    unsigned int            upper = counter; // each bit indicates upper/lower neighbour

    IndexType neighIndex;
    // get neighbor index and overlap fraction
    for (unsigned int dim = 0; dim < ImageDimension; ++dim)
    {
      if (upper & 1)
      {
        neighIndex[dim] = baseIndex[dim] + 1;
        // Take care of the case where the pixel is just
        // in the outer upper boundary of the image grid.
        neighIndex[dim] = std::min(neighIndex[dim], this->m_EndIndex[dim]);
        overlap *= distance[dim];
      }
      else
      {
        neighIndex[dim] = baseIndex[dim];
        // Take care of the case where the pixel is just
        // in the outer lower boundary of the image grid.
        neighIndex[dim] = std::max(neighIndex[dim], this->m_StartIndex[dim]);
        overlap *= 1.0 - distance[dim];
      }
      upper >>= 1;
    }

    // get neighbor value only if overlap is not zero
    if (overlap)
    {
      const PixelType & input = inputImgPtr->GetPixel(neighIndex);
      for (unsigned int k = 0; k < Dimension; ++k)
      {
        output[k] += overlap * static_cast<InternalComputationType>(input[k]);
      }
      totalOverlap += overlap;
    }

    if (totalOverlap == 1.0)
    {
      // finished
      break;
    }
  }

  return (output);
}
} // end namespace itk

#endif
