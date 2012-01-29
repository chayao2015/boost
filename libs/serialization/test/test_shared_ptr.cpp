/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_shared_ptr.cpp

// (C) Copyright 2002 Robert Ramey- http://www.rrsd.com - David Tonge  . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // NULL
#include <cstdio> // remove
#include <fstream>

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::remove;
}
#endif
#include <boost/type_traits/broken_compiler_spec.hpp>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/nvp.hpp>

#include "test_tools.hpp"

#include <boost/serialization/export.hpp>

// This is a simple class.  It contains a counter of the number
// of objects of this class which have been instantiated.
class A
{
private:
    friend class boost::serialization::access;
    int x;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_NVP(x);
    }
public:
    static int count;
    bool operator==(const A & rhs) const {
        return x == rhs.x;
    }
    A(){++count;}    // default constructor
    virtual ~A(){--count;}   // default destructor
};

//BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(A)
BOOST_SERIALIZATION_SHARED_PTR(A)

// B is a subclass of A
class B : public A
{
private:
    friend class boost::serialization::access;
    int y;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(A);
    }
public:
    static int count;
    B() : A() {};
    virtual ~B() {};
};

// B needs to be exported because its serialized via a base class pointer
BOOST_CLASS_EXPORT(B)
BOOST_SERIALIZATION_SHARED_PTR(B)

int A::count = 0;

void save(const char * testfile, const boost::shared_ptr<A>& spa)
{
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
    oa << BOOST_SERIALIZATION_NVP(spa);
}

void load(const char * testfile, boost::shared_ptr<A>& spa)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
    ia >> BOOST_SERIALIZATION_NVP(spa);
}

// trivial test
void save_and_load(boost::shared_ptr<A>& spa)
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);
    save(testfile, spa);
    boost::shared_ptr<A> spa1;
    load(testfile, spa1);

    BOOST_CHECK(
        (spa.get() == NULL && spa1.get() == NULL)
        || * spa == * spa1
    );
    std::remove(testfile);
}

void save2(
    const char * testfile, 
    const boost::shared_ptr<A>& first, 
    const boost::shared_ptr<A>& second
){
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
    oa << BOOST_SERIALIZATION_NVP(first);
    oa << BOOST_SERIALIZATION_NVP(second);
}

void load2(
    const char * testfile, 
    boost::shared_ptr<A>& first, 
    boost::shared_ptr<A>& second)
{
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
    ia >> BOOST_SERIALIZATION_NVP(first);
    ia >> BOOST_SERIALIZATION_NVP(second);
}

// Run tests by serializing two shared_ptrs into an archive,
// clearing them (deleting the objects) and then reloading the
// objects back from an archive.
void save_and_load2(boost::shared_ptr<A>& first, boost::shared_ptr<A>& second)
{
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    save2(testfile, first, second);

    // Clear the pointers, thereby destroying the objects they contain
    first.reset();
    second.reset();

    load2(testfile, first, second);

    BOOST_CHECK(first == second);
    std::remove(testfile);
}

void save3(
    const char * testfile, 
    boost::shared_ptr<A>& first, 
    boost::shared_ptr<A>& second,
    boost::weak_ptr<A>& third
){
    test_ostream os(testfile, TEST_STREAM_FLAGS);
    test_oarchive oa(os, TEST_ARCHIVE_FLAGS);
    oa << BOOST_SERIALIZATION_NVP(third);
    oa << BOOST_SERIALIZATION_NVP(first);
    oa << BOOST_SERIALIZATION_NVP(second);
}

void load3(
    const char * testfile, 
    boost::shared_ptr<A>& first, 
    boost::shared_ptr<A>& second,
    boost::weak_ptr<A>& third
){
    test_istream is(testfile, TEST_STREAM_FLAGS);
    test_iarchive ia(is, TEST_ARCHIVE_FLAGS);
    // note that we serialize the weak pointer first
    ia >> BOOST_SERIALIZATION_NVP(third);
    // inorder to test that a temporarily solitary weak pointer
    // correcttly restored.
    ia >> BOOST_SERIALIZATION_NVP(first);
    ia >> BOOST_SERIALIZATION_NVP(second);
}

void save_and_load3(
    boost::shared_ptr<A>& first, 
    boost::shared_ptr<A>& second,
    boost::weak_ptr<A>& third
){
    const char * testfile = boost::archive::tmpnam(NULL);
    BOOST_REQUIRE(NULL != testfile);

    save3(testfile, first, second, third);

    // Clear the pointers, thereby destroying the objects they contain
    first.reset();
    second.reset();
    third.reset();

    load3(testfile, first, second, third);

    BOOST_CHECK(first == second);
    BOOST_CHECK(first == third.lock());
    std::remove(testfile);
}

// This does the tests
int test_main(int /* argc */, char * /* argv */[])
{
    // These are our shared_ptrs
    boost::shared_ptr<A> spa;

    // trivial test 1
    save_and_load(spa);

    //trivival test 2
    spa = boost::shared_ptr<A>(new A);
    save_and_load(spa);

    // Try to save and load pointers to As, to a text archive
    spa = boost::shared_ptr<A>(new A);
    boost::shared_ptr<A> spa1 = spa;
    save_and_load2(spa, spa1);

    // test a weak pointer
    spa = boost::shared_ptr<A>(new A);
    spa1 = spa;
    boost::weak_ptr<A> wp = spa;
    save_and_load3(spa, spa1, wp);
    
    // Try to save and load pointers to Bs, to a text archive
    spa = boost::shared_ptr<A>(new B);
    spa1 = spa;
    save_and_load2(spa, spa1);

    // obj of type B gets destroyed
    // as smart_ptr goes out of scope
    return EXIT_SUCCESS;
}
