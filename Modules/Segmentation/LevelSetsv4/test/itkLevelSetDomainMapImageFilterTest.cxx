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

#include "itkLevelSetDomainMapImageFilter.h"
#include "itkTestingMacros.h"

int
itkLevelSetDomainMapImageFilterTest(int, char *[])
{
  constexpr unsigned int Dimension = 2;

  using ListPixelType = std::list<int>;

  using InputImageType = itk::Image<ListPixelType, Dimension>;
  using OutputImageType = itk::Image<unsigned short, Dimension>;

  using DomainMapImageFilterType = itk::LevelSetDomainMapImageFilter<InputImageType, OutputImageType>;
  using DomainMapType = DomainMapImageFilterType::DomainMapType;

  InputImageType::IndexType index;
  index[0] = 0;
  index[1] = 0;

  InputImageType::SizeType size;
  size[0] = 10;
  size[1] = 10;

  const InputImageType::RegionType region{ index, size };

  const ListPixelType l;

  auto input = InputImageType::New();
  input->SetRegions(region);
  input->Allocate();
  input->FillBuffer(l);

  for (unsigned int i = 0; i < 10; ++i)
  {
    ListPixelType ll;
    ll.push_back(i);
    ll.push_back(i + 1);

    index[0] = index[1] = i;
    input->SetPixel(index, ll);
  }

  auto filter = DomainMapImageFilterType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(filter, LevelSetDomainMapImageFilter, ImageToImageFilter);


  filter->SetInput(input);
  filter->Update();

  const OutputImageType::Pointer output = filter->GetOutput();

  using OutputImageIteratorType = itk::ImageRegionConstIteratorWithIndex<OutputImageType>;
  OutputImageIteratorType it(output, output->GetLargestPossibleRegion());

  it.GoToBegin();

  const DomainMapType domainMap = filter->GetDomainMap();
  const auto          mapEnd = domainMap.end();
  while (!it.IsAtEnd())
  {
    const OutputImageType::IndexType out_index = it.GetIndex();
    const OutputImageType::PixelType out_id = it.Get();

    if (out_id > 0)
    {
      std::cout << "*** " << std::endl;
      std::cout << out_index << " # " << out_id << std::endl;
      auto mapIt = domainMap.find(out_id);
      if (mapIt != mapEnd)
      {
        const InputImageType::RegionType domainMapRegion = *(mapIt->second.GetRegion());
        std::cout << domainMapRegion;

        const ListPixelType * lout = mapIt->second.GetIdList();
        if (lout->empty())
        {
          return EXIT_FAILURE;
        }

        for (const auto & lIt : *lout)
        {
          std::cout << lIt << ' ';
        }
        std::cout << std::endl;
      }
    }
    ++it;
  }

  return EXIT_SUCCESS;
}
