#include "image.hpp"
#include <iostream>

Image::Image():ptr_base(nullptr), num_size(0) {
    ptr_base = new ImageBase();
}

Image::Image(const uint32_t width, const uint32_t height, const uint8_t quality, const ColorSpace color, const ImageFormat format):Image() {
    set_resolution(width, height);
    set_quality(quality);
    set_color(color);   
    set_format(format); 
    set_timestamp(0);
} 

Image::~Image() {
    delete_image_buffer();
    delete ptr_base;
}

uint64_t Image::timestamp(){
    return ptr_base->num_timestamp;
}

void Image::set_timestamp(const uint64_t timestamp) { 
    ptr_base->num_timestamp = timestamp;
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

char* Image::image() {
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

StreamImage::StreamImage(StreamImage& copy) {
    this->set_id(copy.id()); 
    this->set_desc(copy.desc());
    this->set_resolution(copy.width(),copy.height());
    this->set_quality(copy.quality()); 
    this->set_color(copy.color()); 
    this->set_format(copy.format());
    this->set_timestamp(copy.timestamp());
    if (copy.image()!=nullptr) {
        this->set_image(copy.image(), copy.size());
    }
}

/*
StreamImage::StreamImage(const uint32_t stream_id, const std::string &stream_desc, const uint32_t image_width, const uint32_t image_height, const uint8_t image_quality) {
    set_id(stream_id); 
    set_desc(stream_desc);
    set_resolution(image_width,image_height);
    set_quality(image_quality); 
}*/

StreamImage::StreamImage(const uint32_t stream_id, const std::string &stream_desc, const uint64_t image_timestamp,const uint32_t image_width, const uint32_t image_height, const uint8_t image_quality) {
    set_id(stream_id); 
    set_desc(stream_desc);
    set_resolution(image_width,image_height);
    set_quality(image_quality); 
    set_timestamp(image_timestamp);
}

StreamImage::StreamImage(const uint32_t stream_id, const std::string &stream_desc, const uint64_t image_timestamp, const uint32_t image_width, const uint32_t image_height, const uint8_t image_quality, const ColorSpace image_color, const ImageFormat image_format) {
    set_id(stream_id); 
    set_desc(stream_desc);
    set_resolution(image_width,image_height);
    set_quality(image_quality); 
    set_color(image_color); set_format(image_format);
    set_timestamp(image_timestamp);
}

StreamImage& StreamImage::operator=(StreamImage& copy){
    if (this == &copy) return *this; 
    this->set_id(copy.id()); 
    this->set_desc(copy.desc());
    this->set_resolution(copy.width(),copy.height());
    this->set_quality(copy.quality()); 
    this->set_color(copy.color()); 
    this->set_format(copy.format());
    this->set_timestamp(copy.timestamp());
    if (copy.image()!=nullptr) {
        auto ptr_image= this->image();
        if (ptr_image != nullptr) delete ptr_image;
        this->set_image(copy.image(), copy.size());
    }
    return *this;
} 








