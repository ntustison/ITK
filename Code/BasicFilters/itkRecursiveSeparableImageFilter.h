/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkRecursiveSeparableImageFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) 2000 National Library of Medicine
  All rights reserved.

  See COPYRIGHT.txt for copyright details.

=========================================================================*/
#ifndef __itkRecursiveSeparableImageFilter_h
#define __itkRecursiveSeparableImageFilter_h

#include "itkImageToImageFilter.h"

namespace itk
{
  
/** \class RecursiveSeparableImageFilter
 * \brief Base class for recursive convolution with a kernel.
 *
 * RecursiveSeparableImageFilter is the base class for recursive 
 * filters that are applied in each dimension separatedly
 * 
 * This class implements the recursive filtering
 * method proposed by R.Deriche in IEEE-PAMI
 * Vol.12, No.1, January 1990, pp 78-87.
 */

template <class TInputImage, class TOutputImage, class TComputation>
class ITK_EXPORT RecursiveSeparableImageFilter :
    public ImageSource<TOutputImage> 

{
public:
  /**
   * Standard "Self" typedef.
   */
  typedef RecursiveSeparableImageFilter  Self;

  /**
   * Standard "Superclass" typedef.
   */
  typedef ImageSource<TOutputImage>   Superclass;

  /** 
   * Smart pointer typedef support 
   */
  typedef SmartPointer<Self>   Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  /** 
   * Smart pointer typedef support 
   */
  typedef typename TInputImage::Pointer  InputImagePointer;

  /**
   * Type macro that defines a name for this class
   */
  itkTypeMacro( RecursiveSeparableImageFilter, ImageToImageFilter );

  /**
   * Method for creation through the object factory.
   */
  itkNewMacro(Self);
  
  /**
   * Get the direction in which the filter is to be applied
   */   
  itkGetMacro(Direction, unsigned int);

  /**
   * Set the direction in which the filter is to be applied
   */   
  itkSetMacro(Direction, unsigned int);

  /**
   * Set Input Image
   */
  void SetInputImage( InputImagePointer );
    
  /**
   * Get Input Image
   */
//  typename TInputImage::Pointer GetInputImage( void );
  TInputImage * GetInputImage( void );

  /**
   * GenerateData (apply) the filter
   */   
  void GenerateData(void);


protected:
  RecursiveSeparableImageFilter();
  
  virtual ~RecursiveSeparableImageFilter() {};
  
  RecursiveSeparableImageFilter(const Self&) {}
  
  void operator=(const Self&) {}

  /**
   * Set up the coefficients of the filter to approximate a specific kernel.
   * typically it can be used to approximate a gaussian or one of its
   * derivatives.
   */
  virtual void SetUp(void) = 0;

  /**
   * Apply the recursive filter along one of the dimensions of the image.
   * This allow to filter each one of the dimensions of an image separately.
   * Sigma is given in length units so the spacing between pixels is taken 
   * into account. This is relevant for anisotropic images
   */
  void ApplyRecursiveFilter(unsigned int dimension);

  /**
   * Compute Recursive Filter Coefficients this method prepares the values of
   * the coefficients used for filtering the image. The symmetric flag is
   * used to enforce that the filter will be symmetric or antisymmetric. For
   * example, the Gaussian kernel is symmetric, while its first derivative is
   * antisymmetric.
   */
  virtual void ComputeFilterCoefficients(bool symmetric) = 0;

  /**
   * Apply the Recursive Filter in an array of data.  this method is called
   * for each line of the volume from ApplyRecursiveFilter.
   * \sa ApplyRecursiveFilter 
   */
  void FilterDataArray(TComputation *outs,
                       const TComputation *data, unsigned int ln);


private:  

  /**
   * Direction in which the filter is to be applied
   * this shoul in the range [0,ImageDimension-1]
   */ 
  unsigned int m_Direction;


protected:
  
  /**
   *  Normalization factor
   */
  TComputation K;                       


  /**
   * Spacing along the direction of filtering
   */   
  TComputation m_Spacing;


  /**
   *  Parameter of exponential series
   */
  TComputation a0;
  TComputation a1;
  TComputation b0;
  TComputation b1;
  TComputation c0;
  TComputation c1;
  TComputation w0;
  TComputation w1; 


  /**
   * Causal coefficients
   */
  TComputation n00;
  TComputation n11;
  TComputation n22;
  TComputation n33; 

  
  /**
   * Causal coefficients == Anticausal coefficients
   */
  TComputation d11;
  TComputation d22;
  TComputation d33;
  TComputation d44; 

  
  /**
   * Anti-Causal coefficients (symmetric case)
   */
  TComputation m11;
  TComputation m22;
  TComputation m33;
  TComputation m44; 

};


} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRecursiveSeparableImageFilter.txx"
#endif


#endif
