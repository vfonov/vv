/*=========================================================================
                                                                                
Program:   clitk
Module:    $RCSfile: clitkImageInfo.cxx,v $
Language:  C++
Date:      $Date: 2010/01/06 13:31:56 $
Version:   $Revision: 1.1 $

Copyright (c) CREATIS (Centre de Recherche et d'Applications en Traitement de
l'Image). All rights reserved. See Doc/License.txt or
http://www.creatis.insa-lyon.fr/Public/Gdcm/License.html for details.
                                                                                
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.
                                                                             
=========================================================================*/

/**
   =================================================
   * @file   clitkImageInfo.cxx
   * @author David Sarrut <david.sarrut@creatis.insa-lyon.fr>
   * @date   02 Jul 2006
   =================================================*/

// clitk include
#include "clitkImageInfo_ggo.h"
#include "clitkIO.h"

//====================================================================
int main(int argc, char * argv[]) {

  // init command line
  GGO(clitkImageInfo, args_info);
  CLITK_INIT;

  // check arg
  if (args_info.inputs_num == 0) return 0;

  // read Header 
  for(unsigned int i=0; i<args_info.inputs_num; i++) {
    itk::ImageIOBase::Pointer header = clitk::readImageHeader(args_info.inputs[i]);
    if (header) {
      if (args_info.name_flag) std::cout << "[" << args_info.inputs[i] << "]\t ";
      if (args_info.long_given) {
	//	std::cout << std::endl;
	clitk::printImageHeader(header, std::cout, args_info.long_arg);
      }
      else {
	if (args_info.verbose_flag) clitk::printImageHeader(header, std::cout, 1);
	else {
	  clitk::printImageHeader(header, std::cout, 0);
	  std::cout << std::endl;
	}
      }
    }  // heade null ; non fatal error
      else {
	std::cerr << "*** Warning : I could not read '" << args_info.inputs[i] << "' ***" << std::endl;
      }
  }

  // this is the end my friend  
  return 0;
}
//====================================================================
