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
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkSimpleFilterWatcher.h"

#include "itkLabelImageToLabelMapFilter.h"
#include "itkChangeRegionLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"

#include "itkTestingMacros.h"

int
itkChangeRegionLabelMapFilterTest1(int argc, char * argv[])
{

  if (argc != 7)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " input output idx0 idx1 size0 size1" << std::endl;
    return EXIT_FAILURE;
  }

  constexpr unsigned int dim = 2;

  using ImageType = itk::Image<unsigned char, dim>;

  using LabelObjectType = itk::LabelObject<unsigned char, dim>;
  using LabelMapType = itk::LabelMap<LabelObjectType>;

  using ReaderType = itk::ImageFileReader<ImageType>;
  auto reader = ReaderType::New();
  reader->SetFileName(argv[1]);

  using I2LType = itk::LabelImageToLabelMapFilter<ImageType, LabelMapType>;
  auto i2l = I2LType::New();
  i2l->SetInput(reader->GetOutput());

  using ChangeType = itk::ChangeRegionLabelMapFilter<LabelMapType>;
  auto change = ChangeType::New();
  change->SetInput(i2l->GetOutput());
  ChangeType::IndexType idx;
  idx[0] = std::stoi(argv[3]);
  idx[1] = std::stoi(argv[4]);
  ChangeType::SizeType size;
  size[0] = std::stoi(argv[5]);
  size[1] = std::stoi(argv[6]);
  const ChangeType::RegionType region{ idx, size };
  change->SetRegion(region);
  const itk::SimpleFilterWatcher watcher6(change, "filter");

  using L2IType = itk::LabelMapToLabelImageFilter<LabelMapType, ImageType>;
  auto l2i = L2IType::New();
  l2i->SetInput(change->GetOutput());

  using WriterType = itk::ImageFileWriter<ImageType>;
  auto writer = WriterType::New();
  writer->SetInput(l2i->GetOutput());
  writer->SetFileName(argv[2]);
  writer->UseCompressionOn();

  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());

  change->SetInput(nullptr);
  ITK_TRY_EXPECT_EXCEPTION(change->Update());

  return EXIT_SUCCESS;
}
