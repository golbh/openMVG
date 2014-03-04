
// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef DOCUMENT
#define DOCUMENT

#include "openMVG/cameras/PinholeCamera.hpp"
#include "openMVG/tracks/tracks.hpp"
using namespace openMVG;

#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>

struct Document
{
  std::vector<float> _vec_points;
  std::map<size_t, std::vector<size_t> > _map_visibility; //Inth camera see the Inth 3D point
  tracks::STLMAPTracks _tracks;

  std::map<size_t, PinholeCamera > _map_camera;
  std::vector<std::string> _vec_imageNames;
  std::map<size_t, std::pair<size_t,size_t> > _map_imageSize;

  std::string _sDirectory;


  static bool readCamera(const std::string & sCam, PinholeCamera & cam)
  {
    std::vector<double> val;

    if (stlplus::extension_part(sCam) == "bin")
    {
      std::ifstream in(sCam.c_str(),
        std::ios::in|std::ios::binary);
      if (!in.is_open())	{
        std::cerr << "Error: failed to open file '" << sCam << "' for reading" << std::endl;
        return false;
      }
      val.resize(12);
      in.read((char*)&val[0],(std::streamsize)12*sizeof(double));
      if (in.fail())  {
        val.clear();
      }
    }
    else
      return false;

    if (val.size() == 3*4) //P Matrix
    {
      Mat34 P;
      P << val[0], val[3], val[6], val[9],
        val[1], val[4], val[7], val[10],
        val[2], val[5], val[8], val[11];

      Mat3 R,K;
      Vec3 t;
      KRt_From_P(P, &K, &R, &t);
      cam = PinholeCamera(K, R, t);
      return true;
    }
    return false;
  }


  bool load(const std::string & spath)
  {
    //-- Check if the required file are present.
    _sDirectory = spath;
    std::string sDirectoryPly = stlplus::folder_append_separator(_sDirectory) + "clouds";
    if (stlplus::is_file(stlplus::create_filespec(sDirectoryPly,"visibility","txt"))
      && stlplus::is_file(stlplus::create_filespec(_sDirectory,"views","txt")))
    {
      // Read visibility file (3Dpoint, NbVisbility, [(imageId, featId); ... )
      std::ifstream iFilein(stlplus::create_filespec(sDirectoryPly,"visibility","txt").c_str());
      if (iFilein.is_open())
      {
        size_t trackId = 0;
        while (!iFilein.eof())
        {
          // read one line at a time
          std::string temp;
          std::getline(iFilein, temp);
          std::stringstream sStream(temp);
          float pt[3];
          sStream >> pt[0] >> pt[1] >> pt[2];
          int count;
          sStream >> count;
          size_t imaId, featId;
          for (int i = 0; i < count; ++i)
          {
            sStream >> imaId >> featId;
            _tracks[trackId].insert(std::make_pair(imaId, featId));
            _map_visibility[imaId].push_back(trackId); //imaId camera see the point indexed trackId
          }

          _vec_points.push_back(pt[0]);
          _vec_points.push_back(pt[1]);
          _vec_points.push_back(pt[2]);
          trackId++;
        }
      }
      else
      {
        std::cerr << "Cannot open the visibility file" << std::endl;
      }
    }
    else
    {
      std::cerr << "Required file(s) is missing" << std::endl;
    }

    // Read cameras
    std::string sDirectoryCam = stlplus::folder_append_separator(_sDirectory) + "cameras";
  
    size_t camIndex = 0;
    //Read views file
    {
      std::ifstream iFilein(stlplus::create_filespec(_sDirectory,"views","txt").c_str());
      if (iFilein.is_open())
      {
        std::string temp;
        getline(iFilein,temp); //directory name
        getline(iFilein,temp); //directory name
        size_t nbImages;
        iFilein>> nbImages;
        while(iFilein.good())
        {
          getline(iFilein,temp);
          if (!temp.empty())
          {
            std::stringstream sStream(temp);
            std::string sImageName, sCamName;
            size_t w,h;
            float znear, zfar;
            sStream >> sImageName >> w >> h >> sCamName >> znear >> zfar;
            // Read the corresponding camera
            PinholeCamera cam;
            if (!readCamera(stlplus::folder_append_separator(sDirectoryCam) + sCamName, cam))
              std::cerr << "Cannot read camera" << std::endl;
            _map_camera[camIndex] = cam;

            _vec_imageNames.push_back(sImageName);
            _map_imageSize[camIndex] = std::make_pair(w,h);
            camIndex++;
          }
          temp.clear();
        }
      }
      std::cout << "\n Loaded image names : " << std::endl;
      std::copy(_vec_imageNames.begin(), _vec_imageNames.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
    }
    return !_map_camera.empty();
  }
};

#endif //DOCUMENT
