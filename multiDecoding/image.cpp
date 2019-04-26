#include "image.hpp"
#include <iostream>

Image::Image():ptr_base(nullptr), num_size(0) {
    ptr_base = new ImageBase();
}

Image::Image(const uint32_t width, const uint32_t height, const uint8_t quality, const ColorSpace color, const ImageFormat format):Image() {
    set_resolution(width, height);
    set_quality(quality);
    set_color(color);   set_format(format); 
} 

Image::~Image() {
    delete_image_buffer();
    delete ptr_base;
}

void Image::set_resolution(const uint32_t width, const uint32_t height) { 
    ptr_base->num_width = width;
    ptr_base->num_height = height; 
} 

void Image::set_quality(const uint8_t quality) {
    if (quality<0) 
        ptr_base->num_quality=1;
    else
        ptr_base->num_quality = quality;
}

void Image::set_color(const ColorSpace color) { 
    ptr_base->enum_color = color; 
} 

void Image::set_format(const ImageFormat format) { 
    ptr_base->enum_format = format; 
}

void Image::set_image(const void* image, size_t size) { 
    delete_image_buffer(); // release old buffer
    ptr_base->ptr_image =(char*) std::malloc(size);
    // copy the image to local 
    std::memcpy(ptr_base->ptr_image, image, size);  
    num_size=size;
}

void Image::delete_image_buffer(){
    if (nullptr != ptr_base->ptr_image) {
        std::free(ptr_base->ptr_image); // release buffer
    }
}

uint32_t Image::height() { 
    return ptr_base->num_height; 
} 

uint32_t Image::width() {
    return ptr_base->num_width; 
}

uint8_t Image::quality() {
    return ptr_base->num_quality; 
}

ColorSpace Image::color() {
    return ptr_base->enum_color; 
}

ImageFormat Image::format() { 
    return ptr_base->enum_format; 
}

const void* Image::image() {
    return ptr_base->ptr_image; 
}



size_t Image::size(){
    return num_size;
}

Stream::Stream() {
    ptr_base = new StreamBase();
}

Stream::Stream(const uint32_t id):Stream(){
    set_id(id);
}

Stream::Stream(const uint32_t id, const std::string & desc):Stream(id){
    set_desc(desc);
}

Stream::~Stream(){
    delete ptr_base;
}

void Stream::set_id(const uint32_t id) {
    ptr_base->num_id = id; 
}

void Stream::set_desc(const std::string & desc) {
    ptr_base->str_desc = desc; 
}

uint32_t Stream::id() { 
    return ptr_base->num_id; 
}

std::string Stream::desc() { 
    return ptr_base->str_desc; 
}

StreamImage::StreamImage(const uint32_t id, const std::string &desc, const uint32_t width, const uint32_t height, const uint8_t quality) {
    set_id(id); 
    set_desc(desc);
    set_resolution(width,height);
    set_quality(quality); 
}

StreamImage::StreamImage(const uint32_t id, const std::string &desc, const uint32_t width, const uint32_t height, const uint8_t quality, const ColorSpace color, const ImageFormat format) {
    set_id(id); 
    set_desc(desc);
    set_resolution(width,height);
    set_quality(quality); 
    set_color(color); set_format(format);
}








