#ifndef __remote_processorI_h__
#define __remote_processorI_h__

#include <remote_processor.h>

namespace remote
{

class DetectorInterfaceI : public virtual DetectorInterface
{
public:

    virtual void detect(int,
                        long long int,
                        int,
                        int,
                        ::Ice::Byte,
                        ::Ice::Byte,
                        ::Ice::Byte,
                        short,
                        int,
                        short,
                        ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>,
                        int,
                        const Ice::Current&) override;
};

class ClassifierInterfaceI : public virtual ClassifierInterface
{
public:

    virtual void classify(int,
                          long long int,
                          int,
                          int,
                          ::Ice::Byte,
                          ::Ice::Byte,
                          ::Ice::Byte,
                          short,
                          int,
                          short,
                          ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>,
                          int,
                          int,
                          int,
                          int,
                          int,
                          const Ice::Current&) override;
};

class MatchingInterfaceI : public virtual MatchingInterface
{
public:

    virtual void match(int,
                       long long int,
                       int,
                       int,
                       int,
                       int,
                       ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>,
                       const Ice::Current&) override;
};

}

#endif
