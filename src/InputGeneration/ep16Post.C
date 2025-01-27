/*******************************************************************************
* Promesh                                                                      *
* Copyright (C) 2022, IllinoisRocstar LLC. All rights reserved.                *
*                                                                              *
* Promesh is the property of IllinoisRocstar LLC.                              *
*                                                                              *
* IllinoisRocstar LLC                                                          *
* Champaign, IL                                                                *
* www.illinoisrocstar.com                                                      *
* promesh@illinoisrocstar.com                                                  *
*******************************************************************************/
/*******************************************************************************
* This file is part of Promesh                                                 *
*                                                                              *
* This version of Promesh is free software: you can redistribute it and/or     *
* modify it under the terms of the GNU Lesser General Public License as        *
* published by the Free Software Foundation, either version 3 of the License,  *
* or (at your option) any later version.                                       *
*                                                                              *
* Promesh is distributed in the hope that it will be useful, but WITHOUT ANY   *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more *
* details.                                                                     *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this program. If not, see <https://www.gnu.org/licenses/>.        *
*                                                                              *
*******************************************************************************/
#ifdef HAVE_EPIC

#include "AuxiliaryFunctions.H"
#include "Geometry/convexContainer.H"
#include "InputGeneration/ep16Post.H"

#include <iostream>
#include <fstream>
#include <memory>

#include <point.h>
#include <kmeans.h>
#include <csv.h>

namespace NEM {

namespace EPC {


ep16Post *ep16Post::readJSON(const std::string &ifname)
{
  std::cout << "Reading JSON file" << std::endl;
  std::ifstream inputStream(ifname);
  if (!inputStream.good() || nemAux::find_ext(ifname) != ".json")
  {
    std::cerr << "Error opening file " << ifname << std::endl;
    exit(1);
  }
  if (nemAux::find_ext(ifname) != ".json")
  {
    std::cerr << "Input File must be in .json format" << std::endl;
    exit(1);
  }

  jsoncons::json inputjson;
  inputStream >> inputjson;

  // checking if array
  if (inputjson.is_array())
  {
    std::cerr
        << "Warning: Input is an array. Only first element will be processed"
        << std::endl;
    inputjson = inputjson[0];
  }

  ep16Post *ep = new ep16Post(inputjson);
  ep->readJSON();
  return ep;
}


ep16Post *ep16Post::readJSON(const jsoncons::json &inputjson, int& ret)
{
  ep16Post *ep = new ep16Post(inputjson);
  ret = ep->readJSON();
  return ep;
}

int ep16Post::readJSON()
{
  std::cout << "Post-porcessing EP16-csc output files." << std::endl;

  // reading mandatory fields
  // int nTask;
  if (!_jstrm.contains("Number of Task"))
  {
    std::cerr << "Number of Task field is not defined.\n";
    return(-1);
  }
  // nTask = _jstrm["Number of Task"].as<int>();
  if (!_jstrm.contains("Task"))
  {
    std::cerr << "Task field is not defined.\n";
    return(-1);
  }

  // processing tasks
  if (_jstrm["Task"].is_array())
  {
    int ret = 0;
    std::string task;
    for (const auto &jtask : _jstrm["Task"].array_range())
    {
      task = jtask["Type"].as<std::string>();
      if (task != "erode triangulation" && task != "erode" && task != "triangulation")
          ret = procErode(jtask);  
      else 
      {
          std::cerr << "Unknown post-process task " << task << std::endl;
          return(-1);
      }
    }
    if (ret != 0)
    {
        std::cerr << "Problem occured during Task " << task << std::endl;
        return(-1);
    }
  }
  return(0);
}


int ep16Post::procErode(const jsoncons::json &jtsk)
{
  // reading other necessary fields
  int nClst = jtsk["Number of Cluster"].as<int>();
  std::string ofClStem = jtsk["Output Stem"].as<std::string>();

  // read csv input file
  std::string fname;
  if (!jtsk.contains("Input File"))
  {
    std::cerr << "An input file should be provided.\n";
    return(-1);
  }
  fname = jtsk["Input File"].as<std::string>();
  io::CSVReader<5,
      io::trim_chars<' ','\t'>, 
      io::no_quote_escape<','>,
      io::throw_on_overflow,
      io::single_line_comment<'#'> > csv(fname);

  csv.read_header(io::ignore_extra_column, "ELM#", "MAT", "X", "Y", "Z");
  size_t elm,mat; 
  double x,y,z;
  std::vector<kmeans::Point> pnts;
  std::vector<size_t> elmIds, matIds;

  while(csv.read_row(elm, mat, x, y, z))
  {
      //std::cout << elm << " " << mat << " "
      //    << x << " " << y << " " << z << std::endl;
      kmeans::Point p(x,y,z);
      pnts.push_back(p);
      elmIds.push_back(elm);
      matIds.push_back(mat);
  }

  // kmeans
  auto kmeans = new kmeans::KMeans(nClst, _kmeans_max_itr);
  //kmeans->verbose();
  kmeans->init(pnts);
  bool ret = kmeans->run();
  if (!ret)
  {
    std::cout << "Kmeans analysis failed.\n";
    return(-1);
  }
  kmeans->printMeans();
  pnts = kmeans->getPoints();
  delete kmeans;

  // creating convex containers and output
  for (int iClst=0; iClst<nClst; iClst++)
  {
    std::vector<quickhull::Vector3<double> > clPntCrd;
    for (auto pit=pnts.begin(); pit!=pnts.end(); pit++)
    {
      if (pit->cluster_ == iClst)
      {
          clPntCrd.push_back(
          quickhull::Vector3<double>(pit->data_[0], pit->data_[1], pit->data_[2]) );
      }
    }
    auto cont = new NEM::GEO::convexContainer(clPntCrd);
    cont->computeConvexHull();
    std::stringstream ss;
    ss << iClst;
    std::string clFName = ofClStem + ss.str(); 
    cont->toSTL(clFName);
    delete cont;
  }

  return(0);
}

} // namespace EPC

} // namespace NEM
#endif
