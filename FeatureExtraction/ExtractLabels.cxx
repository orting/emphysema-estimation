
#include <unordered_map>
#include <iostream>


#include "tclap/CmdLine.h"

#include "itkImageFileReader.h"
#include "itkImageRegionConstIterator.h"

#include "ROIReader.h"

const std::string VERSION("0.1");

int main( int argc, char* argv[] ) {
    // Commandline parsing
  TCLAP::CmdLine cmd("Extract the mode from an image inside regions of interest.", ' ', VERSION);

  //
  // Required arguments
  //
  
  // We need a single image
  TCLAP::ValueArg<std::string> 
    imageArg("i", 
	     "image", 
	     "Path to image.",
	     true,
	     "",
	     "path", 
	     cmd);

  // We need a single mask
  TCLAP::ValueArg<std::string> 
    roiArg("r", 
	   "roi", 
	   "Path to roi.",
	   true,
	   "",
	   "path", 
	   cmd);

  
  // We need a directory for storing the ROIs
  TCLAP::ValueArg<std::string> 
    outArg("o", 
	   "out", 
	   "Output path",
	   true, 
	   "", 
	   "path", 
	   cmd);

  try {
    cmd.parse(argc, argv);
  } catch(TCLAP::ArgException &e) {
    std::cerr << "Error : " << e.error() 
	      << " for arg " << e.argId() 
	      << std::endl;
    return EXIT_FAILURE;
  }

  // Store the arguments
  const std::string imagePath( imageArg.getValue() );
  const std::string roiPath( roiArg.getValue() );
  const std::string outPath( outArg.getValue() );
  //// Commandline parsing is done ////
  
  // It is assumed that we use unsigned char and we have 3D images
  typedef char PixelType;
  const unsigned int Dimension = 3;
  typedef itk::Image< PixelType, Dimension > ImageType;

  // Setup the ROI reader
  typedef typename ImageType::RegionType RegionType;
  typedef ROIReader< RegionType > ROIReaderType;
  
  // Setup the image reader
  typedef itk::ImageFileReader< ImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( imagePath );
  try {
    reader->Update();
  }
  catch ( itk::ExceptionObject &e ) {
    std::cerr << "Failed to update reader" << std::endl       
	      << "imagePath: " << imagePath << std::endl
	      << "ExceptionObject: " << e << std::endl;
    return EXIT_FAILURE;
  }

  // Setup the image iterator
  typedef itk::ImageRegionConstIterator< ImageType > IteratorType;  
  
  // Read the roi specification
  std::vector< RegionType > rois;
  try {
    rois = ROIReaderType::read( roiPath );
    std::cout << "Got " << rois.size() << " rois." << std::endl;
  }
  catch ( std::exception &e ) {
    std::cerr << "Error reading ROIs" << std::endl
	      << "roiPath: " << roiPath << std::endl
	      << "exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  // Allocate space for the mode of each roi
  std::ofstream out( outPath );

  // Setup the map
  typedef std::unordered_map< PixelType, size_t > MapType;
  typedef typename MapType::value_type MapValueType;

  
  // Iterate through the ROIs and find the mode
  for ( size_t i = 0; i < rois.size(); ++i ) {
    if ( !out.good() ) {
      std::cerr << "Error writing to " << outPath << std::endl;
      return EXIT_FAILURE;
    }   

    // Get the counts for each pixel value
    MapType counts;
    try {
      IteratorType it( reader->GetOutput(), rois[i] );
      for ( it.GoToBegin(); !it.IsAtEnd(); ++it ) {
	// If it.Get() is not already in counts, then it is inserted and a
	// reference is returned.
	++counts[it.Get()]; 
      }
    }
    catch ( itk::ExceptionObject &e ) {
      std::cerr << "Failed to process" << std::endl       
		<< "ROI: " << rois[i] << std::endl
		<< "Image->LargestPossibleRegion(): "
		<< reader->GetOutput()->GetLargestPossibleRegion() << std::endl
		<< "ExceptionObject: " << e << std::endl;
      return EXIT_FAILURE;
    }

    // Find the mode of the counts
    auto mode =
      std::max_element( counts.begin(),
			counts.end(),
			[](const MapValueType& a,
			   const MapValueType& b) {
			  return a.second < b.second;
			} );

    // Output the pixel value of the mode
    out << std::to_string(mode->first) << std::endl; // flush so we can see what happens
  }

  return EXIT_SUCCESS;
}
