/*=========================================================================
  Program:   vv                     http://www.creatis.insa-lyon.fr/rio/vv

  Authors belong to: 
  - University of LYON              http://www.universite-lyon.fr/
  - Léon Bérard cancer center       http://www.centreleonberard.fr
  - CREATIS CNRS laboratory         http://www.creatis.insa-lyon.fr

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the copyright notices for more information.

  It is distributed under dual licence

  - BSD        See included LICENSE.txt file
  - CeCILL-B   http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
===========================================================================**/
#ifndef _clitkExtractImageFilter_txx
#define _clitkExtractImageFilter_txx

#include "clitkExtractImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkObjectFactory.h"
#include "itkExtractImageFilterRegionCopier.h"
#include "itkProgressReporter.h"


namespace clitk
{

/**
 *
 */
template <class TInputImage, class TOutputImage>
ExtractImageFilter<TInputImage,TOutputImage>
::ExtractImageFilter() 
{
}


/**
 *
 */
template <class TInputImage, class TOutputImage>
void 
ExtractImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "ExtractionRegion: " << m_ExtractionRegion << std::endl;
  os << indent << "OutputImageRegion: " << m_OutputImageRegion << std::endl;
}


template<class TInputImage, class TOutputImage>
void 
ExtractImageFilter<TInputImage,TOutputImage>
::CallCopyOutputRegionToInputRegion(InputImageRegionType &destRegion,
                                    const OutputImageRegionType &srcRegion)
{
  ExtractImageFilterRegionCopierType extractImageRegionCopier;
  extractImageRegionCopier(destRegion, srcRegion, m_ExtractionRegion);
}


template <class TInputImage, class TOutputImage>
void 
ExtractImageFilter<TInputImage,TOutputImage>
::SetExtractionRegion(InputImageRegionType extractRegion)
{
  m_ExtractionRegion = extractRegion;

  unsigned int nonzeroSizeCount = 0;
  InputImageSizeType inputSize = extractRegion.GetSize();
  OutputImageSizeType outputSize;
  OutputImageIndexType outputIndex;

  /**
   * check to see if the number of non-zero entries in the extraction region
   * matches the number of dimensions in the output image.  
   **/
  for (unsigned int i = 0; i < InputImageDimension; ++i)
    {
    if (inputSize[i])
      { 
      outputSize[nonzeroSizeCount] = inputSize[i];    
      outputIndex[nonzeroSizeCount] = extractRegion.GetIndex()[i];
      nonzeroSizeCount++;
      }
    }
    
  if (nonzeroSizeCount != OutputImageDimension)
    {
    itkExceptionMacro("Extraction Region not consistent with output image");
    }

  m_OutputImageRegion.SetSize(outputSize);
  m_OutputImageRegion.SetIndex(outputIndex);
  this->Modified();
}



/** 
 * ExtractImageFilter can produce an image which is a different resolution
 * than its input image.  As such, ExtractImageFilter needs to provide an
 * implementation for GenerateOutputInformation() in order to inform
 * the pipeline execution model.  The original documentation of this
 * method is below.
 *
 * \sa ProcessObject::GenerateOutputInformaton() 
 */
template <class TInputImage, class TOutputImage>
void 
ExtractImageFilter<TInputImage,TOutputImage>
::GenerateOutputInformation()
{
  // do not call the superclass' implementation of this method since
  // this filter allows the input and the output to be of different dimensions
 
  // get pointers to the input and output
  typename Superclass::OutputImagePointer      outputPtr = this->GetOutput();
  typename Superclass::InputImageConstPointer  inputPtr  = this->GetInput();

  if ( !outputPtr || !inputPtr)
    {
    return;
    }

  // Set the output image size to the same value as the extraction region.
  outputPtr->SetLargestPossibleRegion( m_OutputImageRegion );

  // Set the output spacing and origin
  const itk::ImageBase<InputImageDimension> *phyData;

  phyData
    = dynamic_cast<const itk::ImageBase<InputImageDimension>*>(this->GetInput());

  if (phyData)
    {
    // Copy what we can from the image from spacing and origin of the input
    // This logic needs to be augmented with logic that select which
    // dimensions to copy

    unsigned int i;
    const typename InputImageType::SpacingType& 
      inputSpacing = inputPtr->GetSpacing();
    const typename InputImageType::DirectionType&
      inputDirection = inputPtr->GetDirection();
    const typename InputImageType::PointType&
      inputOrigin = inputPtr->GetOrigin();

    typename OutputImageType::SpacingType outputSpacing;
    typename OutputImageType::DirectionType outputDirection;
    typename OutputImageType::PointType outputOrigin;

    if ( static_cast<unsigned int>(OutputImageDimension) > 
         static_cast<unsigned int>(InputImageDimension )    )
      {
      // copy the input to the output and fill the rest of the
      // output with zeros.
      for (i=0; i < InputImageDimension; ++i)
        {
        outputSpacing[i] = inputSpacing[i];
        outputOrigin[i] = inputOrigin[i];
        for (unsigned int dim = 0; dim < InputImageDimension; ++dim)
          {
          outputDirection[i][dim] = inputDirection[i][dim];
          }
        }
      for (; i < OutputImageDimension; ++i)
        {
        outputSpacing[i] = 1.0;
        outputOrigin[i] = 0.0;
        for (unsigned int dim = 0; dim < InputImageDimension; ++dim)
          {
          outputDirection[i][dim] = 0.0;
          }
        outputDirection[i][i] = 1.0;
        }
      }
    else
      {
      // copy the non-collapsed part of the input spacing and origing to the output
      int nonZeroCount = 0;
      for (i=0; i < InputImageDimension; ++i)
        {
        if (m_ExtractionRegion.GetSize()[i])
          {
          outputSpacing[nonZeroCount] = inputSpacing[i];
          outputOrigin[nonZeroCount] = inputOrigin[i];
          int nonZeroCount2 = 0;
          for (unsigned int dim = 0; dim < InputImageDimension; ++dim)
            {
            if (m_ExtractionRegion.GetSize()[dim])
              {
              outputDirection[nonZeroCount][nonZeroCount2] =
                inputDirection[i][dim];
              ++nonZeroCount2;
              }
            }
          nonZeroCount++;
          }
        }
      }

    // set the spacing and origin
    outputPtr->SetSpacing( outputSpacing );
    outputPtr->SetDirection( outputDirection );
    outputPtr->SetOrigin( outputOrigin );
    outputPtr->SetNumberOfComponentsPerPixel(inputPtr->GetNumberOfComponentsPerPixel() );
    }
  else
    {
    // pointer could not be cast back down
    itkExceptionMacro(<< "itk::ExtractImageFilter::GenerateOutputInformation "
                      << "cannot cast input to "
                      << typeid(itk::ImageBase<InputImageDimension>*).name() );
    }
}

/** 
   * ExtractImageFilter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData()
   * routine which is called for each processing thread. The output
   * image data is allocated automatically by the superclass prior to
   * calling ThreadedGenerateData().  ThreadedGenerateData can only
   * write to the portion of the output image specified by the
   * parameter "outputRegionForThread"
   *
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData() 
   */
template <class TInputImage, class TOutputImage>
void 
ExtractImageFilter<TInputImage,TOutputImage>
#if ITK_VERSION_MAJOR >= 4
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, itk::ThreadIdType threadId)
#else
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread, int threadId)
#endif
{
  itkDebugMacro(<<"Actually executing");

  // Get the input and output pointers
  typename Superclass::InputImageConstPointer  inputPtr = this->GetInput();
  typename Superclass::OutputImagePointer outputPtr = this->GetOutput();

  // support progress methods/callbacks
 itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());
  
  // Define the portion of the input to walk for this thread
  InputImageRegionType inputRegionForThread;
  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);
  
  // Define the iterators.
  typedef itk::ImageRegionIterator<TOutputImage> OutputIterator;
  typedef itk::ImageRegionConstIterator<TInputImage> InputIterator;

  OutputIterator outIt(outputPtr, outputRegionForThread);
  InputIterator inIt(inputPtr, inputRegionForThread);

  // walk the output region, and sample the input image
  while( !outIt.IsAtEnd() )
    {
    // copy the input pixel to the output
    outIt.Set( static_cast<OutputImagePixelType>(inIt.Get()));
    ++outIt; 
    ++inIt; 
    progress.CompletedPixel();
    }
}

} // end namespace clitk

#endif
