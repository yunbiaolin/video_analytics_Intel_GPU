module remote
{
    sequence<byte> ByteSeq;

    interface DetectorInterface
    {
        
        void detect(int streamId, 
                          long imageTimestamp, 
                          int imageWidth, 
                          int imageHeight, 
                          byte imageQality, 
                          byte imageColor, 
                          byte imageFormat,
                          short totalPieces,
                          int imageSize, 
                          short currPieceId, 
                          ["cpp:array"] ByteSeq  imagePiece,
                          int pieceSize);
    }

    interface ClassifierInterface 
    {
        void classify(int streamId, 
                          long imageTimestamp, 
                          int imageWidth, 
                          int imageHeight, 
                          byte imageQuality, 
                          byte imageColor, 
                          byte imageFormat, 
                          short totalPieces,
                          int imageSize, 
                          short currPieceId, 
                          ["cpp:array"] ByteSeq  imagePiece,
                          int pieceSize,
                          int top,
                          int bottom,
                          int left,
                          int right);
    }

    interface MatchingInterface
    {
        void match(int streamId,
                    long timestamp,
                    int top,
                    int bottom,
                    int left,
                    int right,
                    ["cpp:array"] ByteSeq featurePiece); 
    }
}