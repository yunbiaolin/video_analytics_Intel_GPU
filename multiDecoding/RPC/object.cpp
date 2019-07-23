#include "object.hpp"
#include <iostream>
#include "image.hpp"

Object::Object(){
    ptr_bbox = new BBox;
    ptr_image = nullptr; 
}

Object::Object(Object& o){
    ptr_bbox = new BBox;
    *ptr_bbox = *o.ptr_bbox;
    ptr_image = new StreamImage(*o.ptr_image); 
}

Object::Object(const BBox &bbox, StreamImage& image){
    ptr_bbox = new BBox;
    *ptr_bbox = bbox;
    ptr_image = new StreamImage(image);
}

Object::~Object(){
    if (ptr_bbox !=nullptr) delete ptr_bbox;
    if (ptr_image !=nullptr) delete ptr_image;    
}

BBox_ptr_t Object::get_bbox(){
    return ptr_bbox;
}

StreamImage_ptr_t Object::get_image(){
    return ptr_image;
}

Object& Object::operator=(const Object o){
    if (this == &o) return *this; 
    *ptr_bbox = *o.ptr_bbox;
    if (o.ptr_image!=nullptr) {
        StreamImage new_image = *o.ptr_image;
//        StreamImage new_image = new StreamImage(copy_image);
        if (this->ptr_image != nullptr) delete this->ptr_image;
        this->ptr_image = &new_image; 
    }
    return *this;
}

Object& Object::operator=(const Object& o){
    if (this == &o) return *this; 
    *ptr_bbox = *o.ptr_bbox;
    if (o.ptr_image!=nullptr) {
        StreamImage new_image = *o.ptr_image;
//        StreamImage new_image = new StreamImage(copy_image);
        if (this->ptr_image != nullptr) delete this->ptr_image;
        this->ptr_image = &new_image; 
    }
    return *this;
}