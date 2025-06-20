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

#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkTimeProbesCollectorBase.h"
#include "itkTIFFImageIO.h"
#include "itkTestingMacros.h"

// Specific ImageIO test

namespace
{

template <typename TImage>
int
itkLargeTIFFImageWriteReadTestHelper(std::string filename, typename TImage::SizeType size)
{
  using ImageType = TImage;
  using PixelType = typename ImageType::PixelType;

  using WriterType = itk::ImageFileWriter<ImageType>;
  using ReaderType = itk::ImageFileReader<ImageType>;

  using IteratorType = itk::ImageRegionIterator<ImageType>;
  using ConstIteratorType = itk::ImageRegionConstIterator<ImageType>;

  using SizeValueType = itk::SizeValueType;

  const typename ImageType::IndexType  index{};
  const typename ImageType::RegionType region{ index, size };
  {
    // Write block
    auto image = ImageType::New();

    image->SetRegions(region);

    SizeValueType numberOfPixels = 1;
    for (unsigned int i = 0; i < ImageType::ImageDimension; ++i)
    {
      numberOfPixels *= static_cast<SizeValueType>(region.GetSize(i));
    }

    constexpr SizeValueType oneMebiByte = static_cast<SizeValueType>(1024) * static_cast<SizeValueType>(1024);

    const SizeValueType sizeInBytes = sizeof(PixelType) * numberOfPixels;

    const SizeValueType sizeInMebiBytes = sizeInBytes / oneMebiByte;


    std::cout << "Trying to allocate an image of size " << sizeInMebiBytes << " MiB " << std::endl;
    itk::TimeProbesCollectorBase chronometer;
    chronometer.Start("Allocate");
    image->Allocate();
    chronometer.Stop("Allocate");

    std::cout << "Initializing pixel values" << std::endl;

    IteratorType itr(image, region);
    itr.GoToBegin();

    PixelType pixelValue{};

    chronometer.Start("Initializing");
    while (!itr.IsAtEnd())
    {
      itr.Set(pixelValue);
      ++pixelValue;
      ++itr;
    }
    chronometer.Stop("Initializing");

    std::cout << "Trying to write the image to disk" << std::endl;

    auto writer = WriterType::New();
    writer->SetInput(image);
    writer->SetFileName(filename);

    chronometer.Start("Write");

    ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());

    chronometer.Stop("Write");

    image = nullptr;
  } // end write block to free the memory

  std::cout << "Trying to read the image back from disk" << std::endl;
  auto reader = ReaderType::New();
  reader->SetFileName(filename);

  const itk::TIFFImageIO::Pointer io = itk::TIFFImageIO::New();
  reader->SetImageIO(io);

  itk::TimeProbesCollectorBase chronometer;
  chronometer.Start("Read");
  ITK_TRY_EXPECT_NO_EXCEPTION(reader->Update());
  chronometer.Stop("Read");

  const typename ImageType::ConstPointer readImage = reader->GetOutput();

  ConstIteratorType ritr(readImage, region);

  ritr.GoToBegin();

  std::cout << "Comparing the pixel values..." << std::endl;

  PixelType pixelValue{};

  chronometer.Start("Compare");
  while (!ritr.IsAtEnd())
  {
    if (ritr.Get() != pixelValue)
    {
      std::cerr << "Test failed!" << std::endl;
      std::cerr << "Error while comparing pixel value at index: " << ritr.GetIndex() << std::endl;
      std::cerr << "Expected: " << pixelValue << ", but got: " << ritr.Get() << std::endl;
      return EXIT_FAILURE;
    }

    ++pixelValue;
    ++ritr;
  }
  chronometer.Stop("Compare");

  chronometer.Report(std::cout);

  std::cout << std::endl;
  std::cout << "Test PASSED !" << std::endl;

  return EXIT_SUCCESS;
}
} // namespace

int
itkLargeTIFFImageWriteReadTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cout << "Usage: " << itkNameOfTestExecutableMacro(argv) << " outputFileName"
              << " numberOfPixelsInOneDimension"
              << " [numberOfZslices]" << std::endl;
    return EXIT_FAILURE;
  }

  const std::string filename = argv[1];

  if (argc == 3)
  {
    constexpr unsigned int Dimension = 2;

    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, Dimension>;

    auto size = ImageType::SizeType::Filled(atol(argv[2]));

    return itkLargeTIFFImageWriteReadTestHelper<ImageType>(filename, size);
  }

  constexpr unsigned int Dimension = 3;

  using PixelType = unsigned short;
  using ImageType = itk::Image<PixelType, Dimension>;

  auto size = ImageType::SizeType::Filled(atol(argv[2]));
  size[2] = atol(argv[3]);

  return itkLargeTIFFImageWriteReadTestHelper<ImageType>(filename, size);
}
