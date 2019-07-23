#include <iterator> 
#include <remote_interface.hpp>
#include <fstream>
#include <utility.hpp>

using namespace remote;

std::mutex detector_buffer_mutex;
std::mutex detector_piece_mutex;
std::mutex detector_count_mutex;
std::mutex detector_queue_mutex;

std::mutex classifer_buffer_mutex;
std::mutex classifer_piece_mutex;
std::mutex classifer_count_mutex;
std::mutex classifer_queue_mutex;

std::mutex matching_count_mutex;
std::mutex matching_queue_mutex;

unsigned DetectorI::num_count = 0;
PieceBuffer* DetectorI::ptr_piece_buffer=nullptr;
ImageQueue* DetectorI::ptr_image_queue=nullptr;

PieceBuffer* ClassifierI::ptr_piece_buffer=nullptr;
ObjectQueue* ClassifierI::ptr_object_queue=nullptr;
unsigned ClassifierI::num_count=0;

FeatureQueue MatchingI::feature_queue(SERVER);
unsigned MatchingI::num_count=0;

void remote::DetectorI::detect(Stream_Id stream_id,
                           Timestamp image_timestamp,
                           int image_width,
                           int image_height,
                           ::Ice::Byte image_quality,
                           ::Ice::Byte image_color,
                           ::Ice::Byte image_format,
                           short total_pieces,
                           int image_size,
                           Piece_Id curr_piece_id,
                           Bytes image_piece,
                           int piece_size, 
                           const Ice::Current&) {
    ImageKey image_key=std::make_pair(stream_id, image_timestamp);

    detector_buffer_mutex.lock();
    auto it = ptr_piece_buffer->find(image_key);
    detector_buffer_mutex.unlock();

    if (it!=ptr_piece_buffer->end()) {
        // key found

        if (total_pieces-1==it->second->pieces.size() && curr_piece_id==total_pieces-1) {
            // this is the last piece
            
            StreamImage image(stream_id,"", image_timestamp, image_width, image_height, image_quality, 
                static_cast<ColorSpace>(image_color), static_cast<ImageFormat>(image_format)); 
            
            char* ptr_image_buffer = (char*) std::malloc(image_size);
            char* ptr_image_buffer_copy=ptr_image_buffer;

            detector_piece_mutex.lock();

            //int temp_size=0;
            for (int i=0; i<total_pieces-1; i++){
                for (auto& piece:it->second->pieces) {
                    if (piece->id==i) {
                        /*
                        std::cout<<"("<<stream_id<<","<<image_timestamp<<") ";
                        std::cout<<"piece: "<<i+1<<" of "<<total_pieces<<" recieved, block size = "<<piece->piece_size;
                        temp_size+=piece->piece_size;
                        std::cout<<" ("<<temp_size<<" of "<<image_size<<")"<<std::endl; */

                        std::memcpy(ptr_image_buffer_copy, piece->ptr_data,piece->piece_size);
                        ptr_image_buffer_copy+=piece->piece_size;
                        std::free(piece->ptr_data); //free the piece data
                        delete piece; // delete the piece structure
                        break;
                    }
                }
            }

            // delete the pieces
            it->second->pieces.clear();
            delete it->second; // delete the PiecedImage stucture 
            detector_piece_mutex.unlock();

            detector_buffer_mutex.lock();
            ptr_piece_buffer->erase(image_key); // delete the map item
            detector_buffer_mutex.unlock();

            // copy the current piece as the last

            /*
            std::cout<<"("<<stream_id<<","<<image_timestamp<<") ";
            std::cout<<"last piece: "<<total_pieces<<" of "<<total_pieces<<" recieved, size = "<<piece_size;
            temp_size+=piece_size;
            std::cout<<" ("<<temp_size<<" of "<<image_size<<")"<<std::endl;
            std::cout<<"--------------------------------------------------------------"<<std::endl;*/

            std::memcpy(ptr_image_buffer_copy, reinterpret_cast<const char*>(image_piece.first), piece_size);
            image.set_image(ptr_image_buffer,image_size);

            detector_queue_mutex.lock();
            ptr_image_queue->push(image);
            detector_queue_mutex.unlock();
            
            assert((ptr_image_buffer_copy+piece_size)==(ptr_image_buffer+image_size));

            /*
            using Clock = std::chrono::high_resolution_clock;
            constexpr auto num = Clock::period::num;
            constexpr auto den = Clock::period::den;
            std::cout << Clock::now().time_since_epoch().count() << " [" << num << '/' << den << "] units since epoch\n";
            std::cout << "image queue size: "<<ptr_image_queue->size()<<std::endl; */

            std::free(ptr_image_buffer);
        } else {
            // piece of existing image
            bool b_duplicated=false;
            for (auto& piece:it->second->pieces) {
                    if (curr_piece_id == piece->id) 
                    {
                        b_duplicated=true;
                        /*
                        std::cout<<"("<<stream_id<<","<<image_timestamp<<") ";
                        std::cout<<"duplicated piece detected: "<<curr_piece_id<<std::endl; */
                        break;
                    }
            }
            if (!b_duplicated) {
                char* ptr_data = (char*) std::malloc(piece_size);
                std::memcpy(ptr_data,reinterpret_cast<const char*>(image_piece.first), piece_size);
                Piece* ptr_piece = new Piece;
                ptr_piece->id=curr_piece_id;
                ptr_piece->piece_size=piece_size;
                ptr_piece->ptr_data=ptr_data;

                detector_piece_mutex.lock();
                it->second->pieces.push_back(ptr_piece);
                detector_piece_mutex.unlock();
            }
        }
    } else {
        // new image

        if (1==total_pieces) {
            // only 1 piece
            StreamImage image(stream_id,"", image_timestamp, image_width, image_height, image_quality, 
                static_cast<ColorSpace>(image_color), static_cast<ImageFormat>(image_format)); 
            char* ptr_image_buffer = (char*) std::malloc(piece_size);
            image.set_image(ptr_image_buffer,piece_size);

            detector_queue_mutex.lock();
            ptr_image_queue->push(image);
            detector_queue_mutex.unlock();

            std::free(ptr_image_buffer);
        } else {
            // piece of new image
            PiecedImage* ptr_pieced_image = new PiecedImage;
            ptr_pieced_image->base.num_width=image_width;
            ptr_pieced_image->base.num_height=image_height;
            ptr_pieced_image->base.num_quality=static_cast<uint8_t>(image_quality);
            ptr_pieced_image->base.num_timestamp=image_timestamp;
            ptr_pieced_image->base.enum_color=static_cast<ColorSpace>(image_color);
            ptr_pieced_image->base.enum_format=static_cast<ImageFormat>(image_format);
            ptr_pieced_image->base.ptr_image=nullptr;
            ptr_pieced_image->size=image_size;
            
            char* ptr_data = (char*) std::malloc(piece_size);
            std::memcpy(ptr_data,reinterpret_cast<const char*>(image_piece.first), piece_size);
            Piece* ptr_piece = new Piece;
            ptr_piece->id=curr_piece_id;
            ptr_piece->piece_size=piece_size;
            ptr_piece->ptr_data=ptr_data;
            ptr_pieced_image->pieces.push_back(ptr_piece);
            
            detector_buffer_mutex.lock();
            ptr_piece_buffer->insert(std::make_pair(image_key, ptr_pieced_image));
            detector_buffer_mutex.unlock();                              
        }
    }
}

remote::DetectorI::DetectorI() {
    detector_count_mutex.lock();
    if (!num_count) {
        ptr_piece_buffer = new PieceBuffer;
        ptr_image_queue = new ImageQueue(SERVER);
        ptr_image_queue->set_batch_size(100);
        ptr_image_queue->reg_detection_fun(nullptr, nullptr);
    }
    num_count++; 
    detector_count_mutex.unlock();
}

remote::DetectorI::DetectorI(DetectionFun_t ptr_detection_fun) {
    detector_count_mutex.lock();
    if (ptr_piece_buffer) {
        ptr_piece_buffer = new PieceBuffer;
        ptr_image_queue = new ImageQueue(SERVER);
        ptr_image_queue->set_batch_size(100);
        ptr_image_queue->reg_detection_fun(ptr_detection_fun, nullptr);
    }
    num_count++; 
    detector_count_mutex.unlock();
}

remote::DetectorI::~DetectorI(){
    detector_count_mutex.lock();
    num_count--; 
    detector_count_mutex.unlock();
    if (!num_count){
        for (auto& kv : (*ptr_piece_buffer)) {
            delete kv.second;
        }
        if (ptr_piece_buffer !=nullptr) {
            delete ptr_piece_buffer;
            ptr_piece_buffer=nullptr;
        }
        if (ptr_image_queue != nullptr) {
            delete ptr_image_queue;
            ptr_image_queue=nullptr;
        }
    }
};



void remote::ClassifierI::classify(Stream_Id stream_id,
                           Timestamp image_timestamp,
                           int image_width,
                           int image_height,
                           ::Ice::Byte image_quality,
                           ::Ice::Byte image_color,
                           ::Ice::Byte image_format,
                           short total_pieces,
                           int image_size,
                           Piece_Id curr_piece_id,
                           Bytes image_piece,
                           int piece_size, 
                           int top,
                           int bottom,
                           int left,
                           int right,
                           const Ice::Current&) {
    ImageKey object_key=std::make_pair(stream_id, image_timestamp);

    classifer_buffer_mutex.lock();
    auto it = ptr_piece_buffer->find(object_key);
    classifer_buffer_mutex.unlock();

    if (it!=ptr_piece_buffer->end()) {
        // key found

        if (total_pieces-1==it->second->pieces.size() && curr_piece_id==total_pieces-1) {
            // this is the last piece
            

            StreamImage image(stream_id,"", image_timestamp, image_width, image_height, image_quality, 
                static_cast<ColorSpace>(image_color), static_cast<ImageFormat>(image_format)); 
            BBox bbox;
            bbox.left = left; 
            bbox.right = right;
            bbox.top = top;
            bbox.bottom = bottom;

            char* ptr_image_buffer = (char*) std::malloc(image_size);
            char* ptr_image_buffer_copy=ptr_image_buffer;

            classifer_piece_mutex.lock();

            for (int i=0; i<total_pieces-1; i++){
                for (auto& piece:it->second->pieces) {
                    if (piece->id==i) {
                        std::memcpy(ptr_image_buffer_copy, piece->ptr_data,piece->piece_size);
                        ptr_image_buffer_copy+=piece->piece_size;
                        std::free(piece->ptr_data); //free the piece data
                        delete piece; // delete the piece structure
                        break;
                    }
                }
            }
            assert((ptr_image_buffer_copy+piece_size)==(ptr_image_buffer+image_size));

            // delete the pieces
            it->second->pieces.clear();
            delete it->second; // delete the PiecedImage stucture 
            classifer_piece_mutex.unlock();

            classifer_buffer_mutex.lock();
            ptr_piece_buffer->erase(object_key); // delete the map item
            classifer_buffer_mutex.unlock();

            // copy the current piece as the last
            std::memcpy(ptr_image_buffer_copy, reinterpret_cast<const char*>(image_piece.first), piece_size);
            image.set_image(ptr_image_buffer,image_size);

            classifer_queue_mutex.lock();
            ::Object object(bbox, image);
            ptr_object_queue->push(object);
            classifer_queue_mutex.unlock();

            /*
            // for debug
            using Clock = std::chrono::high_resolution_clock;
            constexpr auto num = Clock::period::num;
            constexpr auto den = Clock::period::den;
            std::cout << Clock::now().time_since_epoch().count() << " [" << num << '/' << den << "] units since epoch\n";
            std::cout << "object queue size: "<<ptr_object_queue->size()<<std::endl; 
            auto ptr_bbox= object.get_bbox();
            std::cout <<"top: "<<ptr_bbox->top<<" bottom: "<<ptr_bbox->bottom<<" left: "<<ptr_bbox->left<<" right: "<<ptr_bbox->right<<std::endl;
            std::remove("obj_image.jpg");
            std::ofstream fout("obj_image.jpg", std::ios::binary );
            fout.write(image.image(),image.size());
            */
           
            std::free(ptr_image_buffer);
        } else {
            // piece of existing image
            bool b_duplicated=false;
            for (auto& piece:it->second->pieces) {
                    if (curr_piece_id == piece->id) 
                    {
                        b_duplicated=true;
                        /*
                        std::cout<<"("<<stream_id<<","<<image_timestamp<<") ";
                        std::cout<<"duplicated piece detected: "<<curr_piece_id<<std::endl; */
                        break;
                    }
            }
            if (!b_duplicated) {
                char* ptr_data = (char*) std::malloc(piece_size);
                std::memcpy(ptr_data,reinterpret_cast<const char*>(image_piece.first), piece_size);
                Piece* ptr_piece = new Piece;
                ptr_piece->id=curr_piece_id;
                ptr_piece->piece_size=piece_size;
                ptr_piece->ptr_data=ptr_data;

                classifer_piece_mutex.lock();
                it->second->pieces.push_back(ptr_piece);
                classifer_piece_mutex.unlock();
            }
        }
    } else {
        // new image

        if (1==total_pieces) {
            // only 1 piece
            StreamImage image(stream_id,"", image_timestamp, image_width, image_height, image_quality, 
                static_cast<ColorSpace>(image_color), static_cast<ImageFormat>(image_format)); 
            char* ptr_image_buffer = (char*) std::malloc(piece_size);
            image.set_image(ptr_image_buffer,piece_size);
            BBox bbox;
            bbox.left = left; 
            bbox.right = right;
            bbox.top = top;
            bbox.bottom = bottom;

            classifer_queue_mutex.lock();
            ::Object object(bbox, image);
            ptr_object_queue->push(object);
            classifer_queue_mutex.unlock();

            std::free(ptr_image_buffer);
        } else {
            // piece of new image
            PiecedImage* ptr_pieced_image = new PiecedImage;
            ptr_pieced_image->base.num_width=image_width;
            ptr_pieced_image->base.num_height=image_height;
            ptr_pieced_image->base.num_quality=static_cast<uint8_t>(image_quality);
            ptr_pieced_image->base.num_timestamp=image_timestamp;
            ptr_pieced_image->base.enum_color=static_cast<ColorSpace>(image_color);
            ptr_pieced_image->base.enum_format=static_cast<ImageFormat>(image_format);
            ptr_pieced_image->base.ptr_image=nullptr;
            ptr_pieced_image->size=image_size;
            
            char* ptr_data = (char*) std::malloc(piece_size);
            std::memcpy(ptr_data,reinterpret_cast<const char*>(image_piece.first), piece_size);
            Piece* ptr_piece = new Piece;
            ptr_piece->id=curr_piece_id;
            ptr_piece->piece_size=piece_size;
            ptr_piece->ptr_data=ptr_data;
            ptr_pieced_image->pieces.push_back(ptr_piece);
            
            classifer_buffer_mutex.lock();
            ptr_piece_buffer->insert(std::make_pair(object_key, ptr_pieced_image));
            classifer_buffer_mutex.unlock();                              
        }
    }
}

remote::ClassifierI::ClassifierI() {
    classifer_count_mutex.lock();
    if (!num_count) {
        ptr_piece_buffer = new PieceBuffer;
        ptr_object_queue = new ObjectQueue(SERVER);
        ptr_object_queue->set_batch_size(100);
        ptr_object_queue->reg_classification_fun(nullptr);
    }
    num_count++; 
    classifer_count_mutex.unlock();
}

remote::ClassifierI::ClassifierI(ClassificationFun_t ptr_classification_fun) {
    classifer_count_mutex.lock();
    if (ptr_piece_buffer) {
        ptr_piece_buffer = new PieceBuffer;
        ptr_object_queue = new ObjectQueue(SERVER);
        ptr_object_queue->set_batch_size(100);
        ptr_object_queue->reg_classification_fun(ptr_classification_fun);
    }
    num_count++; 
    classifer_count_mutex.unlock();
}

remote::ClassifierI::~ClassifierI(){
    classifer_count_mutex.lock();
    num_count--; 
    classifer_count_mutex.unlock();
    if (!num_count){
        for (auto& kv : (*ptr_piece_buffer)) {
            delete kv.second;
        }
        if (ptr_piece_buffer !=nullptr) {
            delete ptr_piece_buffer;
            ptr_piece_buffer=nullptr;
        }
        if (ptr_object_queue != nullptr) {
            delete ptr_object_queue;
            ptr_object_queue=nullptr;
        }
    }
};


 void remote::MatchingI::match(Stream_Id stream_id, Timestamp feature_timestamp,
                            int top, int bottom, int left, int right,
                           Bytes feature_piece,
                           const Ice::Current&) {

    Feature feature;
    feature.num_stream_id = stream_id;
    feature.num_timestamp = feature_timestamp;
    feature.bbox.left=left;
    feature.bbox.right=right;
    feature.bbox.bottom=bottom;
    feature.bbox.top = top;

    matching_queue_mutex.lock();
    feature_queue.push(feature);
    matching_queue_mutex.unlock();
}

 remote::MatchingI::MatchingI() {
    matching_count_mutex.lock();
    if (!num_count) {
        feature_queue.set_batch_size(100);
        feature_queue.reg_matching_fun(nullptr);
    }
    num_count++; 
    matching_count_mutex.unlock();
}

 remote::MatchingI::MatchingI(MatchingFun_t ptr_matching_fun) {
    matching_count_mutex.lock();
    feature_queue.reg_matching_fun(ptr_matching_fun);
    num_count++; 
    matching_count_mutex.unlock();
}

 remote::MatchingI::~MatchingI(){
    matching_count_mutex.lock();
    num_count--; 
    matching_count_mutex.unlock();
};


