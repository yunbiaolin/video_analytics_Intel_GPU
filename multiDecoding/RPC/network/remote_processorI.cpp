
#include <remote_processorI.h>

void
remote::DetectorInterfaceI::detect(int /*streamId*/,
                                   long long int /*imageTimestamp*/,
                                   int /*imageWidth*/,
                                   int /*imageHeight*/,
                                   ::Ice::Byte /*imageQality*/,
                                   ::Ice::Byte /*imageColor*/,
                                   ::Ice::Byte /*imageFormat*/,
                                   short /*totalPieces*/,
                                   int /*imageSize*/,
                                   short /*currPieceId*/,
                                   ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*> /*imagePiece*/,
                                   int /*pieceSize*/,
                                   const Ice::Current& current)
{
}

void
remote::ClassifierInterfaceI::classify(int /*streamId*/,
                                       long long int /*imageTimestamp*/,
                                       int /*imageWidth*/,
                                       int /*imageHeight*/,
                                       ::Ice::Byte /*imageQuality*/,
                                       ::Ice::Byte /*imageColor*/,
                                       ::Ice::Byte /*imageFormat*/,
                                       short /*totalPieces*/,
                                       int /*imageSize*/,
                                       short /*currPieceId*/,
                                       ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*> /*imagePiece*/,
                                       int /*pieceSize*/,
                                       int /*top*/,
                                       int /*bottom*/,
                                       int /*left*/,
                                       int /*right*/,
                                       const Ice::Current& current)
{
}

void
remote::MatchingInterfaceI::match(int /*streamId*/,
                                  long long int /*timestamp*/,
                                  int /*top*/,
                                  int /*bottom*/,
                                  int /*left*/,
                                  int /*right*/,
                                  ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*> /*featurePiece*/,
                                  const Ice::Current& current)
{
}
