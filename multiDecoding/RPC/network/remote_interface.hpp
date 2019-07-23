#ifndef __DSS_POC_REMOTE_INTERFACE_HPP__
#define __DSS_POC_REMOTE_INTERFACE_HPP__

#include <map>

#include <image.hpp>
#include <image_queue.hpp>
#include <object_queue.hpp>
#include <feature_queue.hpp>
#include <remote_processor.h>
#include <utility.hpp>

namespace remote
{

typedef ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*> Bytes;

typedef short Piece_Id;
typedef int Stream_Id;
typedef long long int Timestamp;
typedef struct Piece {
    Piece_Id id;
    unsigned int piece_size;
    char* ptr_data;
} Piece;
typedef std::pair<Stream_Id, Timestamp> ImageKey;

typedef struct PiecedImage {
    ImageBase base;
    int size; 
    std::vector<Piece*> pieces;
} PiecedImage;

typedef std::map<ImageKey, PiecedImage*> PieceBuffer;


class DetectorI : public remote::DetectorInterface {
    private:
        static PieceBuffer* ptr_piece_buffer;
        static ImageQueue* ptr_image_queue;
        static unsigned num_count;
    public:
        DetectorI();
        DetectorI(DetectionFun_t ptr_detection_fun);
        DetectorI(DetectorI&) = delete;
        DetectorI& operator=(const DetectorI&) = delete;
        ~DetectorI(); 

        virtual void detect(Stream_Id stream_id,
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
                            const Ice::Current&) override;
};

class ClassifierI : public remote::ClassifierInterface {
    private:
        static PieceBuffer* ptr_piece_buffer;
        static ObjectQueue* ptr_object_queue;
        static unsigned num_count;
    public:
        ClassifierI();
        ClassifierI(ClassificationFun_t ptr_classification_fun);
        ClassifierI(ClassifierI&) = delete;
        ClassifierI& operator=(const ClassifierI&) = delete;
        ~ClassifierI(); 

        virtual void classify(Stream_Id stream_id,
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
                            const Ice::Current&) override;
};

 class MatchingI : public remote::MatchingInterface {
    private:
        static FeatureQueue feature_queue;
        static unsigned num_count;
    public:
        MatchingI();
        MatchingI(MatchingFun_t ptr_matching_fun);
        MatchingI(MatchingI&) = delete;
        MatchingI& operator=(const MatchingI&) = delete;
        ~MatchingI(); 

        virtual void match(Stream_Id stream_id, Timestamp feature_timestamp,
                            int top, int bottom, int left, int right,
                            Bytes feature_piece,
                            const Ice::Current&) override;
};


}
#endif