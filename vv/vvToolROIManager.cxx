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

// vv
#include "vvToolROIManager.h"
#include "vvImageReader.h"
#include "vvImageWriter.h"
#include "vvROIActor.h"
#include "vvSlicer.h"
#include "vvROIActor.h"
#include "vvMeshReader.h"
#include "vvStructSelector.h"
#include "vvToolManager.h"

// clitk
#include "clitkDicomRTStruct2ImageFilter.h"
#include "clitkDicomRT_StructureSet.h"

// Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QAbstractEventDispatcher>
#include <QXmlStreamReader>
 
// vtk
#include <vtkLookupTable.h>
#include <vtkRenderWindow.h>

//------------------------------------------------------------------------------
// Create the tool and automagically (I like this word) insert it in
// the main window menu.
ADD_TOOL(vvToolROIManager);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
vvToolROIManager::vvToolROIManager(vvMainWindowBase * parent, Qt::WindowFlags f):
  QWidget(parent->GetTab()), 
  vvToolBase<vvToolROIManager>(parent),
  Ui::vvToolROIManager()
{
  // Store parent
  mMainWindow = parent;
  
  // Assume the initial tab ROI index is 2
  mIndexFirstTab = 2;

  // Build the UI
  Ui_vvToolROIManager::setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  mTree->clear();
  mTree->header()->resizeSection(0, 30);
  mGroupBoxROI->setEnabled(false);
  
  // Temporary disable "Load dicom" button
  //  frame_4->hide();

  // Set default LUT
  mDefaultLUTColor = vtkSmartPointer<vtkLookupTable>::New();
  for(int i=0; i<mDefaultLUTColor->GetNumberOfTableValues(); i++) {
    double r = (rand()/(RAND_MAX+1.0));
    double v = (rand()/(RAND_MAX+1.0));
    double b = (rand()/(RAND_MAX+1.0));
    mDefaultLUTColor->SetTableValue(i, r, v, b);
  }
#include "vvDefaultLut.h"

  // Initialization
  mCurrentSlicerManager = NULL;
  mNumberOfVisibleROI = 0;
  mNumberOfVisibleContourROI = 0;
  mOpenFileBrowserFlag = true; // by default, open the file browser when the tool is launched

  // InitializeNewTool must be called to start
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
vvToolROIManager::~vvToolROIManager()
{
  mROIActorsList.clear();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// STATIC
void vvToolROIManager::Initialize() {
  SetToolName("ROIManager");
  SetToolMenuName("Display ROI (binary image)");
  SetToolIconFilename(":/common/icons/tool-roi.png");
  SetToolTip("Display ROI from a binary image.");
  SetToolExperimental(false);
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
void  vvToolROIManager::InitializeNewTool(bool ReadStateFlag) 
{
  // Check if we need to start a new tool or read in the state file to load
  if (ReadStateFlag == false) {
    // Select the current image as the target
    int i = mMainWindow->GetSlicerManagerCurrentIndex();
    mCurrentSlicerManager = mMainWindow->GetSlicerManagers()[i];
    // Set it as current (only if not ReadStateFlag)
    mMainWindow->GetTab()->setCurrentIndex(mIndexFirstTab);
  }
  else {
    // Set the first tab in front to avoid displaying two roimanager
    // in the same tab. Because toolcreatorBase do show() and I am too
    // lazy to find another solution now.
    mMainWindow->GetTab()->setCurrentIndex(0);

    // Read all information in the XML
    ReadXMLInformation();
    
    // Check that a ROI is not already present 
    mInitialImageIndex += mImageIndex;
    if (mInitialImageIndex >= mMainWindow->GetSlicerManagers().size()) {
      QMessageBox::warning(this, "ROIManager tool", QString("Image index %1 not found, abort.").arg(mInitialImageIndex));
      close();
      return;
    }
    
    // Set the attached image
    mCurrentSlicerManager = mMainWindow->GetSlicerManagers()[mInitialImageIndex];
  }

  // Tab insertion, check that another tool does not already exist for this image
  std::vector<vvToolBaseBase*> & tools = 
    vvToolManager::GetInstance()->GetToolCreatorFromName(GetToolName())->GetListOfTool();
  if (tools.size() > 0) {
    for(uint i=0; i<tools.size()-1; i++) { // current tool is last
      vvToolROIManager * t = dynamic_cast<vvToolROIManager*>(tools[i]);
      if (mCurrentSlicerManager == t->GetCurrentSlicerManager()) {
        QMessageBox::warning(this, "ROIManager tool", "Already a ROI for this image, abort.");
        close();
        return;
      }
    }
  }

  // Display tool in the correct tab
  QWidget * tab = qFindChild<QWidget*>(mMainWindow->GetTab(), "ROItab");
  tab->layout()->addWidget(this);
 
  // If not read in a file we start automatically the browser to load
  // a roi file (binary image)
  if (ReadStateFlag) {    
    mOpenFileBrowserFlag = false;
    InputIsSelected(mCurrentSlicerManager);
    mOpenFileBrowserFlag = true;
  }
  else InputIsSelected(mCurrentSlicerManager);

  // Load ROI (if read in the XML files, empty otherwise)
  OpenBinaryImage(mROIFilenames);

  // Set the options to the open roi
  for(uint i=0; i<mROIActorsParamList.size(); i++) {
    QSharedPointer<vvROIActor> roi = mROIActorsList[i];
    QSharedPointer<vvROIActor> roi_param = mROIActorsParamList[i];
    roi->CopyParameters(roi_param);

    // Update Tree
    QTreeWidgetItem * w = mMapROIToTreeWidget[roi->GetROI()];
    QBrush brush(QColor(roi->GetROI()->GetDisplayColor()[0]*255,
                        roi->GetROI()->GetDisplayColor()[1]*255,
                        roi->GetROI()->GetDisplayColor()[2]*255));
    brush.setStyle(Qt::SolidPattern);
    w->setBackground(2, brush);
    w->setText(3, QString("%1").arg(roi->GetDepth()));  
    roi->UpdateColor();
  }

  // Display the ROI
  UpdateAllContours();
  UpdateAllROIStatus(); 

  // Connect event from mainwindow to this widget
  connect(mMainWindow, SIGNAL(AnImageIsBeingClosed(vvSlicerManager *)), 
          this, SLOT(AnImageIsBeingClosed(vvSlicerManager *)));
  connect(mMainWindow, SIGNAL(SelectedImageHasChanged(vvSlicerManager *)), 
          this, SLOT(SelectedImageHasChanged(vvSlicerManager *)));
  connect(mOpenBinaryButton, SIGNAL(clicked()), this, SLOT(OpenBinaryImage()));
  connect(mOpenDicomButton, SIGNAL(clicked()), this, SLOT(OpenDicomImage()));
  connect(mTree, SIGNAL(itemSelectionChanged()), this, SLOT(SelectedItemChangedInTree()));
  connect(mCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleROIToggled(bool)));
  connect(mOpacitySlider, SIGNAL(valueChanged(int)), this, SLOT(OpacityChanged(int)));
  connect(mChangeColorButton, SIGNAL(clicked()), this, SLOT(ChangeColor()));
  connect(mContourCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleContourROIToggled(bool)));  
  connect(mChangeContourColorButton, SIGNAL(clicked()), this, SLOT(ChangeContourColor()));
  connect(mContourWidthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeContourWidth(int)));
  connect(mDepthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeDepth(int)));
  connect(mReloadButton, SIGNAL(clicked()), this, SLOT(ReloadCurrentROI()));
  connect(mCheckBoxShowAll, SIGNAL(stateChanged(int)), this, SLOT(AllVisibleROIToggled(int)));
  connect(mContourCheckBoxShowAll, SIGNAL(toggled(bool)), this, SLOT(AllVisibleContourROIToggled(bool)));
  connect(mCloseButton, SIGNAL(clicked()), this, SLOT(close()));
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::InputIsSelected(vvSlicerManager *m)
{
  // Initialization
  mCurrentSlicerManager = m;
  mCurrentImage = mCurrentSlicerManager->GetImage();

  // Refuse if non 3D image
  if (mCurrentImage->GetNumberOfDimensions() != 3) {
    QMessageBox::information(this,tr("Sorry only 3D yet"), tr("Sorry only 3D yet"));
    close();
    return;
  }

  // Change gui
  mLabelInputInfo->setText(QString("%1").arg(m->GetFileName().c_str()));

  // Auto display browser to select new contours 
  if (mOpenFileBrowserFlag) OpenBinaryImage();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::AnImageIsBeingClosed(vvSlicerManager * m)
{
  if (m == mCurrentSlicerManager) { 
    close();
    return;
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::close()
{
  disconnect(mTree, SIGNAL(itemSelectionChanged()), this, SLOT(SelectedItemChangedInTree()));
  disconnect(mCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleROIToggled(bool)));
  disconnect(mOpacitySlider, SIGNAL(valueChanged(int)), this, SLOT(OpacityChanged(int)));
  disconnect(mChangeColorButton, SIGNAL(clicked()), this, SLOT(ChangeColor()));
  disconnect(mContourCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleContourROIToggled(bool)));  
  disconnect(mChangeContourColorButton, SIGNAL(clicked()), this, SLOT(ChangeContourColor()));
  disconnect(mContourWidthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeContourWidth(int)));
  disconnect(mDepthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeDepth(int)));

  // Remove actors
  for (unsigned int i = 0; i < mROIActorsList.size(); i++) {
    mROIActorsList[i]->RemoveActors();
  }
  mROIActorsList.clear();

  QWidget::close();
  for(int i=0; i<mCurrentSlicerManager->GetNumberOfSlicers(); i++) {
    mCurrentSlicerManager->GetSlicer(i)->Render();
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::SelectedImageHasChanged(vvSlicerManager * m) {

  if (mCurrentSlicerManager == NULL) return;
  if (m == NULL) return;
  if (m != mCurrentSlicerManager) hide(); 
  else {
    show();
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::OpenBinaryImage() 
{
  // Open images
  QString Extensions = "Images or Dicom-Struct files ( *.mha *.mhd *.hdr *.his *.dcm RS*)";
  Extensions += ";;All Files (*)";
  QStringList filename =
    QFileDialog::getOpenFileNames(this,tr("Open binary image"),
				  mMainWindowBase->GetInputPathName(),Extensions);
  if (filename.size() > 1) OpenBinaryImage(filename);
  if (filename.size() == 0) return;
  
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::OpenBinaryImage(QStringList & filename) 
{
  if (filename.size() == 0) return;
  
  // For each selected file, open the image
  for(int i=0; i<filename.size(); i++) {
    // Open Image
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    vvImageReader::Pointer reader = vvImageReader::New();
    std::vector<std::string> filenames;
    filenames.push_back(filename[i].toStdString());
    reader->SetInputFilenames(filenames);
    reader->Update(vvImageReader::IMAGE);
    QApplication::restoreOverrideCursor();

    if (reader->GetLastError().size() != 0) {
      std::cerr << "Error while reading " << filename[i].toStdString() << std::endl;
      QString error = "Cannot open file \n";
      error += reader->GetLastError().c_str();
      QMessageBox::information(this,tr("Reading problem"),error);
      return;
    }
    vvImage::Pointer binaryImage = reader->GetOutput();
    AddImage(binaryImage, filename[i].toStdString(), mBackgroundValueSpinBox->value(),
             (!mBGModeCheckBox->isChecked()));
    //    mOpenedBinaryImage.push_back(binaryImage);
    mOpenedBinaryImageFilenames.push_back(filename[i]);
    //mMapImageToIndex[binaryImage]=mOpenedBinaryImageFilenames.size()-1;
  }

  // Update the contours
  UpdateAllContours(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::OpenDicomImage() 
{
  DD("OpenDicomImage");
  QString Extensions = "Dicom Files ( *.dcm RS*)";
  Extensions += ";;All Files (*)";
  QString file = QFileDialog::getOpenFileName(this,tr("Merge Images"), 
                                              mMainWindow->GetInputPathName(), 
                                              Extensions);
  if (file.isNull()) return;

  //  AddDCStructContour(index, file);
  vvMeshReader reader;
  reader.SetFilename(file.toStdString());
  vvStructSelector selector;
  selector.SetStructures(reader.GetROINames());
  // selector.EnablePropagationCheckBox(); FIXME Disable

  // FIXME : change text -> allow to save binary image

  if (selector.exec()) {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    reader.SetSelectedItems(selector.getSelectedItems());
    reader.SetImage(mCurrentSlicerManager->GetImage());
    // reader.Update();
    
    // Read and display information
    clitk::DicomRT_StructureSet::Pointer s = clitk::DicomRT_StructureSet::New();
    s->Read(file.toStdString());
    s->Print(std::cout);

    // FIXME -> selection

    DD("here");
    clitk::DicomRTStruct2ImageFilter filter;
    filter.SetCropMaskEnabled(true); // FIXME
    filter.SetImage(mCurrentImage);  // Used FIXME -> change to set IMAGE
    filter.SetROI(s->GetROIFromROINumber(1));  // FIXME selection
    filter.SetWriteOutputFlag(false);
    filter.Update();  

    DD("debug");
    typedef itk::Image<unsigned char, 3> ImageType;
    typedef itk::VTKImageToImageFilter<ImageType> ConnectorType;
    ConnectorType::Pointer connector = ConnectorType::New();
    connector->SetInput(filter.GetOutput());
    connector->Update();
    clitk::writeImage<ImageType>(connector->GetOutput(), "toto.mha");

    // Get image
    DD("filter done");
    vvImage::Pointer binaryImage = vvImage::New();
    binaryImage->AddVtkImage(filter.GetOutput());
    
    // DEBUG  write image
    DD("write debug");
    vvImageWriter::Pointer writer = vvImageWriter::New();
    writer->SetOutputFileName("bidon.mha");
    writer->SetInput(binaryImage);
    writer->Update();
    
    // Add to gui
    DD("gui");
    AddImage(binaryImage, file.toStdString(), mBackgroundValueSpinBox->value(),
             (!mBGModeCheckBox->isChecked()));
    mOpenedBinaryImageFilenames.push_back(file);

    // std::vector<vvMesh::Pointer> contours=reader.GetOutput();
    // for (std::vector<vvMesh::Pointer>::iterator i=contours.begin();
    //      i!=contours.end(); i++)
    //   AddContour(index,*i,selector.PropagationEnabled());
    QApplication::restoreOverrideCursor();
  }



}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::AddImage(vvImage * binaryImage, std::string filename, 
                                double BG, bool modeBG) 
{
  // Check Dimension
  int dim = mCurrentImage->GetNumberOfDimensions();
  int bin_dim = binaryImage->GetNumberOfDimensions();
  if (dim < bin_dim) {
    std::ostringstream os;
    os << "Error. Loaded binary image is " << bin_dim
       << "D while selected image is " << dim << "D" << std::endl;
    QMessageBox::information(this,tr("Reading problem"),os.str().c_str());
    return;
  }
  
  // Compute roi index
  int n = mROIList.size();
  
  // Compute the name of the new ROI
  std::ostringstream oss;
  oss << vtksys::SystemTools::GetFilenameName(vtksys::SystemTools::GetFilenameWithoutLastExtension(filename));
  std::string name = oss.str();
  
  // Set color
  std::vector<double> color;
  color.push_back(1);
  color.push_back(0);
  color.push_back(0);

  // Create ROI
  clitk::DicomRT_ROI::Pointer roi = clitk::DicomRT_ROI::New();
  roi->SetFromBinaryImage(binaryImage, n, name, color, filename);

  // Add a new roi to the list
  mROIList.push_back(roi);
 
  // Set BG or FG mode
  if (modeBG) 
    roi->SetBackgroundValueLabelImage(BG);
  else 
    roi->SetForegroundValueLabelImage(BG);
  
  // Change color
  if (n<mDefaultLUTColor->GetNumberOfTableValues ()) {
    double * color = mDefaultLUTColor->GetTableValue(n % mDefaultLUTColor->GetNumberOfTableValues ());
    roi->SetDisplayColor(color[0], color[1], color[2]);
  }
  
  // Add a new roi actor
  QSharedPointer<vvROIActor> actor = QSharedPointer<vvROIActor>(new vvROIActor);
  actor->SetBGMode(modeBG);
  actor->SetROI(roi);
  actor->SetSlicerManager(mCurrentSlicerManager);
  actor->Initialize(n+1); // depth is n+1 to start at 1
  mROIActorsList.push_back(actor);
  
  // CheckBox for "All"
  if (actor->IsVisible()) mNumberOfVisibleROI++;
  if (actor->IsContourVisible()) mNumberOfVisibleContourROI++;
  
  // Add ROI in tree
  mTreeWidgetList.push_back(QSharedPointer<QTreeWidgetItem>(new QTreeWidgetItem(mTree)));
  QTreeWidgetItem * w = mTreeWidgetList.back().data();
  w->setText(0, QString("%1").arg(roi->GetROINumber()));
  w->setText(1, QString("%1").arg(roi->GetName().c_str()));
  w->setText(3, QString("%1").arg(actor->GetDepth()));  
  QBrush brush(QColor(roi->GetDisplayColor()[0]*255, 
                      roi->GetDisplayColor()[1]*255, 
                      roi->GetDisplayColor()[2]*255));
  brush.setStyle(Qt::SolidPattern);
  w->setBackground(2, brush);
  mMapROIToTreeWidget[roi] = w;
  mMapTreeWidgetToROI[w] = roi;
  mTree->resizeColumnToContents(0);
  mTree->resizeColumnToContents(1);

  // Update 
  UpdateAllROIStatus(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::UpdateAllContours() 
{
  if (mCurrentSlicerManager == NULL) return;
  // Render loaded ROIs (the first is sufficient)
  for(unsigned int i=0; i<mROIList.size(); i++) {
    mROIActorsList[i]->Update();
  }
  for(int i=0; i<mCurrentSlicerManager->GetNumberOfSlicers(); i++) {
    mCurrentSlicerManager->GetSlicer(i)->Render();
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::UpdateAllROIStatus() {
  int nbVisible = 0;
  int nb = mROIList.size();
  for(int i=0; i<nb; i++) {
    if (mROIActorsList[i]->IsVisible()) {
      nbVisible++;
    }
  }

  // change the states
  disconnect(mCheckBoxShowAll, SIGNAL(stateChanged(int)), this, SLOT(AllVisibleROIToggled(int)));  
  disconnect(mContourCheckBoxShowAll, SIGNAL(toggled(bool)), this, SLOT(AllVisibleContourROIToggled(bool)));
  if (nbVisible == nb) mCheckBoxShowAll->setCheckState(Qt::Checked);
  else {
    if (nbVisible == 0) mCheckBoxShowAll->setCheckState(Qt::Unchecked);
    else mCheckBoxShowAll->setCheckState(Qt::PartiallyChecked);
  }
  connect(mContourCheckBoxShowAll, SIGNAL(toggled(bool)), this, SLOT(AllVisibleContourROIToggled(bool)));
  connect(mCheckBoxShowAll, SIGNAL(stateChanged(int)), this, SLOT(AllVisibleROIToggled(int)));
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::SelectedItemChangedInTree() {
  // Search which roi is selected
  QList<QTreeWidgetItem *> l = mTree->selectedItems();
  if (l.size() == 0) {
    //    mCurrentROIActor = 0;
    mCurrentROI = NULL;
    mGroupBoxROI->setEnabled(false);
    return;
  }
  QTreeWidgetItem * w = l[0];
  if (w == NULL) return;
  if (w == 0) return;
  if (mMapTreeWidgetToROI.find(w) == mMapTreeWidgetToROI.end()) {
    //    mCurrentROIActor = 0;
    mCurrentROI = NULL;
    mGroupBoxROI->setEnabled(false);
    return;
  }
  if (w == NULL) return;
  clitk::DicomRT_ROI * roi = mMapTreeWidgetToROI[w];
  if (roi == NULL) return; // sometimes it is called while there is no roi anymore

  // Get selected roi actor
  int n = roi->GetROINumber();
  QSharedPointer<vvROIActor> actor = mROIActorsList[n];
  mCurrentROI = roi;
  mCurrentROIActor = actor;

  // Warning -> avoid unuseful Render here by disconnect slider 
  // Update GUI
  disconnect(mTree, SIGNAL(itemSelectionChanged()), this, SLOT(SelectedItemChangedInTree()));
  disconnect(mCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleROIToggled(bool)));
  disconnect(mOpacitySlider, SIGNAL(valueChanged(int)), this, SLOT(OpacityChanged(int)));
  disconnect(mChangeColorButton, SIGNAL(clicked()), this, SLOT(ChangeColor()));
  disconnect(mContourCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleContourROIToggled(bool)));  
  disconnect(mChangeContourColorButton, SIGNAL(clicked()), this, SLOT(ChangeContourColor()));
  disconnect(mContourWidthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeContourWidth(int)));
  disconnect(mDepthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeDepth(int)));

  mROInameLabel->setText(roi->GetName().c_str());
  mCheckBoxShow->setChecked(actor->IsVisible());
  mContourCheckBoxShow->setChecked(actor->IsContourVisible());
  mContourWidthSpinBox->setValue(actor->GetContourWidth());
  mDepthSpinBox->setValue(actor->GetDepth());
  w->setText(3, QString("%1").arg(actor->GetDepth()));
  mOpacitySlider->setValue((int)lrint(actor->GetOpacity()*100));
  mOpacitySpinBox->setValue((int)lrint(actor->GetOpacity()*100));

  connect(mTree, SIGNAL(itemSelectionChanged()), this, SLOT(SelectedItemChangedInTree()));
  connect(mCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleROIToggled(bool)));
  connect(mOpacitySlider, SIGNAL(valueChanged(int)), this, SLOT(OpacityChanged(int)));
  connect(mChangeColorButton, SIGNAL(clicked()), this, SLOT(ChangeColor()));
  connect(mContourCheckBoxShow, SIGNAL(toggled(bool)), this, SLOT(VisibleContourROIToggled(bool)));  
  connect(mChangeContourColorButton, SIGNAL(clicked()), this, SLOT(ChangeContourColor()));
  connect(mContourWidthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeContourWidth(int)));
  connect(mDepthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ChangeDepth(int)));

  
  // Set the current color to the selected ROI name
  mROInameLabel->setAutoFillBackground(true);// # This is important!!
  // mROInameLabel->setStyleSheet("QLabel { background-color : red; color : blue; }");
  QColor color = QColor(mCurrentROI->GetDisplayColor()[0]*255,
                        mCurrentROI->GetDisplayColor()[1]*255,
                        mCurrentROI->GetDisplayColor()[2]*255);
  // QString values = QString("%1, %2, %3").arg(color.red()).arg(color.green()).arg(color.blue());
  // mROInameLabel->setStyleSheet("QLabel { background-color: rgb("+values+"); }");

  QPalette* palette = new QPalette();
  QColor colorFG = QColor((1-mCurrentROI->GetDisplayColor()[0])*255,
                          (1-mCurrentROI->GetDisplayColor()[1])*255,
                          (1-mCurrentROI->GetDisplayColor()[2])*255);
  palette->setColor(QPalette::WindowText,colorFG);
  palette->setColor(QPalette::Background, color);
  mROInameLabel->setPalette(*palette);	

  // Enable the group box (in case no selection before)
  mGroupBoxROI->setEnabled(true);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::VisibleROIToggled(bool b) {
  if (mCurrentROIActor == NULL) return;
  if (b == mCurrentROIActor->IsVisible()) return; // nothing to do
  mCurrentROIActor->SetVisible(b);
  UpdateAllROIStatus();
  mCurrentSlicerManager->Render(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::VisibleContourROIToggled(bool b) {
  if (mCurrentROIActor == NULL) return;
  if (mCurrentROIActor->IsContourVisible() == b) return; // nothing to do
  mCurrentROIActor->SetContourVisible(b);
  mCurrentROIActor->UpdateColor();
  mCurrentSlicerManager->Render(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::OpacityChanged(int v) {
  if (mCurrentROIActor == NULL) return;
  mCurrentROIActor->SetOpacity((double)v/100.0);
  mCurrentROIActor->UpdateColor();
  mCurrentSlicerManager->Render(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::AllVisibleROIToggled(int b) {
  bool status = false;
  if ((mCheckBoxShowAll->checkState() == Qt::Checked) ||
      (mCheckBoxShowAll->checkState() == Qt::PartiallyChecked))  status = true;

  for(uint i=0; i<mROIList.size(); i++) {
    mROIActorsList[i]->SetVisible(status);
  }
  if (status) mCheckBoxShowAll->setCheckState(Qt::Checked);
  else  mCheckBoxShowAll->setCheckState(Qt::Unchecked);
  mCheckBoxShow->setChecked(status);
  mCurrentSlicerManager->Render(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::AllVisibleContourROIToggled(bool b) {
  bool status = false;
  if ((mContourCheckBoxShowAll->checkState() == Qt::Checked) ||
      (mContourCheckBoxShowAll->checkState() == Qt::PartiallyChecked))  status = true;
  // Update current 
  for(uint i=0; i<mROIActorsList.size(); i++) {
    mROIActorsList[i]->SetContourVisible(status);
  }
  // Update current selection
  if (status) mContourCheckBoxShowAll->setCheckState(Qt::Checked);
  else  mContourCheckBoxShowAll->setCheckState(Qt::Unchecked);
  mContourCheckBoxShow->setChecked(status);
  mCurrentSlicerManager->Render(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ChangeColor() {
  if (mCurrentROIActor == NULL) return;
  QColor color;
  color.setRgbF(mCurrentROIActor->GetROI()->GetDisplayColor()[0],
                mCurrentROIActor->GetROI()->GetDisplayColor()[1],
                mCurrentROIActor->GetROI()->GetDisplayColor()[2]);
  QColor c = QColorDialog::getColor(color, this, "Choose the ROI color");
  mCurrentROIActor->GetROI()->SetDisplayColor(c.redF(), c.greenF(), c.blueF());
  mCurrentROIActor->UpdateColor();

  QTreeWidgetItem * w = mMapROIToTreeWidget[mCurrentROI];
  QBrush brush(QColor(mCurrentROI->GetDisplayColor()[0]*255,
                      mCurrentROI->GetDisplayColor()[1]*255,
                      mCurrentROI->GetDisplayColor()[2]*255));
  brush.setStyle(Qt::SolidPattern);
  w->setBackground(2, brush);
  // Render
  mCurrentSlicerManager->Render();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ChangeContourColor() {
  if (mCurrentROIActor == NULL) return;
  QColor color;
  color.setRgbF(mCurrentROIActor->GetContourColor()[0], 
		mCurrentROIActor->GetContourColor()[1], 
		mCurrentROIActor->GetContourColor()[2]);
  //  QColorDialog d(color);
  QColor c = QColorDialog::getColor(color, this, "Choose the contour color");
  if (!c.isValid()) return; // User cancel
  mCurrentROIActor->SetContourColor(c.redF(), c.greenF(), c.blueF());
  mCurrentROIActor->UpdateColor();
  mCurrentSlicerManager->Render();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ChangeContourWidth(int n) {
  if (mCurrentROIActor == NULL) return;
    mCurrentROIActor->SetContourWidth(n);
    mCurrentROIActor->UpdateColor();
    mCurrentSlicerManager->Render();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ChangeDepth(int n) {
  if (mCurrentROIActor == NULL) return;
  mCurrentROIActor->SetDepth(n);
  // mCurrentROIActor->UpdateImage(); // FIXME  
  mCurrentSlicerManager->Render();
  QList<QTreeWidgetItem *> l = mTree->selectedItems();
  QTreeWidgetItem * w = l[0];
  w->setText(3, QString("%1").arg(mCurrentROIActor->GetDepth()));
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ReloadCurrentROI() {

  // Remove all contours/overlay first
  bool visible = mCurrentROIActor->IsVisible();
  bool cvisible = mCurrentROIActor->IsContourVisible();
  mCurrentROIActor->SetVisible(false);
  mCurrentROIActor->SetContourVisible(false);
  mCurrentSlicerManager->Render();
  
  // Reload image
  vvImageReader::Pointer reader = vvImageReader::New();
  reader->SetInputFilename(mCurrentROI->GetFilename());
  reader->Update(vvImageReader::IMAGE);
  if (reader->GetLastError() != "") {
    QMessageBox::information(mMainWindowBase, tr("Sorry, error. Could not reload"), 
                             reader->GetLastError().c_str());
    return;
  }

  // Free the previous image
  mCurrentROI->GetImage()->GetFirstVTKImageData()->ReleaseData(); // Needed to free
  mCurrentROI->GetImage()->Reset();
  mCurrentROI->SetImage(reader->GetOutput());

  mCurrentROIActor->RemoveActors();

  // Update visu
  mCurrentROIActor->UpdateImage();
  mCurrentROIActor->SetVisible(visible);
  mCurrentROIActor->SetContourVisible(cvisible);
  mCurrentSlicerManager->Render();    
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void  vvToolROIManager::SaveState(std::auto_ptr<QXmlStreamWriter> & m_XmlWriter)
{
  // Get index of the image
  int n = mMainWindow->GetSlicerManagers().size();
  int index=-1;
  for(int i=0; i<n; i++) {
    if (mCurrentSlicerManager == mMainWindow->GetSlicerManagers()[i]) index = i;
  }
  if (index == -1) {
    std::cerr << "Error while writing state for ROIManager tool no currentimage founded." << std::endl;
    return;
  }
  m_XmlWriter->writeTextElement("Image_Index", QString::number(index));


  // Write ROI
  for(uint i=0; i<mROIActorsList.size(); i++) {
    QSharedPointer<vvROIActor> roi = mROIActorsList[i];

    m_XmlWriter->writeStartElement("ROI");
    m_XmlWriter->writeTextElement("Image", mOpenedBinaryImageFilenames[i]);

    m_XmlWriter->writeStartElement("Overlay");
    m_XmlWriter->writeAttribute("Red",  QString("%1").arg(roi->GetOverlayColor()[0]));
    m_XmlWriter->writeAttribute("Green",QString("%1").arg(roi->GetOverlayColor()[1]));
    m_XmlWriter->writeAttribute("Blue", QString("%1").arg(roi->GetOverlayColor()[2]));
    m_XmlWriter->writeAttribute("Visible", QString("%1").arg(roi->IsVisible()));
    m_XmlWriter->writeAttribute("Opacity", QString("%1").arg(roi->GetOpacity()));
    m_XmlWriter->writeAttribute("Depth", QString("%1").arg(roi->GetDepth()));
    m_XmlWriter->writeEndElement();
   
    m_XmlWriter->writeStartElement("Contour");
    m_XmlWriter->writeAttribute("Red",  QString("%1").arg(roi->GetContourColor()[0]));
    m_XmlWriter->writeAttribute("Green",QString("%1").arg(roi->GetContourColor()[1]));
    m_XmlWriter->writeAttribute("Blue", QString("%1").arg(roi->GetContourColor()[2]));
    m_XmlWriter->writeAttribute("Visible", QString("%1").arg(roi->IsContourVisible()));
    m_XmlWriter->writeAttribute("Width", QString("%1").arg(roi->GetContourWidth()));
    m_XmlWriter->writeEndElement();

    m_XmlWriter->writeEndElement();
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ReadXMLInformation() 
{
  std::string value="";
  mInitialImageIndex = -1;
  while (!(m_XmlReader->isEndElement() && value == GetToolName().toStdString())) { 
    m_XmlReader->readNext();
    value = m_XmlReader->qualifiedName().toString().toStdString();
    
    if (value == "Image_Index") 
      mInitialImageIndex = m_XmlReader->readElementText().toInt();
    
    if (m_XmlReader->isStartElement()) {
      if (value == "ROI") {
        ReadXMLInformation_ROI();
      }      
    }
  }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void vvToolROIManager::ReadXMLInformation_ROI() 
{
  QString s;
  std::string value="";
  QSharedPointer<vvROIActor> param = QSharedPointer<vvROIActor>(new vvROIActor);
  param->SetVisible(true);
  clitk::DicomRT_ROI::Pointer roi = clitk::DicomRT_ROI::New();
  // r->SetDisplayColor(1,1,1);
  param->SetROI(roi);

  float r=1.0,g=1.0,b=1.0;
  float cr=1.0,cg=1.0,cb=1.0;
  float opacity = 0.7;
  bool visible = true;
  bool cvisible = true;
  int width = 1;
  int depth=1;

  while (!(m_XmlReader->isEndElement() && value == "ROI")) { 
    m_XmlReader->readNext();
    value = m_XmlReader->qualifiedName().toString().toStdString();
    if (value == "Image") {
      s = m_XmlReader->readElementText();
    }
    
    if (value == "Overlay" && m_XmlReader->isStartElement()) {
      QXmlStreamAttributes attributes = m_XmlReader->attributes();
      if (!m_XmlReader->hasError())
         r = attributes.value("Red").toString().toFloat();
      if (!m_XmlReader->hasError())
         g = attributes.value("Green").toString().toFloat();
      if (!m_XmlReader->hasError())
         b = attributes.value("Blue").toString().toFloat();
      if (!m_XmlReader->hasError())
         visible = attributes.value("Visible").toString().toInt();
      if (!m_XmlReader->hasError())
         opacity = attributes.value("Opacity").toString().toFloat();
     if (!m_XmlReader->hasError())
         depth = attributes.value("Depth").toString().toFloat();
    }


    if (value == "Contour" && m_XmlReader->isStartElement()) {
      QXmlStreamAttributes attributes = m_XmlReader->attributes();
      if (!m_XmlReader->hasError())
         cr = attributes.value("Red").toString().toFloat();
      if (!m_XmlReader->hasError())
         cg = attributes.value("Green").toString().toFloat();
      if (!m_XmlReader->hasError())
         cb = attributes.value("Blue").toString().toFloat();
      if (!m_XmlReader->hasError())
         cvisible = attributes.value("Visible").toString().toInt();
      if (!m_XmlReader->hasError())
         width = attributes.value("Width").toString().toFloat();
    }
    param->SetOverlayColor(r,g,b);
    param->SetVisible(visible);
    param->SetOpacity(opacity); 
    param->SetDepth(depth); 

    param->SetContourColor(cr,cg,cb);
    param->SetContourVisible(cvisible);
    param->SetContourWidth(width);
  }
  mROIFilenames.push_back(s);
  mROIActorsParamList.push_back(param);
}
//------------------------------------------------------------------------------
