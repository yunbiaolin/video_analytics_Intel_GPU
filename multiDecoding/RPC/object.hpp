/*****************************************************************************
*  Intel DSS Vidoe Analytic POC                                              *
*  Copyright (C) 2019 Henry.Wen  lei.xia@intel.com.                          *
*                                                                            *
*  This file is part of the POC.                                             *
*                                                                            *
*  This program is free software under Intel NDA;                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*  @file     object.hpp                                                      *
*  @brief    head file for object classes                                    *
*  @author   Lei.Xia                                                         *
*  @email    lei.xia@intel.com                                               *
*  @version  0.0.0.1                                                         *
*  @date     Mar 25, 2019                                                    *
*  @license  Intel NDA                                                       *
*                                                                            *
*----------------------------------------------------------------------------*
*  Remark         : Define the classes for image operation from decoded video*
*                   streams 
*----------------------------------------------------------------------------*
*  Change History :                                                          *
*  <Date>     | <Version> | <Author>       | <Description>                   *
*----------------------------------------------------------------------------*
*  2019/03/25 | 0.0.0.1   | Lei.Xia        | Create file                     *
*----------------------------------------------------------------------------*
*                                                                            *
*****************************************************************************/

#ifndef __DSS_POC_OBJECT_HPP__
#define __DSS_POC_OBJECT_HPP__

#include <cstdint>
#include <string>
#include <cstring>
#include "image.hpp"
/** 
  * @brief BBox class
  * Defines the bonding box around a object. 
  */
class BBox{
  public:
    int top=-1;
    int bottom=-1;
    int left=-1;
    int right=-1;
}; 

typedef BBox* BBox_ptr_t;

/** 
  * @brief Object class
  * Defines interfaces of the object. 
  */
class Object{
    private: 
      BBox_ptr_t ptr_bbox;
      StreamImage_ptr_t ptr_image;
    public:
      /** 
        * @brief The default constructor
        */
      Object(); 
      
      Object(Object&); 
      /** 
        * @brief Constructor
        * @param bbox         The bonding box of the image
        * @param image        an image object
        */
      Object(const BBox &bbox, StreamImage& image);
      /** 
        * @brief Deconstructor
        */
      ~Object(); 
      /** 
        * @brief Get the bonding box information
        * @return   the bonding box information
        */
      BBox_ptr_t get_bbox();
      /** 
        * @brief Get the object image 
        * @return   the object image
        */
      StreamImage_ptr_t get_image();
      Object& operator=(const Object);
      Object& operator=(const Object&);
};

typedef Object* Object_ptr_t;
#endif