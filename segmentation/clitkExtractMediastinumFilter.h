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

#ifndef CLITKEXTRACTMEDIASTINUMFILTER_H
#define CLITKEXTRACTMEDIASTINUMFILTER_H

#include "clitkFilterBase.h"
#include "clitkFilterWithAnatomicalFeatureDatabaseManagement.h"

namespace clitk {
  
  //--------------------------------------------------------------------
  /*
    Try to extract the mediastinum part of a thorax CT.
    Inputs : 
    - Patient label image
    - Lungs label image
    - Bones label image
    - Trachea label image
  */
  //--------------------------------------------------------------------
  
  template <class TImageType>
  class ITK_EXPORT ExtractMediastinumFilter: 
    public virtual clitk::FilterBase, 
    public clitk::FilterWithAnatomicalFeatureDatabaseManagement,
    public itk::ImageToImageFilter<TImageType, itk::Image<uchar, 3> > 
  {

  public:
    /** Some convenient typedefs. */
    typedef TImageType                       ImageType;
    typedef typename ImageType::ConstPointer ImageConstPointer;
    typedef typename ImageType::Pointer      ImagePointer;
    typedef typename ImageType::RegionType   ImageRegionType; 
    typedef typename ImageType::PixelType    ImagePixelType; 
    typedef typename ImageType::SizeType     ImageSizeType; 
    typedef typename ImageType::IndexType    ImageIndexType; 
    typedef typename ImageType::PointType    ImagePointType; 
        
    /** Some convenient typedefs. */
    typedef uchar MaskImagePixelType; 
    typedef itk::Image<MaskImagePixelType, 3>    MaskImageType;
    typedef typename MaskImageType::ConstPointer MaskImageConstPointer;
    typedef typename MaskImageType::Pointer      MaskImagePointer;
    typedef typename MaskImageType::RegionType   MaskImageRegionType; 
    typedef typename MaskImageType::SizeType     MaskImageSizeType; 
    typedef typename MaskImageType::IndexType    MaskImageIndexType; 
    typedef typename MaskImageType::PointType    MaskImagePointType; 
        
    /** Standard class typedefs. */
    typedef itk::ImageToImageFilter<TImageType, MaskImageType> Superclass;
    typedef ExtractMediastinumFilter            Self;
    typedef itk::SmartPointer<Self>             Pointer;
    typedef itk::SmartPointer<const Self>       ConstPointer;
    
    /** Method for creation through the object factory. */
    itkNewMacro(Self);
    
    /** Run-time type information (and related methods). */
    itkTypeMacro(ExtractMediastinumFilter, InPlaceImageFilter);
    FILTERBASE_INIT;

    /** Connect inputs */
    void SetInput(const ImageType * image);
    void SetInputPatientLabelImage(const MaskImageType * image, MaskImagePixelType bg=0);
    void SetInputLungLabelImage(const MaskImageType * image, MaskImagePixelType bg=0, 
                                MaskImagePixelType fgLeftLung=1, MaskImagePixelType fgRightLung=2);
    void SetInputBonesLabelImage(const MaskImageType * image, MaskImagePixelType bg=0);
    void SetInputTracheaLabelImage(const MaskImageType * image, MaskImagePixelType bg=0);
   
   // Output filename  (for AFBD)
    itkSetMacro(OutputMediastinumFilename, std::string);
    itkGetConstMacro(OutputMediastinumFilename, std::string);

    /** ImageDimension constants */
    itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension);

    // Background / Foreground
    itkSetMacro(BackgroundValuePatient, MaskImagePixelType);
    itkGetConstMacro(BackgroundValuePatient, MaskImagePixelType);
    
    itkSetMacro(BackgroundValueLung, MaskImagePixelType);
    itkGetConstMacro(BackgroundValueLung, MaskImagePixelType);
    
    itkSetMacro(BackgroundValueBones, MaskImagePixelType);
    itkGetConstMacro(BackgroundValueBones, MaskImagePixelType);
    
    itkGetConstMacro(BackgroundValue, MaskImagePixelType);
    itkGetConstMacro(ForegroundValue, MaskImagePixelType);

    itkSetMacro(ForegroundValueLeftLung, MaskImagePixelType);
    itkGetConstMacro(ForegroundValueLeftLung, MaskImagePixelType);
    
    itkSetMacro(ForegroundValueRightLung, MaskImagePixelType);
    itkGetConstMacro(ForegroundValueRightLung, MaskImagePixelType);
    
    itkSetMacro(BackgroundValueTrachea, MaskImagePixelType);
    itkGetConstMacro(BackgroundValueTrachea, MaskImagePixelType);
    
    itkSetMacro(IntermediateSpacing, double);
    itkGetConstMacro(IntermediateSpacing, double);

    itkSetMacro(FuzzyThreshold1, double);
    itkGetConstMacro(FuzzyThreshold1, double);

    itkSetMacro(FuzzyThreshold2, double);
    itkGetConstMacro(FuzzyThreshold2, double);

    itkSetMacro(FuzzyThreshold3, double);
    itkGetConstMacro(FuzzyThreshold3, double);

    itkBooleanMacro(UseBones);
    itkSetMacro(UseBones, bool);
    itkGetConstMacro(UseBones, bool);

    itkSetMacro(UpperThreshold, double);
    itkGetConstMacro(UpperThreshold, double);

    itkSetMacro(LowerThreshold, double);
    itkGetConstMacro(LowerThreshold, double);

  protected:
    ExtractMediastinumFilter();
    virtual ~ExtractMediastinumFilter() {}
    
    virtual void GenerateOutputInformation();
    virtual void GenerateInputRequestedRegion();
    virtual void GenerateData();
       
    itkSetMacro(BackgroundValue, MaskImagePixelType);
    itkSetMacro(ForegroundValue, MaskImagePixelType);
    
    MaskImagePixelType m_BackgroundValuePatient;
    MaskImagePixelType m_BackgroundValueLung;
    MaskImagePixelType m_BackgroundValueBones;
    MaskImagePixelType m_BackgroundValueTrachea;
    MaskImagePixelType m_ForegroundValueLeftLung;
    MaskImagePixelType m_ForegroundValueRightLung;

    MaskImagePixelType m_BackgroundValue;
    MaskImagePixelType m_ForegroundValue;

    typename MaskImageType::Pointer output;

    double m_IntermediateSpacing;
    double m_FuzzyThreshold1;
    double m_FuzzyThreshold2;
    double m_FuzzyThreshold3;
    bool   m_UseBones;
    double m_UpperThreshold;
    double m_LowerThreshold;
    
    std::string m_OutputMediastinumFilename;
    
  private:
    ExtractMediastinumFilter(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented
    
  }; // end class
  //--------------------------------------------------------------------

} // end namespace clitk
//--------------------------------------------------------------------

#ifndef ITK_MANUAL_INSTANTIATION
#include "clitkExtractMediastinumFilter.txx"
#endif

#endif
